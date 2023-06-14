#pragma once

#include "volk/volk.h"

namespace Graphics
{

	VkShaderModule createShaderModule(VkDevice device, const char* path);

	VkPipelineLayout createPipelineLayout(VkDevice device);

	VkPipeline createPipeline(VkDevice device, VkPipelineLayout layout, VkExtent2D windowExtent, VkFormat colorAttachmentFormat);

}