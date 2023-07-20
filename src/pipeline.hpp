#pragma once

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{
	struct GraphicsPipelineCreateInfo
	{
		VkDevice device{};

		std::uint32_t colorAttachmentCount{};
		const VkFormat* pColorAttachmentFormats{};
		VkFormat depthFormat{};

		const char* pVertexShaderPath{};
		const char* pFragmentShaderPath{};

		VkExtent2D viewportExtent{};

		VkSampleCountFlagBits sampleCount{};

		VkPipelineLayout pipelineLayout{};

		bool depthTestEnable{ true };
	};

	VkShaderModule createShaderModule(VkDevice device, const char* path);

	VkPipelineLayout createPipelineLayout(VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout* descriptorSetLayouts);

	VkPipeline createGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);

}