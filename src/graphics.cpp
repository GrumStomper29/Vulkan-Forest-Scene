#include "graphics.hpp"

#include "window.hpp"

#define GLFW_INCLUDE_NONE
#include "glfw/glfw3.h"

#include <cstdlib>
#include <iostream>

namespace Graphics
{

	void init()
	{
		if (glfwInit() == GLFW_FALSE)
		{
			std::cerr << "error: failed to initialize GLFW.\n";
			std::abort();
		}
	}

	void terminate()
	{
		glfwTerminate();
	}

}