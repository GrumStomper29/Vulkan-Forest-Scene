#include "descriptor.hpp"

#include "mesh.hpp"

#include "volk/volk.h"

#include <vector>

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
		/*
		VkDescriptorSetLayoutBinding skyboxBinding
		{
			.binding{ 0 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
			.descriptorCount{ 1 },
			.stageFlags{ VK_SHADER_STAGE_FRAGMENT_BIT },
		};
		*/
		VkDescriptorBindingFlags bindingFlags[2]
		{
			0,
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT 
		};

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI
		{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount{ 2 },
			.pBindingFlags{ bindingFlags },
		};

		VkDescriptorSetLayoutBinding bindings[2]
		{
			{
				.binding{ 0 },
				.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
				.descriptorCount{ 1 },
				.stageFlags{ VK_SHADER_STAGE_FRAGMENT_BIT },
			},

			{
			.binding{ 1 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
			.descriptorCount{ 1001 },
			.stageFlags{ VK_SHADER_STAGE_FRAGMENT_BIT },
			},
		};
		/*
		VkDescriptorSetLayoutBinding binding
		{
			.binding{ 1 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
			.descriptorCount{ 1001 },
			.stageFlags{ VK_SHADER_STAGE_FRAGMENT_BIT },
		};
		*/
		VkDescriptorSetLayoutCreateInfo layoutCI
		{
			.sType{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO },
			.pNext{ &bindingFlagsCI },
			.bindingCount{ 2 },
			.pBindings{ bindings },
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

	void writeTextureSamplers(VkDevice device, VkDescriptorSet descriptorSet, const std::vector<Texture>& textures)
	{
		std::vector<VkDescriptorImageInfo> imageInfos(textures.size());
		std::vector<VkWriteDescriptorSet> writes(textures.size());
		for (int i{ 0 }; i < textures.size(); ++i)
		{
			imageInfos[i] =
			{
				.sampler{ textures[i].vkSampler() },
				.imageView{ textures[i].vkImageView() },
				.imageLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			};
			writes[i] =
			{
				.sType{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
				.dstSet{ descriptorSet },
				.dstBinding{ 1 },
				.dstArrayElement{ static_cast<std::uint32_t>(i) },
				.descriptorCount{ 1 },
				.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
				.pImageInfo{ &imageInfos[i] },
			};
		}
		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}


	void writeSkyboxSampler(VkDevice device, VkDescriptorSet descriptorSet, VkImageView skyboxView, VkSampler skyboxSampler)
	{
		VkDescriptorImageInfo imgInfo
		{
			.sampler{ skyboxSampler },
			.imageView{ skyboxView },
			.imageLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		};
		VkWriteDescriptorSet write
		{
			.sType{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
			.dstSet{ descriptorSet },
			.dstBinding{ 0 },
			.descriptorCount{ 1 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
			.pImageInfo{ &imgInfo },
		};
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}
}