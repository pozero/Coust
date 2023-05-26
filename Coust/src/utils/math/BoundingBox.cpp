#include "pch.h"

#include "utils/math/BoundingBox.h"

namespace coust {

BoundingBox::BoundingBox(
    glm::vec3 const &diag0, glm::vec3 const &diag1) noexcept
    : m_min(glm::min(diag0, diag1)), m_max(glm::max(diag0, diag1)) {
}

BoundingBox::BoundingBox(std::span<glm::vec3 const> points) noexcept {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};
    for (auto const &p : points) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
    m_min = min;
    m_max = max;
}

glm::vec3 BoundingBox::get_size() const noexcept {
    return glm::vec3{
        m_max[0] - m_min[0], m_max[1] - m_min[1], m_max[2] - m_min[2]};
}

glm::vec3 BoundingBox::get_center() const noexcept {
    return 0.5f * glm::vec3{m_max[0] + m_min[0], m_max[1] + m_min[1],
                      m_max[2] + m_min[2]};
}

BoundingBox BoundingBox::get_transformed(glm::mat4 const &mat) const noexcept {
    BoundingBox ret{*this};
    ret.transform(mat);
    return ret;
}

BoundingBox &BoundingBox::transform(glm::mat4 const &mat) noexcept {
    std::array box_vertices{
        glm::vec3{m_min.x, m_min.y, m_min.z},
        glm::vec3{m_max.x, m_min.y, m_min.z},
        glm::vec3{m_min.x, m_max.y, m_min.z},
        glm::vec3{m_min.x, m_min.y, m_max.z},
        glm::vec3{m_min.x, m_max.y, m_max.z},
        glm::vec3{m_max.x, m_min.y, m_max.z},
        glm::vec3{m_max.x, m_max.y, m_min.z},
        glm::vec3{m_max.x, m_max.y, m_max.z},
    };
    for (auto &v : box_vertices) {
        v = glm::vec3{
            mat * glm::vec4{v, 1.0f}
        };
    }
    *this = BoundingBox{box_vertices};
    return *this;
}

BoundingBox &BoundingBox::combine(glm::vec3 const &p) noexcept {
    m_min = glm::min(m_min, p);
    m_max = glm::max(m_max, p);
    return *this;
}

bool BoundingBox::operator==(BoundingBox const &other) const noexcept {
    return (m_min == other.m_min) && (m_max == other.m_max);
}

bool BoundingBox::operator!=(BoundingBox const &other) const noexcept {
    return !this->operator==(other);
}

}  // namespace coust
