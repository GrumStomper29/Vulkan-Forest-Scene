#pragma once

#include "volk/volk.h"

namespace Graphics
{

	class Instance
	{
	public:
		Instance(bool enabledValidation);
		
		Instance(Instance&) = delete;
		Instance& operator=(Instance&) = delete;

		~Instance()
		{
			vkDestroyInstance(m_instance, nullptr);
		}

		VkInstance vkInstance() const noexcept
		{
			return m_instance;
		}

	private:
		VkInstance m_instance{};
	};

}