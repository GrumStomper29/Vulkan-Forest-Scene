#include "graphics.hpp"

#include <iostream>

int main()
{
	Graphics::init();

	Graphics::run();

	Graphics::cleanup();

	std::cin.get();
	return 0;
}