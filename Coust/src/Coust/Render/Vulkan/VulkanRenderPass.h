#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"

namespace Coust::Render::VK
{
	class RenderPass;
	class Framebuffer;

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
		// TODO: use VkCreateRenderPass2 to support automatic depth desolve
		struct ConstructParam
		{
			const Context&              ctx;
			VkFormat                    colorFormat[MAX_ATTACHMENT_COUNT]{};	// specifying the format for the attachment, `VK_FORMAT_UNDEFINED` means we don't use this attachment
			VkFormat                    depthFormat = VK_FORMAT_UNDEFINED;		// `VK_FORMAT_UNDEFINED` means we don't use depth attachment
			AttachmentFlags             clearMask = 0u;							// `(clear & COLOR0) != 0` means `COLOR0` needs clear operation while loading
			AttachmentFlags             discardStartMask = 0u;					// `(discardStartMask & COLOR0) != 0` means `COLOR0` should be discarded while loading (loading operation will be clear if set)
			AttachmentFlags             discardEndMask = 0u;					// `(discardEndMask & COLOR0) != 0` means `COLOR0` should be discarded whil storing
			VkSampleCountFlagBits       sample = VK_SAMPLE_COUNT_1_BIT;
			uint8_t                     resolveMask = 0u;						// `(resolveMask & COLOR0) != 0` means `COLOR0` has a coorsponding color resolve attachment
			uint8_t                     inputAttachmentMask = 0u;				// `(inputAttachmentMask & COLOR0) != 0` means `COLOR0` should be used as input attachment for the second subpass
			bool 						depthResolve = false;					// use resolve for depth attachment or not
			const char*                 dedicatedName = nullptr;
			const char*                 scopeName = nullptr;

			size_t GetHash() const;
		};
		explicit RenderPass(const ConstructParam& p);

		RenderPass(RenderPass&& other) noexcept;

		~RenderPass();

		RenderPass() = delete;
		RenderPass(const RenderPass&) = delete;
		RenderPass& operator=(const RenderPass&) = delete;
		RenderPass& operator=(RenderPass&&) = delete;

		const VkExtent2D GetRenderAreaGranularity() const noexcept;

	private:
		bool Construct( const VkFormat* colorFormat,
						VkFormat depthFormat,
						AttachmentFlags clearMask,
						AttachmentFlags discardStartMask,
						AttachmentFlags discardEndMask,
						VkSampleCountFlagBits sample,
						uint8_t resolveMask,
						uint8_t inputAttachmentMask,
						bool depthResolve);
		
		// Fake constructor, the constructed object is used for searching
		friend class FBOCache;
		explicit RenderPass(const ConstructParam* p, int) noexcept;
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
			Image::View*				color[MAX_ATTACHMENT_COUNT]{};		// the unused attachment slot should be nulled out
			Image::View*				resolve[MAX_ATTACHMENT_COUNT]{};		// the unused attachment slot should be nulled out
			Image::View* 				depth = nullptr;
			Image::View* 				depthResolve = nullptr;
			const char*                 dedicatedName = nullptr;
			const char*					scopeName = nullptr;

			size_t GetHash() const;
		};
		explicit Framebuffer(const ConstructParam& p);

		Framebuffer(Framebuffer&& other) noexcept;

		~Framebuffer();

		Framebuffer() = delete;
		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
		Framebuffer& operator=(Framebuffer&&) = delete;

		const RenderPass& GetRenderPass() const noexcept;

	private:
		// Fake constructor, the constructed object is used for searching
		friend class FBOCache;
		explicit Framebuffer(const ConstructParam* p, int) noexcept;

	private:
		const RenderPass& m_RenderPass;
	};

	// Cache class for both render pass and framebuffer (aka frame buffer object)
	class FBOCache
	{
	public:
		FBOCache(FBOCache&&) = delete;
		FBOCache(const FBOCache&) = delete;
		FBOCache& operator=(FBOCache&&) = delete;
		FBOCache& operator=(const FBOCache&) = delete;

	public:
		FBOCache();

		const RenderPass* GetRenderPass(const RenderPass::ConstructParam& p);

		const Framebuffer* GetFramebuffer(const Framebuffer::ConstructParam& p);

		void GC();

		void Reset() noexcept;

	private:
		// render pass -> last time it's accessed
		std::unordered_map<RenderPass, uint32_t, Hash::HashFn<RenderPass>, Hash::EqualFn<RenderPass>> m_CachedRenderPasses;

		// framebuffer -> last time it's accessed
		std::unordered_map<Framebuffer, uint32_t, Hash::HashFn<Framebuffer>, Hash::EqualFn<Framebuffer>> m_CachedFramebuffers;

		// pointer to the render pass -> the reference count (aka the number of framebuffer attached to it) of the render pass
		std::unordered_map<const RenderPass*, uint32_t> m_RenderPassReferenceCount;

		EvictTimer m_Timer;
	};
}