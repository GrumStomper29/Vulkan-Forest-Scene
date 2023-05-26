#pragma once

#include "volk/volk.h"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include <cstdint>

namespace Graphics
{

	class Window final
	{
	public:
		Window(std::uint32_t width, std::uint32_t height, const char* title);
		
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

		GLFWwindow* glfwWindow() const noexcept
		{
			return m_windowHandle;
		}

		VkExtent2D vkExtent() const noexcept
		{
			return m_extent;
		}

	private:
		GLFWwindow* m_windowHandle{};

		const VkExtent2D m_extent{};
	};

}