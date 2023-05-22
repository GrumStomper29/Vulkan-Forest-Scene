#include "graphics.hpp"

#include "window.hpp"
#include "instance.hpp"
#include "device.hpp"
#include "swapchain.hpp"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include "volk/volk.h"

#include <cstdlib>
#include <iostream>

namespace Graphics
{

	// Note: The vast majority of initialization is done in the contructors of the objects created in Graphics::run().
	void init()
	{
		if (glfwInit() == GLFW_FALSE)
		{
			std::cerr << "error: failed to initialize GLFW.\n";
			std::abort();
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (volkInitialize() != VK_SUCCESS)
		{
			std::cerr << "error: failed to load Vulkan loader.\n";
			std::abort();
		}
	}

	void run()
	{
		Window window{ 1920 / 2, 1080 / 2, "Vulkan-Forest-Scene" };
		Instance instance{};
		Device device{ instance };
		Swapchain swapchain{ instance, window };

		while (!window.shouldClose())
		{
			window.pollEvents();
			window.swapBuffers();
		}
	}

	void cleanup()
	{
		glfwTerminate();
	}

}