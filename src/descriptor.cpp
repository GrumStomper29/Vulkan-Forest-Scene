#include "descriptor.hpp"

#include "volk/volk.h"

namespace Graphics
{

	VkDescriptorPool createDescriptorPool(VkDevice device)
	{
		VkDescriptorPoolSize poolSizes[]
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1001 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolCI
		{
			.sType{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO },
			.maxSets{ 1 },
			.poolSizeCount{ 11 },
			.pPoolSizes{ poolSizes },
		};

		VkDescriptorPool descriptorPool{};
		vkCreateDescriptorPool(device, &poolCI, nullptr, &descriptorPool);

		return descriptorPool;
	}

	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
	{
		VkDescriptorBindingFlags bindingFlag{ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT };

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount{ 1 },
			.pBindingFlags{ &bindingFlag },
		};

		VkDescriptorSetLayoutBinding binding
		{
			.binding{ 0 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
			.descriptorCount{ 1001 },
			.stageFlags{ VK_SHADER_STAGE_FRAGMENT_BIT },
		};

		VkDescriptorSetLayoutCreateInfo layoutCI
		{
			.sType{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO },
			.pNext{ &bindingFlags },
			.bindingCount{ 1 },
			.pBindings{ &binding },
		};

		VkDescriptorSetLayout layout{};
		vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &layout);

		return layout;
	}

	VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		VkDescriptorSetAllocateInfo setAI
		{
			.sType{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO },
			.descriptorPool{ descriptorPool },
			.descriptorSetCount{ 1 },
			.pSetLayouts{ &descriptorSetLayout },
		};

		VkDescriptorSet set{};
		vkAllocateDescriptorSets(device, &setAI, &set);

		return set;
	}

}