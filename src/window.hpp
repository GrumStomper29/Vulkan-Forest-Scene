#pragma once

#include "volk/volk.h"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

namespace Graphics
{

	class Window final
	{
	public:
		Window(int width, int height, const char* title);
		
		Window(Window&) = delete;
		Window& operator=(Window&) = delete;

		~Window()
		{
			glfwDestroyWindow(m_windowHandle);
		}

		bool shouldClose()
		{
			return glfwWindowShouldClose(m_windowHandle);
		}

		void pollEvents()
		{
			glfwPollEvents();
		}

		void swapBuffers()
		{
			glfwSwapBuffers(m_windowHandle);
		}

		GLFWwindow* getGLFWwindowHandle() const
		{
			return m_windowHandle;
		}

		VkExtent2D getExtent() const
		{
			return m_extent;
		}

	private:
		GLFWwindow* m_windowHandle{};

		const VkExtent2D m_extent{};
	};

}