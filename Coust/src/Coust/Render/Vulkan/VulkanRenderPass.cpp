#include "pch.h"

#include "Coust/Render/Vulkan/VulkanRenderPass.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"

#include "Coust/Utils/Hash.h"

namespace Coust::Render::VK 
{
	RenderPass::RenderPass(const ConstructParam& p)
        : Base(p.ctx, VK_NULL_HANDLE),
          Hashable(p.GetHash())
    {
        if (Construct(&p.colorFormat[0], p.depthFormat, p.clearMask, p.discardStartMask, p.discardEndMask, p.sample, p.resolveMask, p.inputAttachmentMask, p.depthResolve))
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
            vkDestroyRenderPass(m_Ctx.Device, m_Handle, nullptr);
    }

    bool RenderPass::Construct( const VkFormat* colorFormat,
                                VkFormat depthFormat,
                                AttachmentFlags clearMask,
                                AttachmentFlags discardStartMask,
                                AttachmentFlags discardEndMask,
                                VkSampleCountFlagBits sample,
                                uint8_t resolveMask,
                                uint8_t inputAttachmentMask,
                                bool depthResolve)
    {
        const bool hasSubpasses = (inputAttachmentMask != 0);
        const bool hasDepth = (depthFormat != VK_FORMAT_UNDEFINED);

        VkAttachmentReference2 inputAttachmentRef[MAX_ATTACHMENT_COUNT]{};
        VkAttachmentReference2 colorAttachmentRef[2][MAX_ATTACHMENT_COUNT]{};
        VkAttachmentReference2 resolveAttachmentRef[MAX_ATTACHMENT_COUNT]{};
        VkAttachmentReference2 depthAttachmentRef{};
        VkAttachmentReference2 depthResolveAttachmentRef{};
        VkSubpassDescriptionDepthStencilResolve depthResolveDesc{};

        VkSubpassDescription2 subpasses[2] = 
        {
            VkSubpassDescription2
            {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = 0,
                .pInputAttachments = nullptr,
                .colorAttachmentCount = 0,
                .pColorAttachments = colorAttachmentRef[0],
                .pResolveAttachments = resolveAttachmentRef,
                .pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : nullptr,
            },
            VkSubpassDescription2
            {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount = 0,
                .pInputAttachments = inputAttachmentRef,
                .colorAttachmentCount = 0,
                .pColorAttachments = colorAttachmentRef[1],
                .pResolveAttachments = resolveAttachmentRef,
                .pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : nullptr,
            }
        };

        // All the attachment descriptions live here, color first, then resolve, followed by depth, finally depth resolve.
        // At most 8 color attachment + 8 resolve attachment + depth attachment + depth resolve attachment
        // Note: this array has the same order as the framebuffer attached to this render pass
        VkAttachmentDescription2 attachements[MAX_ATTACHMENT_COUNT + MAX_ATTACHMENT_COUNT + 1 + 1];

        // there're at most 2 subpasses, so one dependency is enough
        VkSubpassDependency2 dependency[1]
        {
            VkSubpassDependency2
            {
                .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
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

        VkRenderPassCreateInfo2 renderPassCI 
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
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
                colorAttachmentRef[0][refIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                colorAttachmentRef[0][refIdx].layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                colorAttachmentRef[0][refIdx].attachment = curAttachmentIdx;
            }
            else 
            {
                // input attachment
                if (inputAttachmentMask & (1 << i))
                {
                    refIdx = subpasses[0].colorAttachmentCount++;
                    colorAttachmentRef[0][refIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    colorAttachmentRef[0][refIdx].layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                    colorAttachmentRef[0][refIdx].attachment = curAttachmentIdx;

                    refIdx = subpasses[1].inputAttachmentCount++;
                    inputAttachmentRef[refIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    inputAttachmentRef[refIdx].layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                    inputAttachmentRef[refIdx].attachment = curAttachmentIdx;
                    // Spec:
                    // aspectMask is a mask of which aspect(s) can be accessed within the specified subpass as an input attachment.
                    inputAttachmentRef[refIdx].aspectMask = IsDepthStencilFormat(colorFormat[i]) ?
                        VK_IMAGE_ASPECT_DEPTH_BIT :
                        VK_IMAGE_ASPECT_COLOR_BIT;
                }

                refIdx = subpasses[1].colorAttachmentCount++;
                colorAttachmentRef[1][refIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                colorAttachmentRef[1][refIdx].layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                colorAttachmentRef[1][refIdx].attachment = curAttachmentIdx;
            }

            const bool clear = (clearMask & (1 << i)) != 0;
            const bool discard = (discardStartMask & (1 << i)) != 0;

            attachements[curAttachmentIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            attachements[curAttachmentIdx].pNext = nullptr;
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
            attachements[curAttachmentIdx].finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            ++ curAttachmentIdx;
        }

        // resolve attachment
        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            if (colorFormat[i] == VK_FORMAT_UNDEFINED)
                continue;
            
            if ((resolveMask & (1 << i)) == 0)
            {
                resolveAttachmentRef[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                resolveAttachmentRef[i].attachment = VK_ATTACHMENT_UNUSED;
                continue;
            }

            resolveAttachmentRef[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            resolveAttachmentRef[i].layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            resolveAttachmentRef[i].attachment = curAttachmentIdx;

            attachements[curAttachmentIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            attachements[curAttachmentIdx].pNext = nullptr;
            attachements[curAttachmentIdx].flags = 0;
            attachements[curAttachmentIdx].format = colorFormat[i];
            attachements[curAttachmentIdx].samples = VK_SAMPLE_COUNT_1_BIT;
            attachements[curAttachmentIdx].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[curAttachmentIdx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            // we don't use stencil attachment
            attachements[curAttachmentIdx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachements[curAttachmentIdx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachements[curAttachmentIdx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachements[curAttachmentIdx].finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            ++ curAttachmentIdx;
        }

        if (hasDepth)
        {
            const bool clear = (clearMask & (uint32_t) AttachmentFlagBits::DEPTH) != 0;
            const bool discardStart = (discardStartMask & (uint32_t) AttachmentFlagBits::DEPTH) != 0;
            const bool discardEnd = (discardEndMask & (uint32_t) AttachmentFlagBits::DEPTH) != 0;

            depthAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            depthAttachmentRef.attachment = curAttachmentIdx;

            attachements[curAttachmentIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            attachements[curAttachmentIdx].pNext = nullptr;
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
            attachements[curAttachmentIdx].finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            ++ curAttachmentIdx;

            if (depthResolve)
            {
                depthResolveAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                depthResolveAttachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                depthResolveAttachmentRef.attachment = curAttachmentIdx;

                attachements[curAttachmentIdx].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
                attachements[curAttachmentIdx].pNext = nullptr;
                attachements[curAttachmentIdx].flags = 0;
                attachements[curAttachmentIdx].format = depthFormat;
                attachements[curAttachmentIdx].samples = VK_SAMPLE_COUNT_1_BIT;
                attachements[curAttachmentIdx].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachements[curAttachmentIdx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachements[curAttachmentIdx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachements[curAttachmentIdx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachements[curAttachmentIdx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachements[curAttachmentIdx].finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                ++ curAttachmentIdx;

                depthResolveDesc.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
                // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_depth_stencil_resolve.html
                // The VK_RESOLVE_MODE_SAMPLE_ZERO_BIT mode is the only mode that is required of all implementations (that support the extension or support Vulkan 1.2 or higher).
                depthResolveDesc.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
                depthResolveDesc.stencilResolveMode = VK_RESOLVE_MODE_NONE;
                depthResolveDesc.pDepthStencilResolveAttachment = &depthResolveAttachmentRef;
                subpasses[0].pNext = &depthResolveDesc;
                subpasses[1].pNext = &depthResolveDesc;
            }
        }

        renderPassCI.attachmentCount = curAttachmentIdx;
        VK_CHECK(vkCreateRenderPass2(m_Ctx.Device, &renderPassCI, nullptr, &m_Handle));
        return true;
    }

    Framebuffer::Framebuffer(const ConstructParam& p)
        : Base(p.ctx, VK_NULL_HANDLE), 
          Hashable(p.GetHash())
    {
        // all the attachment descriptions live here, color first, then resolve, finally depth. At most 8 color attachment + 8 resolve attachment + depth attachment
        // Note: this array has the same order as the render pass it attaches to
        VkImageView attachments[MAX_ATTACHMENT_COUNT + MAX_ATTACHMENT_COUNT + 1 + 1];
        uint32_t curAttachmentIdx = 0;

        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            if (p.color[i])
                attachments[curAttachmentIdx++] = p.color[i]->GetHandle();
        }

        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            if (p.resolve[i])
                attachments[curAttachmentIdx++] = p.resolve[i]->GetHandle();
        }

        if (p.depth)
            attachments[curAttachmentIdx++] = p.depth->GetHandle();
        
        if (p.depthResolve)
            attachments[curAttachmentIdx++] = p.depthResolve->GetHandle();
        
        VkFramebufferCreateInfo ci 
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = p.renderPass.GetHandle(),
            .attachmentCount = curAttachmentIdx,
            .pAttachments = attachments,
            .width = p.width,
            .height = p.height,
            .layers = p.layers,
        };

        bool success = false;
        VK_REPORT(vkCreateFramebuffer(m_Ctx.Device, &ci, nullptr, &m_Handle), success);

        if (success)
        {
            if (p.dedicatedName)
                SetDedicatedDebugName(p.dedicatedName);
            else if (p.scopeName)
                SetDefaultDebugName(p.scopeName, nullptr);
            else
                COUST_CORE_WARN("Framebuffer created without debug name");
        }
        else  
            m_Handle = VK_NULL_HANDLE;
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
        : Base(std::forward<Base>(other)),
          Hashable(std::forward<Hashable>(other))
    {
    }

    Framebuffer::~Framebuffer()
    {
        if (m_Handle != VK_NULL_HANDLE)
            vkDestroyFramebuffer(m_Ctx.Device, m_Handle, nullptr);
    }

	size_t Framebuffer::ConstructParam::GetHash() const
    {
        size_t h = 0;

        Hash::Combine(h, renderPass);
        Hash::Combine(h, width);
        Hash::Combine(h, height);
        Hash::Combine(h, layers);

        // we can end up with vulkan images of the same format but storing different infos, so hash handle is just fine
        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            if (color[i])
                Hash::Combine(h, color[i]->GetHandle());
        }

        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            if (resolve[i])
                Hash::Combine(h, resolve[i]->GetHandle());
        }

        if (depth)
            Hash::Combine(h, depth->GetHandle());

        return h;
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

        return h;
    }
}