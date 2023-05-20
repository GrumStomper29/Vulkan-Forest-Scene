#include "window.hpp"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include <stdexcept>

namespace Graphics
{

	Window::Window(int width, int height, const char* title)
		: m_windowHandle{ glfwCreateWindow(width, height, title, nullptr, nullptr)}
	{
		if (!m_windowHandle)
		{
			throw std::runtime_error{ "error: GLFW failed to create window.\n" };
		}
	}

}