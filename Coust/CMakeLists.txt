# To use test in source code, we must compile the library as an object library
# https://github.com/doctest/doctest/blob/master/doc/markdown/faq.md#why-are-my-tests-in-a-static-library-not-getting-registered
# https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries
add_library(Coust OBJECT)

target_sources(Coust
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Coust/src/Coust.h
        ${PROJECT_SOURCE_DIR}/Coust/src/pch.cpp

        ${PROJECT_SOURCE_DIR}/Coust/src/core/EntryPoint.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Application.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Application.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Logger.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Logger.cpp

        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Assert.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Compiler.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Log.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Enums.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/TypeName.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/AlignedStorage.h

        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Allocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Area.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Area.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Arena.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/FreeListAllocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/FreeListAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/GrowthPolicy.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/GrowthPolicy.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/HeapAllocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/HeapAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/SmartPtr.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/StackAllocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/StackAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/StlAdaptor.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/StlContainer.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Track.h

        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test.h
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/doctest_impl.cpp
)

add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/fmt EXCLUDE_FROM_ALL)
add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/doctest EXCLUDE_FROM_ALL)

target_link_libraries(Coust 
    PRIVATE 
        fmt::fmt-header-only
        spdlog::spdlog_header_only
        doctest
)

target_include_directories(Coust 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Coust/src
        ${PROJECT_SOURCE_DIR}/Coust/third_party/fmt/include
        ${PROJECT_SOURCE_DIR}/Coust/third_party/spdlog/include
        ${PROJECT_SOURCE_DIR}/Coust/third_party/doctest/
)

target_precompile_headers(Coust 
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/Coust/src/pch.h)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Coust 
        PRIVATE 
            "COUST_DBG"
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(Coust 
        PRIVATE 
            "COUST_RELDBG"
    )
else()
    target_compile_definitions(Coust 
        PRIVATE 
            "COUST_REL"
    )
endif()

target_compile_definitions(Coust 
    PRIVATE
        COUST_ROOT_PATH="${PROJECT_SOURCE_DIR}"
)