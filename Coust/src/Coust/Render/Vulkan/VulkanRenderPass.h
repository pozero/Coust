#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

namespace Coust::Render::VK
{
	class RenderPass;
	class Framebuffer;

	class ImageView;

	constexpr uint32_t MAX_ATTACHMENT_COUNT = 8;
	enum AttachmentFlagBits : uint32_t
	{
		// support at most 8 color attachment (from filament)
		// then we can use several single uint8_t masks to specify their proerties
		NONE  	  = 0X0,
		COLOR0	  = (1 << 0),
		COLOR1	  = (1 << 1),
		COLOR2	  = (1 << 2),
		COLOR3	  = (1 << 3),
		COLOR4	  = (1 << 4),
		COLOR5	  = (1 << 5),
		COLOR6	  = (1 << 6),
		COLOR7	  = (1 << 7),
		DEPTH 	  = (1 << 8),
		COLOR_ALL = COLOR0 | COLOR1 | COLOR2 | COLOR3 |
					COLOR4 | COLOR5 | COLOR6 | COLOR7,
		ALL 	  = COLOR_ALL | DEPTH,
	};
	using AttachmentFlags = uint32_t;

	class RenderPass : public Resource<VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS>,
					   public Hashable
	{
	public:
		using Base = Resource<VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS>;

	public:
		// Each render pass can have at most 2 subpasses (e.g. deferred rendering)
		struct ConstructParam
		{
			const Context& 				ctx;
			VkFormat 					colorFormat[MAX_ATTACHMENT_COUNT];	// specifying the format for the attachment, `VK_FORMAT_UNDEFINED` means we don't use this attachment
			VkFormat 					depthFormat;
			AttachmentFlags 			clearMask = 0u;						// `(clear & COLOR0) != 0` means `COLOR0` needs clear operation while loading
			AttachmentFlags 			discardStartMask = 0u;				// `(discardStartMask & COLOR0) != 0` means `COLOR0` should be discarded while loading (loading operation will be clear if set)
			AttachmentFlags 			discardEndMask = 0u;				// `(discardEndMask & COLOR0) != 0` means `COLOR0` should be discarded whil storing
			VkSampleCountFlagBits 		sample = VK_SAMPLE_COUNT_1_BIT;
			uint8_t 					resolveMask = 0u;					// `(resolveMask & COLOR0) != 0` means `COLOR0` has a coorsponding color resolve attachment
			uint8_t 					inputAttachmentMask = 0u;			// `(inputAttachmentMask & COLOR0) != 0` means `COLOR0` should be used as input attachment for the second subpass
			uint8_t 					presentMask = 0u;					// if the color attachment is gonna be presented
			const char*                 dedicatedName = nullptr;
			const char*					scopeName = nullptr;

			size_t GetHash() const;
		};
		RenderPass(ConstructParam p);

		RenderPass(RenderPass&& other);

		~RenderPass();

		RenderPass() = delete;
		RenderPass(const RenderPass&) = delete;
		RenderPass& operator=(const RenderPass&) = delete;
		RenderPass& operator=(RenderPass&&) = delete;

		const VkExtent2D GetRenderAreaGranularity() const;

	private:
		bool Construct( VkFormat colorFormat[MAX_ATTACHMENT_COUNT],
						VkFormat depthFormat,
						AttachmentFlags clearMask,
						AttachmentFlags discardStartMask,
						AttachmentFlags discardEndMask,
						VkSampleCountFlagBits sample,
						uint8_t resolveMask,
						uint8_t inputAttachmentMask,
						uint8_t presentMask);
	};

	class Framebuffer : public Resource<VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER>,
						public Hashable
	{
	public:
		using Base = Resource<VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER>;

	public:
		struct ConstructParam 
		{
			const Context& 				ctx;
			const RenderPass& 			renderPass;
			uint32_t 					width;
			uint32_t 					height;
			uint32_t 					layers = 1u;
			ImageView* 					color[MAX_ATTACHMENT_COUNT];	// the unused attachment slot should be nulled out
			ImageView* 					resolve[MAX_ATTACHMENT_COUNT];	// the unused attachment slot should be nulled out
			ImageView* 					depth;
			const char*                 dedicatedName = nullptr;
			const char*					scopeName = nullptr;

			size_t GetHash() const;
		};
		Framebuffer(ConstructParam p);

		Framebuffer(Framebuffer&& other) noexcept;

		~Framebuffer();

		Framebuffer() = delete;
		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
		Framebuffer& operator=(Framebuffer&&) = delete;

	private:
		bool Construction(const Context& ctx, 
						  const RenderPass& renderPass, 
						  uint32_t width, 
						  uint32_t height, 
						  uint32_t layers, 
						  ImageView* color[MAX_ATTACHMENT_COUNT],
						  ImageView* resolve[MAX_ATTACHMENT_COUNT],
						  ImageView* depth);
	};
}