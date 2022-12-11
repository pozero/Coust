#pragma once

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

namespace Coust
{
    namespace VK
    {
        class FramebufferManager
        {
        public: 
			struct Param
			{
				uint32_t count;
				uint32_t width, height;
				VkRenderPass renderPass;
				VkImageView colorImageView;
				VkImageView depthImageView;
                const VkImageView* resolveImageViews;
			};

        public:
            FramebufferManager() = default;
            ~FramebufferManager() = default;
            
            void Cleanup();

            bool CreateFramebuffersAttachedToSwapchain(VkRenderPass renderPass, bool useDepth, VkFramebuffer* out_Framebuffers);
            
            bool RecreateFramebuffersAttachedToSwapchain();
            
        private:
            bool CreateFramebuffers(const Param& params, VkFramebuffer* out_Framebuffers);     

        private:
            struct AttachedToSwapchain
            {
                VkFramebuffer* framebuffer;
                VkRenderPass renderPass;
                bool useDepth;
            };
            std::vector<AttachedToSwapchain> m_FramebuffersAttachedToSwapchain{};

        };
    }
}
