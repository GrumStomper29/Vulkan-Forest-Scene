#include "camera.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cmath>

namespace Graphics
{

	Camera::Camera(const glm::vec3& position, const glm::vec2& rotation)
		: m_pos{ position },
		  m_rot{ rotation }
	{
		calcVecs();
	}

	Camera& Camera::move(float right, float up, float forward)
	{
		m_pos += m_right * right;
		m_pos += m_up * up;
		m_pos += m_front * forward;

		return *this;
	}

	Camera& Camera::rotate(float x, float y)
	{
		m_rot.x += x;
		m_rot.y += y;

		return *this;
	}

	glm::mat4 Camera::getViewMatrix()
	{
		calcVecs();
		return glm::lookAt(m_pos, m_pos - m_front, m_up);
	}

	void Camera::calcVecs()
	{
		m_front.x = std::cos(glm::radians(m_rot.y)) * std::cos(glm::radians(m_rot.x));
		m_front.y = std::sin(glm::radians(m_rot.x));
		m_front.z = std::sin(glm::radians(m_rot.y)) * std::cos(glm::radians(m_rot.x));
		m_front = glm::normalize(m_front);
		m_right = glm::normalize(glm::cross({ 0.0f, 1.0f, 0.0f }, m_front));
		m_up = glm::cross(m_front, m_right);
	}

}