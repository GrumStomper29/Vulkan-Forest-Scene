#pragma once

#include "mesh.hpp"

#include "volk/volk.h"

#include <vector>

namespace Graphics
{

	VkDescriptorPool createDescriptorPool(VkDevice device);

	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device);

	VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);

	void writeTextureSamplers(VkDevice device, VkDescriptorSet descriptorSet, const std::vector<Texture>& textures);

}