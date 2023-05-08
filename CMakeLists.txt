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

# change binary directory by build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug/lib")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug/bin")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RelWithDebInfo/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RelWithDebInfo/lib")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RelWithDebInfo/bin")
else()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/bin")
endif()

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

# sanitizer
option(COUST_SANITIZER_ADDRESS "Turn on compiler address sanitier")
option(COUST_SANITIZER_THREAD "Turn on compiler thread sanitier")
option(COUST_SANITIZER_MEMORY "Turn on compiler memory sanitier")
option(COUST_SANITIZER_UNDEFINED "Turn on compiler undefined sanitier")

message(STATUS "COUST_SANITIZER_ADDRESS: ${COUST_SANITIZER_ADDRESS}")
message(STATUS "COUST_SANITIZER_THREAD: ${COUST_SANITIZER_THREAD}")
message(STATUS "COUST_SANITIZER_MEMORY: ${COUST_SANITIZER_MEMORY}")
message(STATUS "COUST_SANITIZER_UNDEFINED: ${COUST_SANITIZER_UNDEFINED}")

function(COMPILATION_CONFIG TARGET)
    if (COUST_IPO)
        set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
    target_compile_definitions(${TARGET} 
    PRIVATE 
            # https://github.com/llvm/llvm-project/issues/56300
            # https://github.com/llvm/llvm-project/commit/8fa0cfeb61a12ad5ca7830a8228155de32aa7ab2
            # https://github.com/google/sanitizers/wiki/AddressSanitizerWindowsPort
            # _DISABLE_STRING_ANNOTATION
            # _DISABLE_VECTOR_ANNOTATION
            $<$<BOOL:${COUST_TEST}>:COUST_TEST>
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${TARGET} 
            PRIVATE 
                /WX
                /W4
                /fp:fast 
                /GR-
                $<$<BOOL:${COUST_SANITIZER_ADDRESS}>:/fsanitize=address>
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
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
                # $<$<BOOL:${COUST_SANITIZER_ADDRESS}>:-fsanitize=address>
                # $<$<BOOL:${COUST_SANITIZER_THREAD}>:-fsanitize=thread>
                # $<$<BOOL:${COUST_SANITIZER_MEMORY}>:-fsanitize=memory>
                # $<$<BOOL:${COUST_SANITIZER_UNDEFINED}>:-fsanitize=undefined>
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
if (EXISTS "${PROJECT_SOURCE_DIR}/build/compile_commands.json")
    file(COPY_FILE build/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json)
endif()