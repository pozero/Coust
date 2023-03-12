#include "pch.h"

#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

#include "Coust/Utils/Hash.h"

namespace Coust::Render::VK 
{
	RenderPass::RenderPass(ConstructParam p)
        : Base(p.ctx.Device, VK_NULL_HANDLE),
          Hashable(p.GetHash())
    {
        if (Construct(p.colorFormat, p.depthFormat, p.clearMask, p.discardStartMask, p.discardEndMask, p.sample, p.resolveMask, p.inputAttachmentMask, p.presentMask))
        {
            if (p.dedicatedName)
                SetDedicatedDebugName(p.dedicatedName);
            else if (p.scopeName)
                SetDefaultDebugName(p.scopeName, nullptr);
            else
                COUST_CORE_WARN("Render pass created without debug name");
        }
        else  
            m_Handle = VK_NULL_HANDLE;
    }

	RenderPass::RenderPass(RenderPass&& other)
        : Base(std::forward<Base>(other)),
          Hashable(std::forward<Hashable>(other))
    {
    }

	RenderPass::~RenderPass()
    {
        if (m_Handle != VK_NULL_HANDLE)
            vkDestroyRenderPass(m_Device, m_Handle, nullptr);
    }

    bool RenderPass::Construct( VkFormat colorFormat[MAX_ATTACHMENT_COUNT],
                                VkFormat depthFormat,
                                AttachmentFlags clearMask,
                                AttachmentFlags discardStartMask,
                                AttachmentFlags discardEndMask,
                                VkSampleCountFlagBits sample,
                                uint8_t resolveMask,
                                uint8_t inputAttachmentMask,
                                uint8_t presentMask)
    {
        const bool hasSubpasses = (inputAttachmentMask != 0);
        const bool hasDepth = (depthFormat != VK_FORMAT_UNDEFINED);

        VkAttachmentReference inputAttachmentRef[MAX_ATTACHMENT_COUNT];
        VkAttachmentReference colorAttachmentRef[2][MAX_ATTACHMENT_COUNT];
        VkAttachmentReference resolveAttachmentRef[MAX_ATTACHMENT_COUNT];
        VkAttachmentReference depthAttachmentRef[MAX_ATTACHMENT_COUNT];

        VkSubpassDescription subpasses[2] = 
        {
            VkSubpassDescription
            {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = 0,
                .pInputAttachments = nullptr,
                .colorAttachmentCount = 0,
                .pColorAttachments = colorAttachmentRef[0],
                .pResolveAttachments = resolveAttachmentRef,
                .pDepthStencilAttachment = hasDepth ? depthAttachmentRef : nullptr,
            },
            VkSubpassDescription
            {
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = 0,
                .pInputAttachments = inputAttachmentRef,
                .colorAttachmentCount = 0,
                .pColorAttachments = colorAttachmentRef[1],
                .pResolveAttachments = resolveAttachmentRef,
                .pDepthStencilAttachment = hasDepth ? depthAttachmentRef : nullptr,
            }
        };

        // all the attachment descriptions live here, color first, then resolve, finally depth. At most 8 color attachment + 8 resolve attachment + depth attachment
        VkAttachmentDescription attachements[MAX_ATTACHMENT_COUNT + MAX_ATTACHMENT_COUNT + 1];

        // there're at most 2 subpasses, so one dependency is enough
        VkSubpassDependency dependency[1]
        {
            VkSubpassDependency
            {
                .srcSubpass = 0,
                .dstSubpass = 1,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDependencyFlagBits.html
                // `VK_DEPENDENCY_BY_REGION_BIT` specifies that dependencies will be framebuffer-local
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            }
        };

        VkRenderPassCreateInfo renderPassCI 
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 0,
            .pAttachments = attachements,
            .subpassCount = hasSubpasses ? 2u : 1u,
            .pSubpasses = subpasses,
            .dependencyCount = hasSubpasses ? 1u : 0u,
            .pDependencies = dependency,
        };

        uint32_t curAttachmentIdx = 0;

        // color & input attachments
        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            // unused slot
            if (colorFormat[i] == VK_FORMAT_UNDEFINED)
                continue;
            uint32_t refIdx = 0;

