#include "pch.h"

#include "utils/math/Hash.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanFormat.h"
#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/VulkanRenderPass.h"

namespace coust {
namespace render {

VkDevice VulkanRenderPass::get_device() const noexcept {
    return m_dev;
}

VkRenderPass VulkanRenderPass::get_handle() const noexcept {
    return m_handle;
}

VulkanRenderPass::VulkanRenderPass(VkDevice dev, Param const& param) noexcept
    : m_dev(dev) {
    bool const has_subpass = (param.input_attachment_mask != 0);
    bool const has_depth = (param.depth_format != VK_FORMAT_UNDEFINED);
    std::array<VkAttachmentReference2, MAX_ATTACHMENT_COUNT>
        input_attachment_ref{};
    std::array<std::array<VkAttachmentReference2, MAX_ATTACHMENT_COUNT>, 2>
        color_attachment_ref{};
    std::array<VkAttachmentReference2, MAX_ATTACHMENT_COUNT>
        resolve_attachment_ref{};
    VkAttachmentReference2 depth_attachment_ref{};
    VkAttachmentReference2 depth_resolve_attachment_ref{};
    VkSubpassDescriptionDepthStencilResolve depth_resolve_desc{};
    std::array<VkSubpassDescription2, 2> subpasses{
        VkSubpassDescription2{
                              .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
                              .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                              .inputAttachmentCount = 0,
                              .pInputAttachments = nullptr,
                              .colorAttachmentCount = 0,
                              .pColorAttachments = color_attachment_ref[0].data(),
                              .pResolveAttachments = resolve_attachment_ref.data(),
                              .pDepthStencilAttachment =
                              has_depth ? &depth_attachment_ref : nullptr,
                              },
        VkSubpassDescription2{
                              .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
                              .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                              .inputAttachmentCount = 0,
                              .pInputAttachments = input_attachment_ref.data(),
                              .colorAttachmentCount = 0,
                              .pColorAttachments = color_attachment_ref[1].data(),
                              .pResolveAttachments = resolve_attachment_ref.data(),
                              .pDepthStencilAttachment =
                              has_depth ? &depth_attachment_ref : nullptr,
                              }
    };
    // All the attachment descriptions live here, color first, then resolve,
    // followed by depth, finally depth resolve. At most 8 color attachment + 8
    // resolve attachment + depth attachment + depth resolve attachment Note:
    // this array has the same order as the framebuffer attached to this render
    // pass
    std::array<VkAttachmentDescription2,
        MAX_ATTACHMENT_COUNT + MAX_ATTACHMENT_COUNT + 1 + 1>
        attachements{};
    // there're at most 2 subpasses, so one dependency is enough
    std::array<VkSubpassDependency2, 1> dependency{
        VkSubpassDependency2{
                             .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                             .srcSubpass = 0,
                             .dstSubpass = 1,
                             .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                             .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                             // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDependencyFlagBits.html
                             // `VK_DEPENDENCY_BY_REGION_BIT` specifies that dependencies will be
                             // framebuffer-local
                             .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                             }
    };
    VkRenderPassCreateInfo2 render_pass_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .attachmentCount = 0,
        .pAttachments = attachements.data(),
        .subpassCount = has_subpass ? 2u : 1u,
        .pSubpasses = subpasses.data(),
        .dependencyCount = has_subpass ? 1u : 0u,
        .pDependencies = dependency.data(),
    };
    uint32_t attachment_idx = 0;
    // color & input attachments
    for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
        // unused slot
        if (param.color_formats[i] == VK_FORMAT_UNDEFINED)
            continue;
        uint32_t ref_idx = 0;
        // default subpass
        if (!has_subpass) {
            ref_idx = subpasses[0].colorAttachmentCount++;
            color_attachment_ref[0][ref_idx].sType =
                VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            color_attachment_ref[0][ref_idx].layout =
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            color_attachment_ref[0][ref_idx].attachment = attachment_idx;
        } else {
            // input attachment
            if (param.input_attachment_mask & (1 << i)) {
                ref_idx = subpasses[0].colorAttachmentCount++;
                color_attachment_ref[0][ref_idx].sType =
                    VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                color_attachment_ref[0][ref_idx].layout =
                    VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                color_attachment_ref[0][ref_idx].attachment = attachment_idx;
                ref_idx = subpasses[1].inputAttachmentCount++;
                input_attachment_ref[ref_idx].sType =
                    VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                input_attachment_ref[ref_idx].layout =
                    VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                input_attachment_ref[ref_idx].attachment = attachment_idx;
                // Spec:
                // aspectMask is a mask of which aspect(s) can be accessed
                // within the specified subpass as an input attachment.
                input_attachment_ref[ref_idx].aspectMask =
                    is_depth_stencil_format(param.color_formats[i]) ?
                        VK_IMAGE_ASPECT_DEPTH_BIT :
                        VK_IMAGE_ASPECT_COLOR_BIT;
            }
            ref_idx = subpasses[1].colorAttachmentCount++;
            color_attachment_ref[1][ref_idx].sType =
                VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            color_attachment_ref[1][ref_idx].layout =
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            color_attachment_ref[1][ref_idx].attachment = attachment_idx;
        }
        bool const clear = (param.clear_mask & (1 << i)) != 0;
        bool const discard = (param.discard_start_mask & (1 << i)) != 0;
        bool const is_present_src = (param.present_src_mask & (1 << i)) != 0;
        attachements[attachment_idx].sType =
            VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachements[attachment_idx].pNext = nullptr;
        attachements[attachment_idx].flags = 0;
        attachements[attachment_idx].format = param.color_formats[i];
        attachements[attachment_idx].samples = param.sample;
        attachements[attachment_idx].loadOp =
            clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
                    (discard ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                               VK_ATTACHMENT_LOAD_OP_LOAD);
        // perform store if it is not gonna be sampled
        attachements[attachment_idx].storeOp =
            param.sample == VK_SAMPLE_COUNT_1_BIT ?
                VK_ATTACHMENT_STORE_OP_STORE :
                VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // we don't use stencil attachment
        attachements[attachment_idx].stencilLoadOp =
            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachements[attachment_idx].stencilStoreOp =
            VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachements[attachment_idx].initialLayout =
            discard ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
        attachements[attachment_idx].finalLayout =
            is_present_src ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                             VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        ++attachment_idx;
    }
    // resolve attachment
    for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++i) {
        if (param.color_formats[i] == VK_FORMAT_UNDEFINED)
            continue;
        if ((param.resolve_mask & (1 << i)) == 0) {
            resolve_attachment_ref[i].sType =
                VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            resolve_attachment_ref[i].attachment = VK_ATTACHMENT_UNUSED;
            continue;
        }
        resolve_attachment_ref[i].sType =
            VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        resolve_attachment_ref[i].layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        resolve_attachment_ref[i].attachment = attachment_idx;
        attachements[attachment_idx].sType =
            VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachements[attachment_idx].pNext = nullptr;
        attachements[attachment_idx].flags = 0;
        attachements[attachment_idx].format = param.color_formats[i];
        attachements[attachment_idx].samples = VK_SAMPLE_COUNT_1_BIT;
        attachements[attachment_idx].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachements[attachment_idx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        // we don't use stencil attachment
        attachements[attachment_idx].stencilLoadOp =
            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachements[attachment_idx].stencilStoreOp =
            VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachements[attachment_idx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachements[attachment_idx].finalLayout =
            VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        ++attachment_idx;
    }
    if (has_depth) {
        const bool clear =
            (param.clear_mask & (uint32_t) AttachmentFlagBits::DEPTH) != 0;
        const bool discard_start =
            (param.discard_start_mask & (uint32_t) AttachmentFlagBits::DEPTH) !=
            0;
        const bool discard_end = (param.discard_end_mask &
                                     (uint32_t) AttachmentFlagBits::DEPTH) != 0;
        depth_attachment_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        depth_attachment_ref.attachment = attachment_idx;
        attachements[attachment_idx].sType =
            VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachements[attachment_idx].pNext = nullptr;
        attachements[attachment_idx].flags = 0;
        attachements[attachment_idx].format = param.depth_format;
        attachements[attachment_idx].samples = param.sample;
        attachements[attachment_idx].loadOp =
            clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
                    (discard_start ? VK_ATTACHMENT_LOAD_OP_DONT_CARE :
                                     VK_ATTACHMENT_LOAD_OP_LOAD);
        attachements[attachment_idx].storeOp =
            discard_end ? VK_ATTACHMENT_STORE_OP_DONT_CARE :
                          VK_ATTACHMENT_STORE_OP_STORE;
        // we don't use stencil attachment
        attachements[attachment_idx].stencilLoadOp =
            VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachements[attachment_idx].stencilStoreOp =
            VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachements[attachment_idx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachements[attachment_idx].finalLayout =
            VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        ++attachment_idx;
        if (param.depth_resolve) {
            depth_resolve_attachment_ref.sType =
                VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            depth_resolve_attachment_ref.layout =
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depth_resolve_attachment_ref.attachment = attachment_idx;
            attachements[attachment_idx].sType =
                VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            attachements[attachment_idx].pNext = nullptr;
            attachements[attachment_idx].flags = 0;
            attachements[attachment_idx].format = param.depth_format;
            attachements[attachment_idx].samples = VK_SAMPLE_COUNT_1_BIT;
            attachements[attachment_idx].loadOp =
                VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[attachment_idx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachements[attachment_idx].stencilLoadOp =
                VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[attachment_idx].stencilStoreOp =
                VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachements[attachment_idx].initialLayout =
                VK_IMAGE_LAYOUT_UNDEFINED;
            attachements[attachment_idx].finalLayout =
                VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            ++attachment_idx;
            depth_resolve_desc.sType =
                VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_depth_stencil_resolve.html
            // The VK_RESOLVE_MODE_SAMPLE_ZERO_BIT mode is the only mode that is
            // required of all implementations (that support the extension or
            // support Vulkan 1.2 or higher).
            depth_resolve_desc.depthResolveMode =
                VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
            depth_resolve_desc.stencilResolveMode = VK_RESOLVE_MODE_NONE;
            depth_resolve_desc.pDepthStencilResolveAttachment =
                &depth_resolve_attachment_ref;
            subpasses[0].pNext = &depth_resolve_desc;
            subpasses[1].pNext = &depth_resolve_desc;
        }
    }
    render_pass_info.attachmentCount = attachment_idx;
    COUST_VK_CHECK(vkCreateRenderPass2(m_dev, &render_pass_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "");
}

VulkanRenderPass::VulkanRenderPass(VulkanRenderPass&& other) noexcept
    : m_dev(other.m_dev), m_handle(other.m_handle) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanRenderPass& VulkanRenderPass::operator=(
    VulkanRenderPass&& other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    return *this;
}

VulkanRenderPass::~VulkanRenderPass() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VkExtent2D VulkanRenderPass::get_render_area_granularity() const noexcept {
    VkExtent2D res{};
    if (m_handle != VK_NULL_HANDLE)
        vkGetRenderAreaGranularity(m_dev, m_handle, &res);
    return res;
}

static_assert(detail::IsVulkanResource<VulkanRenderPass>);

}  // namespace render
}  // namespace coust

namespace std {

std::size_t hash<coust::render::VulkanRenderPass::Param>::operator()(
    coust::render::VulkanRenderPass::Param const& key) const noexcept {
    size_t h = 0;
    for (uint32_t i = 0; i < key.color_formats.size(); ++i) {
        coust::hash_combine(h, key.color_formats[i]);
    }
    coust::hash_combine(h, key.depth_format);
    coust::hash_combine(h, key.clear_mask);
    coust::hash_combine(h, key.discard_start_mask);
    coust::hash_combine(h, key.discard_end_mask);
    coust::hash_combine(h, key.sample);
    coust::hash_combine(h, key.resolve_mask);
    coust::hash_combine(h, key.input_attachment_mask);
    coust::hash_combine(h, key.present_src_mask);
    coust::hash_combine(h, key.depth_resolve);
    return h;
}

bool equal_to<coust::render::VulkanRenderPass::Param>::operator()(
    coust::render::VulkanRenderPass::Param const& left,
    coust::render::VulkanRenderPass::Param const& right) const noexcept {
    return std::ranges::equal(left.color_formats, right.color_formats) &&
           left.depth_format == right.depth_format &&
           left.clear_mask == right.clear_mask &&
           left.discard_start_mask == right.discard_start_mask &&
           left.discard_end_mask == right.discard_end_mask &&
           left.sample == right.sample &&
           left.resolve_mask == right.resolve_mask &&
           left.input_attachment_mask == right.input_attachment_mask &&
           left.present_src_mask == right.present_src_mask &&
           left.depth_resolve == right.depth_resolve;
}

}  // namespace std
