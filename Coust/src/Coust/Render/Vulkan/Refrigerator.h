#pragma once

#include <functional>
#include <unordered_map>

namespace Coust::Render::VK
{
    // Some vulkan resources created by renderer (buffer, texture) should be deleted after the commands related to it are all submitted.
    // We refrigerate them here utill they are expired, then we can pour it into the trash.
    class Refrigerator
    {
    public:
        Refrigerator(Refrigerator&&) = delete;
        Refrigerator(const Refrigerator&) = delete;
        Refrigerator& operator=(Refrigerator&&) = delete;
        Refrigerator& operator=(const Refrigerator&) = delete;

    public:
        explicit Refrigerator() = default;

        void Reset() noexcept;

        template <typename T>
        void Register(T* resource)
        {
            m_Resources[resource].deletor = [](void* res)
            {
                T* realType = (T*) res;
                delete realType;
            };
        }

        void Acquire(void* resource) noexcept;

        void Release(void* resource) noexcept;

        void GC();

    private:
        struct Tag
        {
            // The reference count to a specific resource reaches 0 doesn't mean we can safely delete it becuase the GPU might be using it,
            // we give each resource a countdown value to determine when we can safely delete it when no one refers to it.
            uint16_t refCount = 1;
            uint16_t remainFrame = 0;
            std::function<void(void*)> deletor;
        };

        std::unordered_map<void*, Tag> m_Resources;
    };
}