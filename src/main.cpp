#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include <cstdlib>
#include <iostream>

int main()
{
	if (!glfwInit())
	{
		std::cerr << "error: failed to initialize GLFW.\n";
		std::abort();
	}

	GLFWwindow* window{ glfwCreateWindow(640, 480, "Vulkan Forest Scene", nullptr, nullptr) };

	if (!window)
	{
		std::cerr << "error: failed to create window.\n";
		std::abort();
	}

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}