            // default subpass
            if (!hasSubpasses)
            {
                refIdx = subpasses[0].colorAttachmentCount++;
                colorAttachmentRef[0][refIdx].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRef[0][refIdx].attachment = curAttachmentIdx;
            }
            else 
            {
                // input attachment
                if (inputAttachmentMask & (1 << i))
                {
                    refIdx = subpasses[0].colorAttachmentCount++;
                    colorAttachmentRef[0][refIdx].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    colorAttachmentRef[0][refIdx].attachment = curAttachmentIdx;

                    refIdx = subpasses[1].inputAttachmentCount++;
                    inputAttachmentRef[refIdx].layout = IsDepthOnlyFormat(colorFormat[i]) ? 
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : 
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    inputAttachmentRef[refIdx].attachment = curAttachmentIdx;
                }

                refIdx = subpasses[1].colorAttachmentCount++;
                colorAttachmentRef[1][refIdx].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRef[1][refIdx].attachment = curAttachmentIdx;
            }

            const bool clear = (clearMask & (1 << i)) != 0;
            const bool discard = (discardStartMask & (1 << i)) != 0;
            const bool present = (presentMask & (1 << i)) != 0;

            attachements[curAttachmentIdx].flags = 0;
            attachements[curAttachmentIdx].format = colorFormat[i];
            attachements[curAttachmentIdx].samples = sample;
            attachements[curAttachmentIdx].loadOp = clear ? 
                VK_ATTACHMENT_LOAD_OP_CLEAR : 
                (discard ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD);
            // perform store if it is not gonna be sampled
            attachements[curAttachmentIdx].storeOp = sample == VK_SAMPLE_COUNT_1_BIT ?
                VK_ATTACHMENT_STORE_OP_STORE :
                VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // we don't use stencil attachment
            attachements[curAttachmentIdx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[curAttachmentIdx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachements[curAttachmentIdx].initialLayout = discard ? 
                VK_IMAGE_LAYOUT_UNDEFINED :
                VK_IMAGE_LAYOUT_GENERAL;
            attachements[curAttachmentIdx].finalLayout = present ? 
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : 
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ++ curAttachmentIdx;
        }

        // resolve attachment
        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            if (colorFormat[i] == VK_FORMAT_UNDEFINED)
                continue;
            
            if ((resolveMask & (1 << i)) == 0)
            {
                resolveAttachmentRef[i].attachment = VK_ATTACHMENT_UNUSED;
                continue;
            }

            resolveAttachmentRef[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveAttachmentRef[i].attachment = curAttachmentIdx;

            attachements[curAttachmentIdx].flags = 0;
            attachements[curAttachmentIdx].format = colorFormat[i];
            attachements[curAttachmentIdx].samples = sample;
            attachements[curAttachmentIdx].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[curAttachmentIdx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            // we don't use stencil attachment
            attachements[curAttachmentIdx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[curAttachmentIdx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachements[curAttachmentIdx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachements[curAttachmentIdx].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ++ curAttachmentIdx;
        }

        if (hasDepth)
        {
            const bool clear = (clearMask & (uint32_t) AttachmentFlagBits::DEPTH) != 0;
            const bool discardStart = (discardStartMask & (uint32_t) AttachmentFlagBits::DEPTH) != 0;
            const bool discardEnd = (discardEndMask & (uint32_t) AttachmentFlagBits::DEPTH) != 0;

            depthAttachmentRef->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentRef->attachment = curAttachmentIdx;

            attachements[curAttachmentIdx].flags = 0;
            attachements[curAttachmentIdx].format = depthFormat;
            attachements[curAttachmentIdx].samples = sample;
            attachements[curAttachmentIdx].loadOp = clear ? 
                VK_ATTACHMENT_LOAD_OP_CLEAR : 
                (discardStart ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD);
            attachements[curAttachmentIdx].storeOp = discardEnd ?
                VK_ATTACHMENT_STORE_OP_DONT_CARE :
                VK_ATTACHMENT_STORE_OP_STORE;
            // we don't use stencil attachment
            attachements[curAttachmentIdx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[curAttachmentIdx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachements[curAttachmentIdx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachements[curAttachmentIdx].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            ++ curAttachmentIdx;
        }

        renderPassCI.attachmentCount = curAttachmentIdx;
        VK_CHECK(vkCreateRenderPass(m_Device, &renderPassCI, nullptr, &m_Handle));
        return true;
    }

	size_t RenderPass::ConstructParam::GetHash() const
    {
        size_t h = 0;

        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            Hash::Combine(h, colorFormat[i]);
        }

        Hash::Combine(h, depthFormat);
        Hash::Combine(h, clearMask);
        Hash::Combine(h, discardStartMask);
        Hash::Combine(h, discardEndMask);
        Hash::Combine(h, sample);
        Hash::Combine(h, resolveMask);
        Hash::Combine(h, inputAttachmentMask);
        Hash::Combine(h, presentMask);

        return h;
    }
}