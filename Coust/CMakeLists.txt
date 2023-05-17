# To use test in source code, we must compile the library as an object library
# https://github.com/doctest/doctest/blob/master/doc/markdown/faq.md#why-are-my-tests-in-a-static-library-not-getting-registered
# https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries
if (COUST_TEST)
    add_library(Coust OBJECT)
target_sources(Coust
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_AlignedStorage.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_allocators_GrowthPolicy.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_containers_GrowthPolicy.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_Events.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_FreeListAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_GlobalAllocation.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_Logger_static.h
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_MemoryPool.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_MonotonicAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_RobinHash.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_PoolAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_SmartPointer.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_StdAdapter_StdContainer.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test.h
        ${PROJECT_SOURCE_DIR}/Coust/src/test/doctest_impl.cpp
)
else()
    add_library(Coust STATIC)
endif()

target_sources(Coust
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Coust/src/Coust.h
        ${PROJECT_SOURCE_DIR}/Coust/src/pch.cpp

        ${PROJECT_SOURCE_DIR}/Coust/src/core/EntryPoint.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Application.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Application.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Logger.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Logger.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Memory.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Memory.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Window.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/Window.cpp

        ${PROJECT_SOURCE_DIR}/Coust/src/event/ApplicationEvent.h
        ${PROJECT_SOURCE_DIR}/Coust/src/event/ApplicationEvent.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/event/Event.h
        ${PROJECT_SOURCE_DIR}/Coust/src/event/EventBus.h
        ${PROJECT_SOURCE_DIR}/Coust/src/event/KeyEvent.h
        ${PROJECT_SOURCE_DIR}/Coust/src/event/KeyEvent.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/event/MouseEvent.h
        ${PROJECT_SOURCE_DIR}/Coust/src/event/MouseEvent.cpp

        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Assert.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Compiler.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Log.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/Enums.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/TypeName.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/AlignedStorage.h

        ${PROJECT_SOURCE_DIR}/Coust/src/utils/containers/GrowthPolicy.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/containers/RobinHash.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/containers/RobinMap.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/containers/RobinSet.h

        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Allocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Area.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/Area.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/FreeListAllocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/FreeListAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/GrowthPolicy.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/HeapAllocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/HeapAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/MemoryPool.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/MemoryPool.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/MonotonicAllocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/MonotonicAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/PoolAllocator.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/PoolAllocator.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/SmartPtr.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/StlAdaptor.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/allocators/StlContainer.h
)

add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/doctest EXCLUDE_FROM_ALL)
set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/sdl EXCLUDE_FROM_ALL)

target_link_libraries(Coust 
    PRIVATE 
        spdlog::spdlog_header_only
        doctest
        SDL2::SDL2-static
)

target_include_directories(Coust 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Coust/src
        ${PROJECT_SOURCE_DIR}/Coust/third_party/spdlog/include
        ${PROJECT_SOURCE_DIR}/Coust/third_party/doctest/
        ${PROJECT_SOURCE_DIR}/Coust/third_party/sdl/include
)

target_precompile_headers(Coust 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/Coust/src/pch.h)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Coust 
        PUBLIC 
            "COUST_DBG"
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(Coust 
        PUBLIC 
            "COUST_RELDBG"
    )
else()
    target_compile_definitions(Coust 
        PUBLIC 
            "COUST_REL"
    )
endif()

target_compile_definitions(Coust 
    PRIVATE
        COUST_ROOT_PATH="${PROJECT_SOURCE_DIR}"
)