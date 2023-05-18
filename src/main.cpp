#include "graphics.hpp"
#include "window.hpp"

int main()
{
	Graphics::init();

	Graphics::Window window{ 800, 450, "Vulkan-Forest-Scene" };

	while (!window.shouldClose())
	{
		window.pollEvents();
	}

	Graphics::terminate();

	return 0;
}