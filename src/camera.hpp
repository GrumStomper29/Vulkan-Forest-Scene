#pragma once

#include "glm/glm.hpp"

namespace Graphics
{

	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::vec3& position, const glm::vec2& rotation);

		glm::vec3& position()
		{
			return m_pos;
		}
		const glm::vec3& position() const
		{
			return m_pos;
		}

		glm::vec2& rotation()
		{
			return m_rot;
		}
		const glm::vec2& rotation() const
		{
			return m_rot;
		}

		const glm::vec3& front() const
		{
			return m_front;
		}
		const glm::vec3& right() const
		{
			return m_right;
		}
		const glm::vec3& up() const
		{
			return m_up;
		}

		Camera& move(float right, float up, float forward);
		Camera& rotate(float x, float y);

		glm::mat4 getViewMatrix();

	private:
		glm::vec3 m_pos{};
		glm::vec2 m_rot{};

		glm::vec3 m_front{};
		glm::vec3 m_right{};
		glm::vec3 m_up{};

		void calcVecs();
	};

}