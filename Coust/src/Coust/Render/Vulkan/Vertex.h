#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"
#include "Coust/Render/Vulkan/VulkanShader.h"

#include "Coust/Utils/Hash.h"

#include <string>
#include <unordered_map>

namespace Coust::Render::VK 
{
    class VertexBuffer;
    class IndexBuffer;
    class RenderPrimitive;

    class VertexBuffer
    {
    public:
        VertexBuffer(VertexBuffer&&) = delete;
        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer&operator=(VertexBuffer&&) = delete;
        VertexBuffer&operator=(const VertexBuffer&) = delete;
    
    public:
        // Each location has at most 4 component, and we assume there're at most 4 byte of data in each location, which means our vertex shader won't skip location index
        struct LocationBundle
        {
            std::string_view components[4]{};

            bool operator==(const LocationBundle& other) const noexcept;

            size_t GetHash() const noexcept;
        };

        VertexBuffer() noexcept = default;
        ~VertexBuffer() noexcept = default;

        void Add(std::string_view component[4], Buffer* buffer) noexcept;

        bool Get(const std::vector<ShaderResource>& resources, std::vector<VkBuffer>& out_Handles) const noexcept;

    private:
        std::unordered_map<LocationBundle, Buffer*, Hash::HashFn<LocationBundle>, Hash::EqualFn<LocationBundle>> m_VertexDataPerLocation;
    };

    class IndexBuffer 
    {
    public:
        IndexBuffer() = delete;
        IndexBuffer(IndexBuffer&&) = delete;
        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(IndexBuffer&&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;

    public:
        explicit IndexBuffer(VkIndexType indexType, Buffer* buffer) noexcept;

        std::pair<VkBuffer, VkIndexType> Get() const noexcept;
    
    private:
        Buffer* m_Buffer;
        VkIndexType m_IndexType;
    };

    class RenderPrimitive
    {
    public:
        RenderPrimitive(RenderPrimitive&&) = delete;
        RenderPrimitive(const RenderPrimitive&) = delete;
        RenderPrimitive& operator=(RenderPrimitive&&) = delete;
        RenderPrimitive& operator=(const RenderPrimitive&) = delete;
    
    public:
        RenderPrimitive() noexcept = default;

        void Set(VertexBuffer* vertexBuffer, IndexBuffer *indexBuffer) noexcept;

        std::pair<VertexBuffer*, IndexBuffer*> Get() const noexcept;
    
    private:
        VertexBuffer* m_VertexBuf;
        IndexBuffer* m_IndexBuf;
    };
}