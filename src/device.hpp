#pragma once

#include "instance.hpp"

#include "volk/volk.h"
#include "vma/vk_mem_alloc.h"

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

		VkPhysicalDevice vkPhysicalDevice() const noexcept
		{
			return m_physicalDevice;
		}

		VkDevice vkDevice() const noexcept
		{
			return m_device;
		}

		VmaAllocator vmaAllocator() const noexcept
		{
			return m_allocator;
		}

		std::uint32_t graphicsQueueFamily() const noexcept
		{
			return m_graphicsQueueFamily;
		}

		VkQueue graphicsQueue() const noexcept
		{
			return m_graphicsQueue;
		}

	private:
		VkPhysicalDevice m_physicalDevice{};

		VkDevice m_device{};

		VmaAllocator m_allocator{};

		std::uint32_t m_graphicsQueueFamily{};
		VkQueue m_graphicsQueue{};

		const Instance& m_instance;
	};

}