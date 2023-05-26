#pragma once

#include "render/Mesh.h"

#include <filesystem>

namespace coust {
namespace render {

MeshAggregate process_gltf(std::filesystem::path path) noexcept;

}  // namespace render
}  // namespace coust
