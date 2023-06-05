#include "pipeline.hpp"

#include "device.hpp"

#include "volk/volk.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace Graphics
{

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module);
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo();
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo();
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo(const VkViewport& viewport, const VkRect2D& scissor);
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(VkCullModeFlags cullMode, VkFrontFace frontFace);
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo();
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo(const std::vector<VkPipelineColorBlendAttachmentState>& attachments);

	Pipeline::Pipeline(const Device& device, const char* vertexBinPath, const char* fragmentBinPath, VkExtent2D windowExtent)
		: m_device{ device }
		, m_viewport{ 0.0f, 0.0f, static_cast<float>(windowExtent.width), static_cast<float>(windowExtent.height), 0.0f, 1.0f }
	{
		auto vertexShaderModule{ loadShader(vertexBinPath, m_device.vkDevice()) };
		auto fragmentShaderModule{ loadShader(fragmentBinPath, m_device.vkDevice()) };

		VkPipelineShaderStageCreateInfo stages[2]{
			shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule),
			shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule)
		};

		auto vertexInputState{ vertexInputStateCreateInfo() };

		auto inputAssemblyState{ inputAssemblyStateCreateInfo() };

		VkRect2D scissor{ .offset{ 0, 0 }, .extent{ windowExtent } };
		auto viewportState{ viewportStateCreateInfo(m_viewport, scissor) };

		auto rasterizationState{ rasterizationStateCreateInfo(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE) };

		auto multisampleState{ multisampleStateCreateInfo() };

		VkPipelineColorBlendAttachmentState attachment;
		attachment.blendEnable = VK_FALSE;
		attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		auto colorBlendState{ colorBlendStateCreateInfo({ attachment }) };

		VkPipelineLayoutCreateInfo layoutInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO } };
		layoutInfo.setLayoutCount = 0;
		layoutInfo.pushConstantRangeCount = 0;

		vkCreatePipelineLayout(m_device.vkDevice(), &layoutInfo, nullptr, &m_layout);

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{ .sType{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO } };
		graphicsPipelineCreateInfo.stageCount = 2;
		graphicsPipelineCreateInfo.pStages = &stages[0];
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputState;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		graphicsPipelineCreateInfo.pViewportState = &viewportState;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizationState;
		graphicsPipelineCreateInfo.pMultisampleState = &multisampleState;
		graphicsPipelineCreateInfo.pColorBlendState = &colorBlendState;
		graphicsPipelineCreateInfo.layout = m_layout;

		vkCreateGraphicsPipelines(m_device.vkDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_pipeline);

		vkDestroyShaderModule(m_device.vkDevice(), vertexShaderModule, nullptr);
		vkDestroyShaderModule(m_device.vkDevice(), fragmentShaderModule, nullptr);
	}

	Pipeline::~Pipeline()
	{
		vkDestroyPipeline(m_device.vkDevice(), m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_device.vkDevice(), m_layout, nullptr);
	}

	VkShaderModule Pipeline::loadShader(const char* path, VkDevice device)
	{
		// Start the read at the end to get the size of the file
		std::ifstream shaderStream{ path, std::ios::ate | std::ios::binary };
		if (!shaderStream)
		{
			throw std::runtime_error{ "error: SPIR-V file could not be opened at path: " + std::string{ path } };
		}

		std::vector<char> shaderCode(shaderStream.tellg());

		shaderStream.seekg(0);

		shaderStream.read(shaderCode.data(), shaderCode.size());

		VkShaderModuleCreateInfo shaderInfo{ .sType{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO } };
		shaderInfo.codeSize = shaderCode.size();
		// Vulkan reads binary as uint32_t yet I have it stored as char
		shaderInfo.pCode = reinterpret_cast<const std::uint32_t*>(shaderCode.data());

		VkShaderModule shaderModule{};
		vkCreateShaderModule(device, &shaderInfo, nullptr, &shaderModule);
		return shaderModule;
	}

	// =========================================================================================
	// ======================================= UTILITIES =======================================
	// =========================================================================================

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module)
	{
		VkPipelineShaderStageCreateInfo stageInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };
		stageInfo.stage = stage;
		stageInfo.module = module;
		stageInfo.pName = "main";
		
		return stageInfo;
	}

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo()
	{
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO } };
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

		return vertexInputInfo;
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO } };
		inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		return inputAssemblyStateInfo;
	}

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo(const VkViewport& viewport, const VkRect2D& scissor)
	{
		VkPipelineViewportStateCreateInfo viewportStateInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO } };
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.pViewports = &viewport;
		viewportStateInfo.scissorCount = 1;
		viewportStateInfo.pScissors = &scissor;

		return viewportStateInfo;
	}

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(VkCullModeFlags cullMode, VkFrontFace frontFace)
	{
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO } };
		rasterizationInfo.depthClampEnable = VK_FALSE;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.cullMode = cullMode;
		rasterizationInfo.frontFace = frontFace;
		rasterizationInfo.depthBiasEnable = VK_FALSE;
		rasterizationInfo.lineWidth = 1.0f;

		return rasterizationInfo;
	}

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo()
	{
		VkPipelineMultisampleStateCreateInfo multisampleInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO } };
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;
		
		return multisampleInfo;
	}

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo(const std::vector<VkPipelineColorBlendAttachmentState>& attachments)
	{
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{ .sType{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO } };
		colorBlendInfo.logicOpEnable = VK_TRUE;
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Experiment with this
		colorBlendInfo.attachmentCount = attachments.size();
		colorBlendInfo.pAttachments = attachments.data();

		return colorBlendInfo;
	}

}