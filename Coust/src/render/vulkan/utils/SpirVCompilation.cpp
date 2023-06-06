#include "pch.h"

#include "utils/Compiler.h"
#include "utils/filesystem/FileIO.h"
#include "render/vulkan/VulkanShader.h"
#include "render/vulkan/utils/VulkanVersion.h"
#include "render/vulkan/utils/SpirVCompilation.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {
namespace detail {

shaderc_include_result *GlslIncluder::GetInclude(const char *requested_source,
    [[maybe_unused]] shaderc_include_type type, const char *requesting_source,
    [[maybe_unused]] size_t include_depth) noexcept {
    IncludeFileInfo *include_info =
        get_default_alloc().construct<IncludeFileInfo>();
    std::filesystem::path include_path{requested_source};
    if (include_path.is_absolute()) {
        include_info->full_file_path = include_path.string().c_str();
    } else {
        include_path =
            std::filesystem::path{requesting_source}.parent_path().append(
                requested_source);
        include_info->full_file_path = include_path.string().c_str();
    }
    file::ByteArray byte_array = file::read_file_whole(include_path);
    include_info->file_content = byte_array.to_string_view();
    shaderc_include_result *result =
        get_default_alloc().construct<shaderc_include_result>(
            include_info->full_file_path.c_str(),
            include_info->full_file_path.length(),
            include_info->file_content.c_str(),
            include_info->file_content.length(), (void *) include_info);
    return result;
}

void GlslIncluder::ReleaseInclude(
    shaderc_include_result *include_result) noexcept {
    IncludeFileInfo *info = (IncludeFileInfo *) include_result->user_data;
    get_default_alloc().destruct(info);
}

constexpr shaderc_env_version get_shaderc_evn_ver(uint32_t vulkanVer) noexcept {
    switch (vulkanVer) {
        case VK_API_VERSION_1_0:
            return shaderc_env_version_vulkan_1_0;
        case VK_API_VERSION_1_1:
            return shaderc_env_version_vulkan_1_1;
        case VK_API_VERSION_1_2:
            return shaderc_env_version_vulkan_1_2;
        case VK_API_VERSION_1_3:
            return shaderc_env_version_vulkan_1_3;
        default:
            return shaderc_env_version_vulkan_1_3;
    }
}

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wswitch-enum")
constexpr shaderc_shader_kind stage_to_shaderc_shader_type(
    VkShaderStageFlagBits stage) noexcept {
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return shaderc_glsl_default_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return shaderc_glsl_default_fragment_shader;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return shaderc_glsl_default_geometry_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return shaderc_glsl_default_compute_shader;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return shaderc_glsl_default_tess_control_shader;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return shaderc_glsl_default_tess_evaluation_shader;
        default:
            return shaderc_glsl_infer_from_source;
    }
}
WARNING_POP

memory::vector<uint32_t, DefaultAlloc> compile_glst_to_spv(
    ShaderSource const &source, int vk_shader_stage) noexcept {
    shaderc::Compiler compiler{};
    shaderc::CompileOptions opt{};
    opt.SetIncluder(std::make_unique<GlslIncluder>());
    opt.SetSourceLanguage(shaderc_source_language_glsl);
    opt.SetTargetEnvironment(shaderc_target_env_vulkan,
        get_shaderc_evn_ver(COUST_VULKAN_API_VERSION));
    opt.SetOptimizationLevel(shaderc_optimization_level_zero);
    opt.AddMacroDefinition("__VK_GLSL__", "1");
    for (auto const &[name, val] : source.get_macros()) {
        opt.AddMacroDefinition(name.c_str(), val.c_str());
    }
    auto result = compiler.CompileGlslToSpv(source.get_code().c_str(),
        source.get_code().length(),
        stage_to_shaderc_shader_type((VkShaderStageFlagBits) vk_shader_stage),
        source.get_path().string().c_str(), opt);
    COUST_PANIC_IF_NOT(
        result.GetCompilationStatus() == shaderc_compilation_status_success,
        "GLSL compilation failed, shaderc reported: {}",
        result.GetErrorMessage());
    memory::vector<uint32_t, DefaultAlloc> ret{
        result.begin(), result.end(), get_default_alloc()};
    return ret;
}

}  // namespace detail
}  // namespace render
}  // namespace coust
