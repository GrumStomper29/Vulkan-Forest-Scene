#pragma once

#include "volk/volk.h"
#include "glm/glm.hpp"

namespace Graphics
{

	struct PushConstants
	{
		glm::mat4 vertexTransform{};
	};

	VkShaderModule createShaderModule(VkDevice device, const char* path);

	VkPipelineLayout createPipelineLayout(VkDevice device);

	VkPipeline createPipeline(VkDevice device, VkPipelineLayout layout, VkExtent2D windowExtent, VkFormat colorAttachmentFormat);

}