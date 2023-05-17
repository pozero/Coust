# We might need to support modle here, so always use the latest possible stable version
cmake_minimum_required(VERSION 3.26)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

project(Coust 
    VERSION 1.0
    DESCRIPTION "Painful switch to CMake"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

find_program(SCCACH_PATH sccache)
if(SCCACH_PATH)
    message(STATUS "Using sccache at ${SCCACH_PATH}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACH_PATH})
else()
    message(STATUS "Can't find sccache")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# link time optimization / interprocedual optimization
option(COUST_IPO "Enable linke time optimization (might significantly increase build time)")
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
    message(STATUS "COUST_IPO: ${COUST_IPO}")
else()
    set(COUST_IPO OFF)
    message(STATUS "IPO isn't supported: <${IPO_ERROR}>")
endif()

# test
option(COUST_TEST "Enable test in source code")
message(STATUS "COUST_TEST: ${COUST_TEST}")
option(COUST_TEST_ALL "Enable all tests in source code if test is enabled, otherwise the option is omitted")
if (COUST_TEST)
    message(STATUS "COUST_TEST_ALL: ${COUST_TEST_ALL}")
endif()

# sanitizer
# llvm on windows don't support memory sanitizer and thread sanitizer
option(COUST_SANITIZER_ADDRESS "Turn on compiler address sanitier")
option(COUST_SANITIZER_UNDEFINED "Turn on compiler undefined behaviour sanitier")

# We have to disable debug in runtime library, or will get errors similar to the following:
# [build] lld-link: error: /failifmismatch: mismatch detected for '_ITERATOR_DEBUG_LEVEL':
# [build] >>> clang_rt.asan_cxx-x86_64.lib(ubsan_type_hash_win.cpp.obj) has value 0
# [build] >>> Coust/CMakeFiles/Coust.dir/src/test/Test_AlignedStorage.cpp.obj has value 2
# that's because llvm on windows do not provide sanitizer library built in debug mode...
# also stated here:
# https://github.com/google/sanitizers/wiki/AddressSanitizerWindowsPort#debug-crt-incompatibility
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (COUST_SANITIZER_ADDRESS OR COUST_SANITIZER_UNDEFINED)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
endif()

message(STATUS "COUST_SANITIZER_ADDRESS: ${COUST_SANITIZER_ADDRESS}")
message(STATUS "COUST_SANITIZER_UNDEFINED: ${COUST_SANITIZER_UNDEFINED}")

function(COMPILATION_CONFIG TARGET)
    if (COUST_IPO)
        set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
    target_compile_definitions(${TARGET} 
        PRIVATE 
            $<$<BOOL:${COUST_TEST}>:COUST_TEST>
            $<$<BOOL:${COUST_TEST_ALL}>:COUST_TEST_ALL>
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${TARGET} 
            PRIVATE 
                /WX
                /W4
                /w14242 # 'identfier': conversion from 'type1' to 'type1', possible loss of data
                /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
                /w14263 # 'function': member function does not override any base class virtual member function
                /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
                /w14287 # 'operator': unsigned/negative constant mismatch
                /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
                /w14296 # 'operator': expression is always 'boolean_value'
                /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
                /w14545 # expression before comma evaluates to a function which is missing an argument list
                /w14546 # function call before comma missing argument list
                /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
                /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
                /w14555 # expression has no effect; expected expression with side-effect
                /w14619 # pragma warning: there is no warning number 'number'
                /w14640 # Enable warning on thread unsafe static member initialization
                /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
                /w14905 # wide string literal cast to 'LPSTR'
                /w14906 # string literal cast to 'LPWSTR'
                /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
                /fp:fast 
                /GR-
                $<$<BOOL:${COUST_SANITIZER_ADDRESS}>:/fsanitize=address>
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.0.3")
            message(FATAL_ERROR "clang version ${CMAKE_CXX_COMPILER_VERSION} is too low to use address sanitizer, minimum version required: 16.0.3: https://github.com/llvm/llvm-project/issues/56300")
        endif()
        target_compile_options(${TARGET} 
            PRIVATE 
                -Werror
                -Weverything
                -Weffc++
                -Wno-c++98-compat
                -Wno-c++98-compat-pedantic
                -Wno-c++11-compat
                -Wno-c++14-compat
                -Wno-c++17-compat
                -Wno-c++20-compat
                -Wno-c++20-extensions
                -Wno-old-style-cast
                -Wno-newline-eof
                -ffast-math 
                -fno-rtti
                $<$<CONFIG:Debug>:-fstandalone-debug>
                $<$<CONFIG:RelWithDebInfo>:-fstandalone-debug>
                $<$<BOOL:${COUST_SANITIZER_ADDRESS}>:-fsanitize=address>
                $<$<BOOL:${COUST_SANITIZER_UNDEFINED}>:-fsanitize=undefined>
        )
        target_link_options(${TARGET}
            PRIVATE 
                $<$<BOOL:${COUST_SANITIZER_ADDRESS}>:-fsanitize=address>
                $<$<BOOL:${COUST_SANITIZER_UNDEFINED}>:-fsanitize=undefined>
        )
    else()
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

add_subdirectory(Coust)
add_subdirectory(Coustol)

compilation_config(Coust)
compilation_config(Coustol)

# Copy the compile_commands.json to the project root directory
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
    if (EXISTS "${PROJECT_SOURCE_DIR}/compile_commands.json")
        file(REMOVE ${PROJECT_SOURCE_DIR}/compile_commands.json)
    endif()
    file(COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json)
endif()