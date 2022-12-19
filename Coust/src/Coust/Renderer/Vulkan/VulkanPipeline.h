#pragma once

#include "Coust/Renderer/Vulkan/VulkanUtils.h"

#include "Coust/Utils/FileSystem.h"

#include <unordered_set>

namespace Coust
{
    namespace VK 
    {
        class PipelineManager
        {
        public:
            bool Initialize();
            void Cleanup();

            struct Param 
            {
                const std::vector<std::filesystem::path> shaderFiles;
                const std::vector<std::vector<const char*>> macroes;
                bool useDepth;
                bool useBlending;
                VkPipelineLayout layout;
                VkRenderPass renderpass;
            };
            bool BuildGraphics(const Param& param, VkPipeline* out_Pipeline);

        private:
            VkPipelineCache m_GraphicsCache = VK_NULL_HANDLE;

            std::vector<VkPipeline> m_CreatedPipeline{};
            std::unordered_set<VkPipelineLayout> m_UsedLayout{};
        };
    }
}
