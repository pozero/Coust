#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/allocators/SmartPtr.h"
#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "shaderc/shaderc.hpp"
WARNING_POP

namespace coust {
namespace render {
namespace detail {

class GlslIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    ~GlslIncluder() noexcept override = default;

    shaderc_include_result *GetInclude(const char *requested_source,
        shaderc_include_type type, const char *requesting_source,
        size_t include_depth) noexcept override;

    void ReleaseInclude(
        shaderc_include_result *include_result) noexcept override;

private:
    struct IncludeFileInfo {
        memory::string<DefaultAlloc> full_file_path{get_default_alloc()};
        memory::string<DefaultAlloc> file_content{get_default_alloc()};
    };
};

memory::vector<uint32_t, DefaultAlloc> compile_glst_to_spv(
    class ShaderSource const &source, int vk_shader_stage) noexcept;

}  // namespace detail
}  // namespace render
}  // namespace coust
