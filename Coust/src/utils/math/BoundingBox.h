#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "glm/glm.hpp"
WARNING_POP

namespace coust {

class BoundingBox {
public:
    BoundingBox() noexcept = default;
    BoundingBox(BoundingBox &&) noexcept = default;
    BoundingBox(BoundingBox const &) noexcept = default;
    BoundingBox &operator=(BoundingBox &&) noexcept = default;
    BoundingBox &operator=(BoundingBox const &) noexcept = default;

public:
    static constexpr void serialize(BoundingBox &box, auto &archive) noexcept {
        archive(box.m_min.x, box.m_min.y, box.m_min.z, box.m_max.x, box.m_max.y,
            box.m_max.z);
    }

public:
    BoundingBox(glm::vec3 const &diag0, glm::vec3 const &diag1) noexcept;

    BoundingBox(std::span<glm::vec3 const> points) noexcept;

    glm::vec3 get_size() const noexcept;

    glm::vec3 get_center() const noexcept;

    BoundingBox get_transformed(glm::mat4 const &mat) const noexcept;

    BoundingBox &transform(glm::mat4 const &mat) noexcept;

    BoundingBox &combine(glm::vec3 const &p) noexcept;

    bool operator==(BoundingBox const &other) const noexcept;

    bool operator!=(BoundingBox const &other) const noexcept;

public:
    glm::vec3 m_min{std::numeric_limits<float>::max()};
    glm::vec3 m_max{std::numeric_limits<float>::lowest()};
};

}  // namespace coust
