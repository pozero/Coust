#include "pch.h"

#include "utils/Compiler.h"
#include "utils/allocators/StlContainer.h"
#include "core/Memory.h"
#include "render/asset/MeshConvertion.h"
#include "render/vulkan/VulkanDriver.h"
#include "render/Renderer.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

Renderer::Renderer() noexcept {
    memory::vector<const char*, DefaultAlloc> inst_ext{get_default_alloc()};
    memory::vector<const char*, DefaultAlloc> inst_layer{get_default_alloc()};
    memory::vector<const char*, DefaultAlloc> dev_ext{get_default_alloc()};
    VkPhysicalDeviceFeatures required_phydev_features{};
    const void* inst_creation_pnext = nullptr;
    const void* dev_creation_pnext = nullptr;
    dev_ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    dev_ext.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
    VkPhysicalDeviceSynchronization2Features phydev_sync2_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = nullptr,
        .synchronization2 = VK_TRUE,
    };
    dev_creation_pnext = &phydev_sync2_feature;
#if defined(COUST_VK_DBG)
    inst_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    inst_layer.push_back("VK_LAYER_KHRONOS_validation");
#endif
    m_vk_driver.initialize(inst_ext, inst_layer, dev_ext,
        required_phydev_features, inst_creation_pnext, dev_creation_pnext);
    m_attached_render_target = &m_vk_driver.get().get_attached_render_target();
}

void Renderer::begin_frame() noexcept {
    m_vk_driver.get().update_swapchain();
    m_vk_driver.get().begin_frame();
}

void Renderer::render(std::filesystem::path gltf_path,
    std::filesystem::path vert_shader_path,
    std::filesystem::path frag_shader_path) noexcept {
    auto model_iter =
        m_path_to_idx.find({gltf_path.string().c_str(), get_default_alloc()});
    uint32_t vertex_index_buf_idx = 0;
    if (model_iter != m_path_to_idx.end()) {
        vertex_index_buf_idx = model_iter.mapped();
    } else {
        m_gltfes.push_back(process_gltf(gltf_path));
        m_vertex_index_bufes.push_back(
            m_vk_driver.get().create_vertex_index_buffer(m_gltfes.back()));
        vertex_index_buf_idx = (uint32_t) m_gltfes.size() - 1;
        m_path_to_idx.emplace(
            memory::string<DefaultAlloc>{
                gltf_path.string().c_str(), get_default_alloc()},
            vertex_index_buf_idx);
    }
    VulkanVertexIndexBuffer const& vertex_index_buf =
        m_vertex_index_bufes[vertex_index_buf_idx];
    m_vk_driver.get().begin_render_pass(*m_attached_render_target,
        AttachmentFlagBits::COLOR0, 0, 0, 0, VkClearValue{});
    m_vk_driver.get().bind_shader(
        VK_SHADER_STAGE_VERTEX_BIT, vert_shader_path, {});
    m_vk_driver.get().bind_shader(
        VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader_path, {});
    m_vk_driver.get().draw(
        vertex_index_buf, VulkanGraphicsPipeline::RasterState{}, {});
    m_vk_driver.get().end_render_pass();
}

void Renderer::end_frame() noexcept {
    m_vk_driver.get().end_frame();
    m_vk_driver.get().commit();
}

}  // namespace render
}  // namespace coust
