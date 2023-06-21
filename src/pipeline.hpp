#pragma once

#include "volk/volk.h"

#include <cstdint>

namespace Graphics
{

	VkShaderModule createShaderModule(VkDevice device, const char* path);

	VkPipelineLayout createPipelineLayout(VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout* descriptorSetLayouts);

	VkPipeline createPipeline(VkDevice device, VkPipelineLayout layout, VkExtent2D windowExtent, VkFormat colorAttachmentFormat);

}