#include "pch.h"

#include "render/vulkan/StagePool.h"

namespace coust {
namespace render {

StagePool::StagePool(VkDevice dev, VmaAllocator alloc) noexcept
    : m_dev(dev),
      m_alloc(alloc),
      m_gc_timer(GARBAGE_COLLECTION_PERIOD),
      m_buffer_hit_counter("Stage Pool [Buffer]"),
      m_image_hit_counter("Stage Pool [Image]") {
}

memory::shared_ptr<VulkanBuffer> StagePool::acquire_staging_buf(
    VkDeviceSize size) noexcept {
    std::optional<uint32_t> best_staging_buf{};
    for (uint32_t i = 0; i < m_free_staging_bufs.size(); ++i) {
        auto const& staging_buf = m_free_staging_bufs[i];
        if (staging_buf.buf->get_size() >= size) {
            if (!best_staging_buf.has_value() ||
                (m_free_staging_bufs[best_staging_buf.value()].buf->get_size() >
                    staging_buf.buf->get_size())) {
                best_staging_buf = i;
            }
        }
    }
    if (best_staging_buf.has_value()) {
        m_buffer_hit_counter.hit();
        auto iter = m_free_staging_bufs.begin() + best_staging_buf.value();
        auto buf = iter->buf;
        m_used_staging_bufs.emplace_back(
            StagingBuffer{buf, m_gc_timer.current_count()});
        m_free_staging_bufs.erase(iter);
        return buf;
    } else {
        m_buffer_hit_counter.miss();
        std::array<uint32_t, 3> constexpr related_queues{
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
        };
        auto buf = memory::allocate_shared<VulkanBuffer>(get_default_alloc(),
            m_dev, m_alloc, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VulkanBuffer::Usage::staging, related_queues);
        m_used_staging_bufs.emplace_back(
            StagingBuffer{buf, m_gc_timer.current_count()});
        return buf;
    }
}

memory::shared_ptr<VulkanHostImage> StagePool::acquire_staging_img(
    VkCommandBuffer cmdbuf, VkFormat format, uint32_t width,
    uint32_t height) noexcept {
    auto iter = std::ranges::find_if(
        m_free_staging_imgs, [format, width, height](StagingImage const& img) {
            auto const [w, h] = img.img->get_extent();
            return img.img->get_format() == format && w == width && h == height;
        });
    if (iter != m_free_staging_imgs.end()) {
        m_image_hit_counter.hit();
        auto stage = *iter;
        m_free_staging_imgs.erase(iter);
        stage.last_accessed = m_gc_timer.current_count();
        m_used_staging_imgs.push_back(std::move(stage));
        return stage.img;
    } else {
        m_image_hit_counter.miss();
        auto stage_img = memory::allocate_shared<VulkanHostImage>(
            get_default_alloc(), m_dev, m_alloc, cmdbuf, format, width, height);
        m_used_staging_imgs.push_back(
            StagingImage{stage_img, m_gc_timer.current_count()});
        return stage_img;
    }
}

void StagePool::gc() noexcept {
    m_gc_timer.tick();

    {
        memory::vector<StagingBuffer, DefaultAlloc> tmp{get_default_alloc()};
        tmp.swap(m_free_staging_bufs);
        for (auto const& staging_buf : tmp) {
            if (!m_gc_timer.should_recycle(staging_buf.last_accessed)) {
                m_free_staging_bufs.push_back(std::move(staging_buf));
            }
        }
    }

    {
        memory::vector<StagingBuffer, DefaultAlloc> tmp{get_default_alloc()};
        tmp.swap(m_used_staging_bufs);
        for (auto& staging_buf : tmp) {
            if (staging_buf.buf.use_count() == 1) {
                staging_buf.last_accessed = m_gc_timer.current_count();
                m_free_staging_bufs.push_back(std::move(staging_buf));
            } else {
                m_used_staging_bufs.push_back(std::move(staging_buf));
            }
        }
    }

    {
        memory::vector<StagingImage, DefaultAlloc> tmp{get_default_alloc()};
        tmp.swap(m_free_staging_imgs);
        for (auto const& staging_img : tmp) {
            if (!m_gc_timer.should_recycle(staging_img.last_accessed)) {
                m_free_staging_imgs.push_back(std::move(staging_img));
            }
        }
    }

    {
        memory::vector<StagingImage, DefaultAlloc> tmp{get_default_alloc()};
        tmp.swap(m_used_staging_imgs);
        for (auto& staging_img : tmp) {
            if (staging_img.img.use_count() == 1) {
                staging_img.last_accessed = m_gc_timer.current_count();
                m_free_staging_imgs.push_back(std::move(staging_img));
            } else {
                m_used_staging_imgs.push_back(std::move(staging_img));
            }
        }
    }
}

void StagePool::reset() noexcept {
    m_free_staging_bufs.clear();
    m_used_staging_bufs.clear();
    m_free_staging_imgs.clear();
    m_used_staging_imgs.clear();
}

}  // namespace render
}  // namespace coust
