#pragma once

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

namespace Graphics
{

	class Window final
	{
	public:
		Window(int width, int height, const char* title);
		~Window();

		bool shouldClose()
		{
			return glfwWindowShouldClose(m_windowHandle);
		}

		void pollEvents()
		{
			glfwPollEvents();
		}

	private:
		GLFWwindow* m_windowHandle{};
	};

}