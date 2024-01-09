#pragma once

#include "core/Memory.h"
#include "utils/math/BoundingBox.h"
#include "utils/allocators/StlContainer.h"
#include "render/vulkan/VulkanSampler.h"

#include <array>

namespace coust {
namespace render {

// the setup is tightly related to glTF format

// the location of vertex attributes are fixed, if its not provided,
// then use an invalid value to mark absence
// - POSITION       VEC3    float
// - NORMAL         VEC3    float
// - TANGENT        VEC4    float
// - COLOR_0        VEC4    float/ubyte/ushort
// - TEXCOORD_0     VEC2    float/ubyte/ushort
// - TEXCOORD_1     VEC2    float/ubyte/ushort
// - TEXCOORD_2     VEC2    float/ubyte/ushort
// - TEXCOORD_3     VEC2    float/ubyte/ushort
enum VertexAttrib : uint32_t {
    position = 0,
    normal,
    tangent,
    color_0,
    texcoord_0,
    texcoord_1,
    texcoord_2,
    texcoord_3,
};
uint32_t constexpr INVALID_VERTEX_ATTRIB = std::numeric_limits<uint32_t>::max();

std::pair<bool, VertexAttrib> to_vertex_attrib(
    std::string_view attribute_name) noexcept;

std::string_view to_string_view(VertexAttrib attrib) noexcept;

uint32_t constexpr MAX_LOD_COUNT = 8u;
// by supporting different streams in a single mesh data buffer, we can
// store different attributes seperately.
uint32_t constexpr MAX_VERTEX_ATTRIB_COUNT = 8;

struct Sampler {
    MagFilter mag;
    MinFilter min;
    VkSamplerAddressMode mode_u;
    VkSamplerAddressMode mode_v;
};

// Fixed format of RGBA with 32bit floating point number
struct Image {
    uint32_t width = 0;
    uint32_t height = 0;
    memory::vector<float, DefaultAlloc> data{get_default_alloc()};

    static VkFormat constexpr FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
};

struct Texture {
    uint32_t sampler = 0;
    uint32_t image = 0;
};

struct Material {
    std::array<float, 4> albedo_factor{};
    uint32_t albedo_texture = 0;
    uint32_t albedo_texcoord = 0;
};

struct MaterialAggregate {
    memory::vector<Sampler, DefaultAlloc> samplers{get_default_alloc()};
    memory::vector<Image, DefaultAlloc> images{get_default_alloc()};
    memory::vector<Texture, DefaultAlloc> textures{get_default_alloc()};
    memory::vector<Material, DefaultAlloc> materials{get_default_alloc()};
};

struct Mesh {
    struct Primitive {
        uint32_t index_offset = 0;
        uint32_t index_count = 0;

        uint32_t material_index = 0;

        //                  attrib_offset[1] == 1
        //                          |
        //  attrib0                 v                 attrib2
        // +--------------------+--------------------+-------------+
        // |  +  +  +  +  +  +  | * * * * * * * * * *|   -   -   - |
        // +--------------------+--------------------+-------------+
        //                ^       attrib1                ^
        //                |                              |
        //        attrib_offset[0] == 4         attrib_offset[2] == 0
        std::array<uint32_t, MAX_VERTEX_ATTRIB_COUNT> attrib_offset{
            INVALID_VERTEX_ATTRIB,
            INVALID_VERTEX_ATTRIB,
            INVALID_VERTEX_ATTRIB,
            INVALID_VERTEX_ATTRIB,
            INVALID_VERTEX_ATTRIB,
            INVALID_VERTEX_ATTRIB,
            INVALID_VERTEX_ATTRIB,
            INVALID_VERTEX_ATTRIB,
        };

        BoundingBox bounding_box;
    };

    memory::vector<Primitive, DefaultAlloc> primitives{get_default_alloc()};
};

struct Node {
    uint32_t mesh_idx;

    // this vector records indices of all local transformations from root
    // to this mesh
    memory::vector<uint32_t, DefaultAlloc> transformation_indices{
        get_default_alloc()};
};

uint32_t constexpr MAX_TEXTURE_PER_MESH_AGGREGATE = 256;

struct MeshAggregate {
    // we store all attributes in a big contiguous buffer, so here we need
    // to seperate them by recording their offset in **count of bytes**.
    // useful when updating descriptor sets (`VkDescriptorBufferInfo` in
    // `VkWriteDescriptorSet` struct consumed by `vkUpdateDescriptorSets`)
    // vertex_buffer:
    //    ------+-------------+-------------+-------------+-----------
    //          | attrib0 ... | attrib1 ... | attrib2 ... |   ...
    //    ------+-------------+-------------+-------------+-----------
    //          ^
    //          |
    // attrib_bytes_offset[0]
    std::array<size_t, MAX_VERTEX_ATTRIB_COUNT> attrib_bytes_offset{
        INVALID_VERTEX_ATTRIB,
        INVALID_VERTEX_ATTRIB,
        INVALID_VERTEX_ATTRIB,
        INVALID_VERTEX_ATTRIB,
        INVALID_VERTEX_ATTRIB,
        INVALID_VERTEX_ATTRIB,
        INVALID_VERTEX_ATTRIB,
        INVALID_VERTEX_ATTRIB,
    };

    uint8_t valid_attrib_mask = 0;

    memory::vector<uint32_t, DefaultAlloc> index_buffer{get_default_alloc()};
    memory::vector<float, DefaultAlloc> vertex_buffer{get_default_alloc()};

    memory::vector<Mesh, DefaultAlloc> meshes{get_default_alloc()};

    memory::vector<Node, DefaultAlloc> nodes{get_default_alloc()};
    memory::vector<glm::mat4, DefaultAlloc> transformations{
        get_default_alloc()};

    MaterialAggregate material_aggregate{};

public:
    static constexpr void serialize(
        MeshAggregate& self, auto& archive) noexcept {
        archive(self.attrib_bytes_offset, self.valid_attrib_mask,
            self.index_buffer, self.vertex_buffer, self.meshes, self.nodes);
        size_t transformation_cnt = self.transformations.size();
        archive(transformation_cnt);
        self.transformations.resize(transformation_cnt);
        for (size_t i = 0; i < transformation_cnt; ++i) {
            auto& t = self.transformations[i];
            archive(t[0][0], t[0][1], t[0][2], t[0][3], t[1][0], t[1][1],
                t[1][2], t[1][3], t[2][0], t[2][1], t[2][2], t[2][3], t[3][0],
                t[3][1], t[3][2], t[3][3]);
        }
        archive(self.material_aggregate);
    }

    static size_t get_primitve_count(MeshAggregate const& ma);

    static size_t get_transformation_index_count(MeshAggregate const& ma);
};

}  // namespace render
}  // namespace coust
