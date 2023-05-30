#include "pch.h"

#include "test/Test.h"

#include "utils/filesystem/FileIO.h"
#include "utils/filesystem/NaiveSerialization.h"
#include "render/asset/MeshConvertion.h"

#include "glm/ext/matrix_transform.hpp"

TEST_CASE("[Coust] [render] [asset] MeshConvertion" * doctest::skip(true)) {
    using namespace coust;
    SUBCASE("Minimal glTF file") {
        std::filesystem::path test_asset_path = file::get_absolute_path_from(
            "Coust", "asset", "test", "minimal.gltf");
        render::MeshAggregate ma = render::process_gltf(test_asset_path);
        CHECK(ma.attrib_bytes_offset[0] == 0);
        CHECK(ma.attrib_bytes_offset[1] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[2] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[3] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[4] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[5] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[6] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[7] == render::INVALID_VERTEX_ATTRIB);
        REQUIRE(ma.index_buffer.size() == 3);
        REQUIRE(ma.vertex_buffer.size() == 9);
        REQUIRE(ma.meshes.size() == 1);
        REQUIRE(ma.meshes[0].primitives.size() == 1);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[0] == 0);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[1] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[2] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[3] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[4] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[5] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[6] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[7] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].bounding_box.m_min ==
              glm::vec3{0.0f, 0.0f, 0.0f});
        CHECK(ma.meshes[0].primitives[0].bounding_box.m_max ==
              glm::vec3{1.0f, 1.0f, 0.0f});
        REQUIRE(ma.transformations.size() == 0);
        REQUIRE(ma.nodes.size() == 1);
        CHECK(ma.nodes[0].mesh_idx == 0);
        CHECK(ma.nodes[0].transformation_indices.size() == 0);

        file::ByteArray byte_array = file::to_byte_array(ma);
        render::MeshAggregate from_bytes =
            file::from_byte_array<render::MeshAggregate>(byte_array);
        CHECK(std::ranges::equal(
            ma.attrib_bytes_offset, from_bytes.attrib_bytes_offset));
        CHECK(std::ranges::equal(ma.index_buffer, from_bytes.index_buffer));
        CHECK(
            std::ranges::equal(ma.transformations, from_bytes.transformations));
    }

    SUBCASE("Simple glTF meshes") {
        std::filesystem::path test_asset_path = file::get_absolute_path_from(
            "Coust", "asset", "test", "simple_meshes.gltf");
        render::MeshAggregate ma = render::process_gltf(test_asset_path);
        CHECK(ma.attrib_bytes_offset[0] == 0);
        CHECK(ma.attrib_bytes_offset[1] == 36);
        CHECK(ma.attrib_bytes_offset[2] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[3] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[4] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[5] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[6] == render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.attrib_bytes_offset[7] == render::INVALID_VERTEX_ATTRIB);
        REQUIRE(ma.index_buffer.size() == 3);
        CHECK(ma.index_buffer[0] == 0);
        CHECK(ma.index_buffer[1] == 1);
        CHECK(ma.index_buffer[2] == 2);
        REQUIRE(ma.vertex_buffer.size() == 18);
        REQUIRE(ma.meshes.size() == 1);
        REQUIRE(ma.meshes[0].primitives.size() == 1);
        CHECK(ma.meshes[0].primitives[0].index_start_idx == 0);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[0] == 0);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[1] == 9);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[2] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[3] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[4] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[5] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[6] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].attrib_start_idx[7] ==
              render::INVALID_VERTEX_ATTRIB);
        CHECK(ma.meshes[0].primitives[0].bounding_box.m_min ==
              glm::vec3{0.0f, 0.0f, 0.0f});
        CHECK(ma.meshes[0].primitives[0].bounding_box.m_max ==
              glm::vec3{1.0f, 1.0f, 0.0f});
        REQUIRE(ma.nodes.size() == 2);
        CHECK(ma.nodes[0].mesh_idx == 0);
        CHECK(ma.nodes[1].mesh_idx == 0);
        REQUIRE(ma.nodes[0].transformation_indices.size() == 0);
        REQUIRE(ma.nodes[1].transformation_indices.size() == 1);
        REQUIRE(ma.nodes[1].transformation_indices[0] == 0);
        REQUIRE(ma.transformations.size() == 1);
        glm::mat4 translation{1.0f};
        translation = glm::translate(translation, glm::vec3{1.0f, 0.0f, 0.0f});
        CHECK(ma.transformations[0] == translation);

        file::ByteArray byte_array = file::to_byte_array(ma);
        render::MeshAggregate from_bytes =
            file::from_byte_array<render::MeshAggregate>(byte_array);
        CHECK(std::ranges::equal(
            ma.attrib_bytes_offset, from_bytes.attrib_bytes_offset));
        CHECK(std::ranges::equal(ma.index_buffer, from_bytes.index_buffer));
        CHECK(
            std::ranges::equal(ma.transformations, from_bytes.transformations));
    }
}
