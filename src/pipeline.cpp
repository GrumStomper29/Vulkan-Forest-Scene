#include "pipeline.hpp"

#include "frame.hpp"
#include "mesh.hpp"

#include "volk/volk.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <vector>

namespace Graphics
{

	VkShaderModule createShaderModule(VkDevice device, const char* path)
	{
		// Start at end of the file to get code size
		std::ifstream stream{ path, std::ios::binary | std::ios::ate };

		if (!stream.is_open())
		{
			std::string error{ "failed to open SPIR-V file at path: " };
			error += path;
			throw std::exception{ error.c_str() };
		}

		std::size_t codeSize{ static_cast<std::size_t>(stream.tellg()) };
		std::vector<std::uint32_t> code(codeSize / sizeof(std::uint32_t));

		stream.seekg(0);
		stream.read(reinterpret_cast<char*>(code.data()), codeSize);

		stream.close();

		VkShaderModuleCreateInfo moduleCI
		{
			.sType{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO },
			.codeSize{ codeSize },
			.pCode{ code.data() },
		};

		VkShaderModule module{};
		vkCreateShaderModule(device, &moduleCI, nullptr, &module);

		return module;
	}

	VkPipelineLayout createPipelineLayout(VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout* descriptorSetLayouts)
	{
		VkPushConstantRange range
		{
			.stageFlags{ VK_SHADER_STAGE_ALL_GRAPHICS },
			.offset{ 0 },
			.size{ sizeof(PushConstants) }
		};

		VkPipelineLayoutCreateInfo layoutCI
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO },
			.setLayoutCount{ layoutCount },
			.pSetLayouts{ descriptorSetLayouts },
			.pushConstantRangeCount{ 1 },
			.pPushConstantRanges{ &range },
		};

		VkPipelineLayout layout{};
		vkCreatePipelineLayout(device, &layoutCI, nullptr, &layout);

		return layout;
	}

	VkPipeline createPipeline(VkDevice device, VkPipelineLayout layout, VkExtent2D windowExtent, VkFormat colorAttachmentFormat)
	{
		VkPipelineRenderingCreateInfo renderingInfo
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO },
			.viewMask{ 0 },
			.colorAttachmentCount{ 1 },
			.pColorAttachmentFormats{ &colorAttachmentFormat },
			.depthAttachmentFormat{ VK_FORMAT_D32_SFLOAT },
			.stencilAttachmentFormat{ VK_FORMAT_UNDEFINED },
		};

		VkShaderModule vertexShaderModule{ createShaderModule(device, "shaders/uber.vert.spv") };
		VkShaderModule fragmentShaderModule{ createShaderModule(device, "shaders/uber.frag.spv") };

		VkPipelineShaderStageCreateInfo stages[2]{};
		stages[0] =
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
			.stage{ VK_SHADER_STAGE_VERTEX_BIT },
			.module{ vertexShaderModule },
			.pName{ "main" },
		};
		stages[1] =
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
			.stage{ VK_SHADER_STAGE_FRAGMENT_BIT },
			.module{ fragmentShaderModule },
			.pName{ "main" },
		};

		VkVertexInputBindingDescription binding
		{
			.binding{ 0 },
			.stride{ sizeof(Vertex) },
			.inputRate{ VK_VERTEX_INPUT_RATE_VERTEX },
		};

		VkVertexInputAttributeDescription attribs[4]{};
		attribs[0] =
		{
			.location{ 0 },
			.binding{ 0 },
			.format{ VK_FORMAT_R32G32B32_SFLOAT },
			.offset{ offsetof(Vertex, Vertex::pos) },
		};
		attribs[1] =
		{
			.location{ 1 },
			.binding{ 0 },
			.format{ VK_FORMAT_R32G32B32_SFLOAT },
			.offset{ offsetof(Vertex, Vertex::norm) },
		};
		attribs[2] =
		{
			.location{ 2 },
			.binding{ 0 },
			.format{ VK_FORMAT_R32G32B32_SFLOAT },
			.offset{ offsetof(Vertex, Vertex::color) },
		};
		attribs[3] =
		{
			.location{ 3 },
			.binding{ 0 },
			.format{ VK_FORMAT_R32G32_SFLOAT },
			.offset{ offsetof(Vertex, Vertex::tex) },
		};

		VkPipelineVertexInputStateCreateInfo vertexInputState
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO },
			.vertexBindingDescriptionCount{ 1 },
			.pVertexBindingDescriptions{ &binding },
			.vertexAttributeDescriptionCount{ 4 },
			.pVertexAttributeDescriptions{ attribs },
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO },
			.topology{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
			.primitiveRestartEnable{ VK_FALSE },
		};
		
		VkViewport viewport
		{
			.x{ 0.0f },
			.y{ 0.0f },
			.width{ static_cast<float>(windowExtent.width) },
			.height{ static_cast<float>(windowExtent.height) },
			.minDepth{ 0.0f },
			.maxDepth{ 1.0f },
		};

		VkRect2D scissor
		{
			.offset{ 0, 0 },
			.extent{ windowExtent },
		};

		VkPipelineViewportStateCreateInfo viewportState
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO },
			.viewportCount{ 1 },
			.pViewports{ &viewport },
			.scissorCount{ 1 },
			.pScissors{ &scissor }
		};

		VkPipelineRasterizationStateCreateInfo rasterizationState
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO },
			.depthClampEnable{ VK_FALSE },
			.rasterizerDiscardEnable{ VK_FALSE },
			.polygonMode{ VK_POLYGON_MODE_FILL },
			.cullMode{ VK_CULL_MODE_NONE },
			.frontFace{ VK_FRONT_FACE_CLOCKWISE },
			.depthBiasEnable{ VK_FALSE },
			.lineWidth{ 1.0f },
		};

		VkPipelineMultisampleStateCreateInfo multisampleState
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO },
			.rasterizationSamples{ VK_SAMPLE_COUNT_1_BIT },
			.sampleShadingEnable{ VK_FALSE },
			.alphaToCoverageEnable{ VK_FALSE },
			.alphaToOneEnable{ VK_FALSE },
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilState
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO },
			.depthTestEnable{ VK_TRUE },
			.depthWriteEnable{ VK_TRUE },
			.depthCompareOp{ VK_COMPARE_OP_LESS },
			.depthBoundsTestEnable{ VK_FALSE },
			.stencilTestEnable{ VK_FALSE },
			.minDepthBounds{ 0.0f },
			.maxDepthBounds{ 1.0f },
		};

		VkPipelineColorBlendAttachmentState attachment
		{
			.blendEnable{ VK_FALSE },
			.colorWriteMask{ VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT }
		};

		VkPipelineColorBlendStateCreateInfo colorBlendState
		{
			.sType{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO },
			.logicOpEnable{ VK_FALSE },
			.attachmentCount{ 1 },
			.pAttachments{ &attachment },
		};

		VkGraphicsPipelineCreateInfo pipelineCI
		{
			.sType{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO },
			.pNext{ &renderingInfo },
			.stageCount{ 2 },
			.pStages{ &stages[0] },
			.pVertexInputState{ &vertexInputState },
			.pInputAssemblyState{ &inputAssemblyState },
			.pViewportState{ &viewportState },
			.pRasterizationState{ &rasterizationState },
			.pMultisampleState{ &multisampleState },
			.pDepthStencilState{ &depthStencilState },
			.pColorBlendState{ &colorBlendState },
			.layout{ layout },
		};

		VkPipeline pipeline{};
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline);

		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
		vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

		return pipeline;
	}

}