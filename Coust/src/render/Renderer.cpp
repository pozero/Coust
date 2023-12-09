#include "pch.h"

#include "utils/math/Hash.h"
#include "utils/filesystem/FileCache.h"
#include "utils/filesystem/NaiveSerialization.h"
#include "utils/Compiler.h"
#include "utils/allocators/StlContainer.h"
#include "core/Memory.h"
#include "render/asset/MeshConvertion.h"
#include "render/vulkan/VulkanDriver.h"
#include "render/Renderer.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
// #include "glm/gtx/euler_angles.hpp"
WARNING_POP

namespace coust {
namespace render {

Renderer::Renderer() noexcept
    : m_camera(glm::vec3{0.0f, 0.0f, 2.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
          glm::vec3{0.0f, -1.0f, 0.0f}) {
    memory::vector<const char*, DefaultAlloc> inst_ext{get_default_alloc()};
    memory::vector<const char*, DefaultAlloc> inst_layer{get_default_alloc()};
    memory::vector<const char*, DefaultAlloc> dev_ext{get_default_alloc()};
    VkPhysicalDeviceFeatures required_phydev_features{};
    required_phydev_features.fillModeNonSolid = VK_TRUE;
    required_phydev_features.multiDrawIndirect = VK_TRUE;
    const void* inst_creation_pnext = nullptr;
    const void* dev_creation_pnext = nullptr;
    dev_ext.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    dev_ext.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
    VkPhysicalDeviceMaintenance4Features phydev_maintenance4_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES,
        .pNext = nullptr,
        .maintenance4 = VK_TRUE,
    };
    VkPhysicalDeviceSynchronization2Features phydev_sync2_feature{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &phydev_maintenance4_feature,
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

Renderer::~Renderer() noexcept {
    m_vk_driver.get().wait();
    m_vertex_index_bufes.clear();
    m_transformation_bufes.clear();
    m_vk_driver.destroy();
}

void Renderer::prepare(std::filesystem::path transformation_comp_shader_path,
    std::filesystem::path gltf_path, std::filesystem::path vert_shader_path,
    std::filesystem::path frag_shader_path) noexcept {
    auto idx_iter =
        m_path_to_idx.find({gltf_path.string().c_str(), get_default_alloc()});
    if (idx_iter != m_path_to_idx.end()) {
        m_cur_idx = idx_iter.mapped();
    } else {
        size_t gltf_hash_tag = calc_std_hash(gltf_path);
        auto [byte_array, cache_status] =
            file::Caches::get_instance().get_cache_data(
                gltf_path.string(), gltf_hash_tag);
        if (cache_status == file::Caches::Status::available) {
            m_gltfes.push_back(
                file::from_byte_array<MeshAggregate>(byte_array));
        } else {
            m_gltfes.push_back(process_gltf(gltf_path));
            file::Caches::get_instance().add_cache_data(gltf_path.string(),
                gltf_hash_tag, file::to_byte_array(m_gltfes.back()), true);
        }
        m_vertex_index_bufes.push_back(
            m_vk_driver.get().create_vertex_index_buffer(m_gltfes.back()));
        m_transformation_bufes.push_back(
            m_vk_driver.get().create_transformation_buffer(m_gltfes.back()));
        m_cur_idx = (uint32_t) m_gltfes.size() - 1;
        m_path_to_idx.emplace(
            memory::string<DefaultAlloc>{
                gltf_path.string().c_str(), get_default_alloc()},
            m_cur_idx);
    }
    m_vk_driver.get().bind_shader(VK_PIPELINE_BIND_POINT_GRAPHICS,
        VK_SHADER_STAGE_VERTEX_BIT, vert_shader_path);
    m_vk_driver.get().bind_shader(VK_PIPELINE_BIND_POINT_GRAPHICS,
        VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader_path);
    m_vk_driver.get().bind_shader(VK_PIPELINE_BIND_POINT_COMPUTE,
        VK_SHADER_STAGE_COMPUTE_BIT, transformation_comp_shader_path);
}

void Renderer::begin_frame() noexcept {
    m_vk_driver.get().calculate_transformation(
        m_transformation_bufes[m_cur_idx]);
    m_vk_driver.get().commit_compute();
    m_vk_driver.get().add_compute_to_graphics_dependency();
    m_vk_driver.get().update_swapchain();
    m_vk_driver.get().begin_frame();
}

void Renderer::render() noexcept {
    VulkanVertexIndexBuffer const& vertex_index_buf =
        m_vertex_index_bufes[m_cur_idx];
    VulkanTransformationBuffer const& trans_buf =
        m_transformation_bufes[m_cur_idx];
    VkClearValue clear_val{
        .color = {.float32 = {0.7f, 0.7f, 0.7f, 1.0f}},
    };
    m_vk_driver.get().begin_render_pass(*m_attached_render_target,
        AttachmentFlagBits::COLOR0, 0, 0, 0, clear_val);
    VulkanGraphicsPipeline::RasterState raster_state{};
    raster_state.front_face = VK_FRONT_FACE_CLOCKWISE;
    raster_state.polygon_mode = VK_POLYGON_MODE_LINE;
    glm::mat4 proj_view_mat =
        m_camera.get_proj_matrix() * m_camera.get_view_matrix();
    m_vk_driver.get().draw(
        vertex_index_buf, trans_buf, proj_view_mat, raster_state, {});
    m_vk_driver.get().end_render_pass();
}

void Renderer::end_frame() noexcept {
    m_vk_driver.get().end_frame();
    m_vk_driver.get().graphics_commit_present();
}

FPSCamera& Renderer::get_camera() noexcept {
    return m_camera;
}

}  // namespace render
}  // namespace coust
