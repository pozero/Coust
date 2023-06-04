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
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_NaiveSerialization.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_FileCache.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/test/Test_MeshConvertion.cpp
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

        ${PROJECT_SOURCE_DIR}/Coust/src/core/layers/Layer.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/layers/RenderLayer.h
        ${PROJECT_SOURCE_DIR}/Coust/src/core/layers/RenderLayer.cpp

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
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/PtrMath.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/AlignedStorage.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/TimeStep.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/TimeStep.cpp

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

        ${PROJECT_SOURCE_DIR}/Coust/src/utils/filesystem/FileCache.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/filesystem/FileCache.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/filesystem/FileIO.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/filesystem/FileIO.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/filesystem/NaiveSerialization.h

        ${PROJECT_SOURCE_DIR}/Coust/src/utils/math/Hash.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/math/BoundingBox.h
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/math/BoundingBox.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/utils/math/NormalizedUInteger.h

        ${PROJECT_SOURCE_DIR}/Coust/src/render/Mesh.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/Mesh.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/Renderer.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/Renderer.cpp

        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/volk_impl.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/vma_impl.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanDriver.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanDriver.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanShader.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanShader.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanCommand.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanCommand.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanDescriptor.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanDescriptor.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanPipeline.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanPipeline.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanRenderPass.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanRenderPass.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanPipelineCache.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanPipelineCache.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanBuffer.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanBuffer.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanImage.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanImage.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanStagePool.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanStagePool.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanFramebuffer.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanFramebuffer.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanFBOCache.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanFBOCache.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanSwapchain.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanSwapchain.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanRenderTarget.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanRenderTarget.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanSampler.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanSampler.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanVertex.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/VulkanVertex.cpp
        
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanFormat.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanFormat.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/CacheSetting.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/CacheSetting.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanVersion.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/SpirVReflection.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/SpirVReflection.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/SpirVCompilation.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/SpirVCompilation.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanCheck.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanAllocation.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanEnum2String.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanEnum2String.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanTagger.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/vulkan/utils/VulkanAttachment.h

        ${PROJECT_SOURCE_DIR}/Coust/src/render/asset/MeshConvertion.h
        ${PROJECT_SOURCE_DIR}/Coust/src/render/asset/MeshConvertion.cpp
        ${PROJECT_SOURCE_DIR}/Coust/src/render/asset/tinygltf_impl.cpp
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/spdlog EXCLUDE_FROM_ALL)

add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/doctest EXCLUDE_FROM_ALL)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/sdl EXCLUDE_FROM_ALL)

set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF)
set(TINYGLTF_HEADER_ONLY ON)
set(TINYGLTF_INSTALL OFF)
add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/tinygltf EXCLUDE_FROM_ALL)

add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/glm EXCLUDE_FROM_ALL)

# MSVC would throw a weird fatal error about meshopt: C1033: cannot open program database
# set(MESHOPT_BUILD_DEMO OFF)
# set(MESHOPT_BUILD_GLTFPACK OFF)
# set(MESHOPT_BUILD_SHARED_LIBS OFF)
# set(MESHOPT_WERROR OFF)
# add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/meshopt EXCLUDE_FROM_ALL)

add_subdirectory(${PROJECT_SOURCE_DIR}/Coust/third_party/volk EXCLUDE_FROM_ALL)

target_link_libraries(Coust 
    PRIVATE 
        spdlog::spdlog_header_only
        doctest
        SDL2::SDL2-static
        glm::glm
        # meshoptimizer
        volk_headers
        $ENV{VULKAN_SDK}/Lib/shaderc_combined.lib
        $ENV{VULKAN_SDK}/Lib/spirv-cross-core.lib
        $ENV{VULKAN_SDK}/Lib/spirv-cross-glsl.lib
)

target_include_directories(Coust 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Coust/src
        ${PROJECT_SOURCE_DIR}/Coust/third_party/spdlog/include
        ${PROJECT_SOURCE_DIR}/Coust/third_party/doctest/
        ${PROJECT_SOURCE_DIR}/Coust/third_party/sdl/include
        ${PROJECT_SOURCE_DIR}/Coust/third_party/tinygltf
        ${PROJECT_SOURCE_DIR}/Coust/third_party/glm
        # ${PROJECT_SOURCE_DIR}/Coust/third_party/meshopt/src
        ${PROJECT_SOURCE_DIR}/Coust/third_party/vma/include
        $ENV{VULKAN_SDK}/Include
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