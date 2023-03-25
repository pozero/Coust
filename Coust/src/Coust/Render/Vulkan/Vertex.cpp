#include "pch.h"

#include "Coust/Render/Vulkan/Vertex.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK 
{
    bool VertexBuffer::LocationBundle::operator==(const LocationBundle& other) const noexcept
    {
        for (uint32_t i = 0; i < 4; ++ i)
        {
            if (!components[i].empty() && !other.components[i].empty())
            {
                if (components[i] != other.components[i])
                    return false;
            }
            else if (components[i].empty() && other.components[i].empty())
                continue;
            else
                return false;
        }
        return true;
    }

    size_t VertexBuffer::LocationBundle::GetHash() const noexcept
    {
        size_t h = 0;
        for (uint32_t i = 0; i < 4; ++ i)
        {
            Hash::Combine(h, components[i]);
        }
        return h;
    }

    void VertexBuffer::Add(std::string_view component[4], Buffer* buffer)
    {
        LocationBundle lb{};
        for (uint32_t i = 0; i < 4; ++ i)
        {
            lb.components[i] = component[i];
        }
        m_VertexDataPerLocation[lb] = buffer;
    }

    bool VertexBuffer::Get(const std::vector<ShaderResource>& resources, std::vector<VkBuffer>& out_Handles) const
    {
        out_Handles.resize(m_VertexDataPerLocation.size());
        for (const auto& p : m_VertexDataPerLocation)
        {
            uint32_t location[4] { INVALID_IDX };
            uint32_t offset[4] { 0 };
            const std::string_view* name = &p.first.components[0];
            uint32_t componentCount = 0;
            for (; componentCount < 4 && !name[componentCount].empty(); ++ componentCount)
            {
                auto iter = std::find_if(resources.begin(), resources.end(),
                    [name, componentCount](const ShaderResource& res) -> bool
                    {
                        const bool isInput = res.Type == ShaderResourceType::Input;
                        const bool isVertex = res.Stage == VK_SHADER_STAGE_VERTEX_BIT;
                        return name[componentCount] == res.Name && isInput && isVertex;
                    });
                if (iter == resources.end())
                {
                    COUST_CORE_ERROR("Can't find vertex input variable of name {}", name[componentCount]);
                    return false;
                }
                location[componentCount] = iter->Location;
                offset[componentCount] = iter->Offset;
            }

            bool atSameLocation = true;
            bool inCorrectOrder = true;
            for (uint32_t i = 1; i < componentCount; ++ i)
            {
                atSameLocation = atSameLocation && location[i - 1] == location[i];
                inCorrectOrder = inCorrectOrder && offset[i - 1] < offset[i];
            }
            if (atSameLocation && inCorrectOrder)
                out_Handles[location[0]] = p.second->GetHandle();
            else
            {
                COUST_CORE_ERROR("The input data layout doesn't match the shader. The layout is:\n\tLocation: {}, {}, {}, {}\n\tOffset: {}, {}, {}, {}", 
                    location[0], location[1], location[2], location[3], offset[0], offset[1], offset[2], offset[3]);
                return false;
            }
        }
        return true;
    }

    IndexBuffer::IndexBuffer(VkIndexType indexType, Buffer* buffer) noexcept
        : m_Buffer(buffer), m_IndexType(indexType)
    {}

    std::tuple<VkBuffer, VkIndexType> IndexBuffer::Get() const
    {
        return { m_Buffer->GetHandle(), m_IndexType };
    }

    void RenderPrimitive::Set(VertexBuffer* vertexBuffer, IndexBuffer *indexBuffer) noexcept
    {
        m_VertexBuf = vertexBuffer;
        m_IndexBuf = indexBuffer;
    }

    std::tuple<VertexBuffer*, IndexBuffer*> RenderPrimitive::Get() const noexcept
    {
        return { m_VertexBuf, m_IndexBuf };
    }
}