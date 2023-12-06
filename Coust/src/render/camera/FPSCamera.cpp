#include "pch.h"

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "glm/gtc/matrix_transform.hpp"
WARNING_POP

#include "core/Application.h"
#include "render/camera/FPSCamera.h"

namespace coust {
namespace render {

FPSCamera::FPSCamera(glm::vec3 const& position, glm::vec3 const& look_at,
    glm::vec3 const& world_up, float fov, float speed,
    float sensitivity) noexcept
    : m_fov(fov),
      m_speed(speed),
      m_sensitivity(sensitivity),
      m_pos(position),
      m_world_up(world_up),
      m_front(glm::normalize(look_at - position)) {
    update_direction();
}

void FPSCamera::move_forward(TimeStep ts) noexcept {
    float const delta = ts.to_second() * m_speed;
    m_pos += delta * m_front;
}

void FPSCamera::move_backward(TimeStep ts) noexcept {
    float const delta = ts.to_second() * m_speed;
    m_pos -= delta * m_front;
}

void FPSCamera::move_left(TimeStep ts) noexcept {
    float const delta = ts.to_second() * m_speed;
    m_pos -= delta * m_right;
}

void FPSCamera::move_right(TimeStep ts) noexcept {
    float const delta = ts.to_second() * m_speed;
    m_pos += delta * m_right;
}

void FPSCamera::rotate(float cursor_x, float cursor_y) noexcept {
    static float last_x = cursor_x;
    static float last_y = cursor_y;
    if (m_holding) {
        float const offset_x = last_x - cursor_x;
        float const offset_y = last_y - cursor_y;
        m_yaw += offset_x * m_sensitivity;
        m_pitch = glm::clamp(m_pitch + offset_y * m_sensitivity, -89.0f, 89.0f);
        update_direction();
    }
    last_x = cursor_x;
    last_y = cursor_y;
}

void FPSCamera::set_holding(bool holded) noexcept {
    m_holding = holded;
}

void FPSCamera::zoom(float wheel_y) noexcept {
    m_fov = glm::clamp(m_fov - wheel_y, 1.0f, 45.0f);
}

glm::mat4 FPSCamera::get_view_matrix() const noexcept {
    return glm::lookAt(m_pos, m_pos + m_front, m_up);
}

glm::mat4 FPSCamera::get_proj_matrix() const noexcept {
    auto const [width, height] =
        Application::get_instance().get_window().get_size();
    return glm::perspectiveZO(
        glm::radians(m_fov), float(width) / float(height), 0.1f, 100.0f);
}

void FPSCamera::update_direction() noexcept {
    glm::vec3 unnormalized_front{};
    unnormalized_front.x =
        cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    unnormalized_front.y = sin(glm::radians(m_pitch));
    unnormalized_front.z =
        sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(unnormalized_front);
    m_right = glm::normalize(glm::cross(m_front, m_world_up));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

}  // namespace render
}  // namespace coust
