#include "pch.h"

#include "Coust/Render/Vulkan/RenderTarget.h"

namespace Coust::Render::VK 
{
    Attachment::Attachment(Image& image, uint16_t level, uint16_t layer) noexcept
        : image(&image), level(level), layer(layer)
    {}

    RenderTarget::RenderTarget(const ConstrucParam& param) noexcept
        : m_Depth(param.depth), m_Extent(param.extent), m_AttachedToSwapchain(false)
    {
        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            m_Color[i] = param.color[i];
        }
        if (param.sampleCount == VK_SAMPLE_COUNT_1_BIT)
            return;

        // If it's a multisample target, get or create their msaa counterparts
        m_SampleCount = std::max(param.ctx.MSAASampleCount, param.sampleCount);
        for (uint32_t i = 0; i < MAX_ATTACHMENT_COUNT; ++ i)
        {
            auto& attachment = m_Color[i];
            if (!attachment.image)
                continue;
            
            if (attachment.image->GetMSAAImage())
                m_MsaaColor[i].image = attachment.image->GetMSAAImage().get();
            else
            {
                // if the sample count of the color attachment is one, then we need to create a msaa counterpart to resolve to the color attachment
                if (attachment.image->GetSampleCount() == VK_SAMPLE_COUNT_1_BIT)
                {
                    Image::ConstructParam_Create msaaIP 
                    {
                        .ctx = param.ctx,
                        .width = attachment.image->GetExtent().width,
                        .height = attachment.image->GetExtent().height,
                        .format = attachment.image->GetFormat(),
                        .usage = Image::Usage::ColorAttachment,
                        .mipLevels = attachment.image->GetMipLevel(),
                        .samples = m_SampleCount,
                        .tiling = VK_IMAGE_TILING_OPTIMAL,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .scopeName = "RenderTarget MSAA",
                    };
                    COUST_CORE_PANIC_IF(!Image::Create(m_MsaaColor[i].image, msaaIP), "Creation msaa image for color attachment failed");
                    attachment.image->SetMASSImage(std::shared_ptr<Image>{m_MsaaColor[i].image});
                }
                // Or the sample count of the color attachment is already multisampled, which means it doesn't require resolvement.
            }
        }

        if (!param.depth.image)
            return;
        
        if (m_Depth.image->GetSampleCount() == VK_SAMPLE_COUNT_1_BIT)
        {
            Image::ConstructParam_Create msaaIP 
            {
                .ctx = param.ctx,
                .width = m_Depth.image->GetExtent().width,
                .height = m_Depth.image->GetExtent().height,
                .format = m_Depth.image->GetFormat(),
                .usage = Image::Usage::DepthStencilAttachment,
                .mipLevels = m_Depth.image->GetMipLevel(),
                .samples = m_SampleCount,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .scopeName = "RenderTarget MSAA",
            };
            COUST_CORE_PANIC_IF(!Image::Create(m_MsaaDepth.image, msaaIP), "Creation msaa image for depth attachment failed");
            m_Depth.image->SetMASSImage(std::shared_ptr<Image>{m_MsaaDepth.image});
        }
    }

    RenderTarget::RenderTarget() noexcept
        : m_AttachedToSwapchain(true)
    {}

    Attachment RenderTarget::GetColor(uint32_t idx) const noexcept { return m_Color[idx]; }

    Attachment RenderTarget::GetMsaaColor(uint32_t idx) const noexcept { return m_MsaaColor[idx]; }

    Attachment RenderTarget::GetDepth() const noexcept { return m_Depth; }

    Attachment RenderTarget::GetMsaaDepth() const noexcept { return m_MsaaDepth; }

    VkExtent2D RenderTarget::GetExtent() const noexcept { return m_Extent; }

    VkSampleCountFlagBits RenderTarget::GetSampleCount() const noexcept { return m_SampleCount; }

    bool RenderTarget::IsAttachedToSwapchain() const noexcept { return m_AttachedToSwapchain; }

    void RenderTarget::Attach(const Swapchain& swapchain) noexcept
    {
        COUST_CORE_ASSERT(m_AttachedToSwapchain, "Can't attach non-present render target to swapchain");
        m_Color[0].image = &swapchain.GetColorAttachment();
        m_Extent = swapchain.Extent;
    }
}