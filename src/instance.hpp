#pragma once

#include "volk/volk.h"

namespace Graphics
{

	class Instance
	{
	public:
		Instance();
		
		Instance(Instance&) = delete;
		Instance& operator=(Instance&) = delete;

		~Instance()
		{
			vkDestroyInstance(m_instance, nullptr);
		}

	private:
		VkInstance m_instance{};
	};

}