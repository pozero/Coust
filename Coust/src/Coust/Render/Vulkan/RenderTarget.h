#pragma once 

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"
#include "Coust/Render/Vulkan/VulkanSwapchain.h"
#include "Coust/Render/Vulkan/VulkanRenderPass.h"

namespace Coust::Render::VK 
{
    class Attachment;
    class RenderTarget;

    // Attachment is basically a warpper around an image.
    class Attachment 
    {
    public:
        Attachment() = default;

        explicit Attachment(Image& image, uint16_t level, uint16_t layer) noexcept;

        Image* image = nullptr;
        uint16_t level = 65535;
        uint16_t layer = 65535;
    };

    class RenderTarget 
    {
    public:
        RenderTarget(RenderTarget&&) = delete;
        RenderTarget(const RenderTarget&) = delete;
        RenderTarget& operator=(RenderTarget&&) = delete;
        RenderTarget& operator=(const RenderTarget&) = delete;
    
    public:
        struct ConstrucParam 
        {
            const Context&              ctx;
            const StagePool&            stagePool;
            VkExtent2D                  extent;
            VkSampleCountFlagBits       sampleCount;
            Attachment                  color[MAX_ATTACHMENT_COUNT]{};
            Attachment                  depth{};
        };
        // Render pass before present
        explicit RenderTarget(const ConstrucParam& param);

        // Presenting render pass, it'll be attached to swapchain and have only one color attachment
        explicit RenderTarget();

        Attachment GetColor(uint32_t idx) const noexcept;
        Attachment GetMsaaColor(uint32_t idx) const noexcept;
        Attachment GetDepth() const noexcept;
        Attachment GetMsaaDepth() const noexcept;
        VkExtent2D GetExtent() const noexcept;
        VkSampleCountFlagBits GetSampleCount() const noexcept;
        bool IsAttachedToSwapchain() const noexcept;

        void Attach(const Swapchain& swapchain) noexcept;

    private:
        // The color attachment, it's render target or resolve target
        Attachment m_Color[MAX_ATTACHMENT_COUNT]{};
        Attachment m_MsaaColor[MAX_ATTACHMENT_COUNT]{};
        Attachment m_Depth{};
        Attachment m_MsaaDepth{};
        VkExtent2D m_Extent{ 0, 0 };
        VkSampleCountFlagBits m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
        const bool m_AttachedToSwapchain;
    };
}