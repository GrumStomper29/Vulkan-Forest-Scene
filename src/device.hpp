#pragma once

#include "instance.hpp"

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{

	class Device
	{
	public:
		Device(const Instance& instance);

		Device(Device&) = delete;
		Device& operator=(Device&) = delete;

		~Device();

	private:
		VkPhysicalDevice m_physicalDevice{};

		VkDevice m_device{};

		std::uint32_t m_graphicsQueueFamily{};
		VkQueue m_graphicsQueue{};
	};

}