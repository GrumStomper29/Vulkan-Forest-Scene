#include "window.hpp"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include <cstdint>
#include <stdexcept>

namespace Graphics
{

	Window::Window(std::uint32_t width, std::uint32_t height, const char* title)
		: m_extent{ width, height }
	{
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_windowHandle = glfwCreateWindow(width, height, title, glfwGetPrimaryMonitor(), nullptr);

		if (!m_windowHandle)
		{
			throw std::runtime_error{ "error: GLFW failed to create window.\n" };
		}
	}

}