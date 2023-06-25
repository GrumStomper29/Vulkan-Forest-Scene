#pragma once

#include "volk/volk.h"

namespace Graphics
{

	VkDescriptorPool createDescriptorPool(VkDevice device);

	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device);

	VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);

}