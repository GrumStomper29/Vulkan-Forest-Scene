#pragma once

#include "device.hpp"

#include "volk/volk.h"

#include <vector>

namespace Graphics
{

	class Pipeline
	{
	public:
		Pipeline(const Device& device, const char* vertexBinPath, const char* fragmentBinPath, VkExtent2D windowExtent);

		Pipeline(Pipeline&) = delete;
		Pipeline& operator=(Pipeline&) = delete;

		~Pipeline();

		VkPipeline vkPipeline() const
		{
			return m_pipeline;
		}

		VkPipelineLayout vkPipelineLayout() const
		{
			return m_layout;
		}

		static VkShaderModule loadShader(const char* path, VkDevice device);

	private:

		VkPipeline m_pipeline{};
		VkPipelineLayout m_layout{};

		VkViewport m_viewport{};

		const Device& m_device;
	};

}