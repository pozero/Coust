#pragma once

#include "glm/mat4x4.hpp"

#include "utils/TimeStep.h"

namespace coust {
namespace render {

class FPSCamera {
public:
    FPSCamera() = delete;
    FPSCamera(FPSCamera&&) = delete;
    FPSCamera(FPSCamera const&) = delete;
    FPSCamera& operator=(FPSCamera&&) = delete;
    FPSCamera& operator=(FPSCamera const&) = delete;

public:
    FPSCamera(glm::vec3 const& position, glm::vec3 const& look_at,
        glm::vec3 const& world_up, float fov = 45.0f, float speed = 10.0f,
        float sensitivity = 0.1f) noexcept;

    void move_forward(TimeStep ts) noexcept;

    void move_backward(TimeStep ts) noexcept;

    void move_left(TimeStep ts) noexcept;

    void move_right(TimeStep ts) noexcept;

    void rotate(float cursor_x, float cursor_y) noexcept;

    void set_holding(bool holded) noexcept;

    void zoom(float wheel_y) noexcept;

    glm::mat4 get_view_matrix() const noexcept;

    glm::mat4 get_proj_matrix() const noexcept;

private:
    void update_direction() noexcept;

private:
    float m_fov;
    float m_speed;
    float m_sensitivity;
    float m_pitch = 0;
    float m_yaw = -90.0f;

    glm::vec3 m_pos;
    glm::vec3 m_world_up;
    glm::vec3 m_up;
    glm::vec3 m_front;
    glm::vec3 m_right;

    bool m_holding = false;
};

}  // namespace render
}  // namespace coust
