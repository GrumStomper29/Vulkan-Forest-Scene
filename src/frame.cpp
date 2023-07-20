#include "frame.hpp"

#include "alloc.hpp"
#include "cmd_buffer.hpp"
#include "sync.hpp"
#include "swapchain.hpp"
#include "mesh.hpp"
#include "attachment.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"
#include "glm/glm.hpp"

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace Graphics
{

	struct QueuedMesh
	{
		const RenderObject::Mesh& mesh{};
		const glm::mat4& transform{};
	};

	VkDescriptorSetLayout Frame::m_descriptorSetLayout{};

	void Frame::init(VkDevice device)
	{
		VkDescriptorSetLayoutBinding setLayoutBinding
		{
			.binding{ 0 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			.descriptorCount{ 1 },
			.stageFlags{ VK_SHADER_STAGE_VERTEX_BIT },
		};
		VkDescriptorSetLayoutCreateInfo setLayoutCI
		{
			.sType{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO },
			.bindingCount{ 1 },
			.pBindings{ &setLayoutBinding },
		};
		vkCreateDescriptorSetLayout(device, &setLayoutCI, nullptr, &m_descriptorSetLayout);
	}

	void Frame::cleanup(VkDevice device)
	{
		vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
	}

	Frame::Frame(VkDevice device, VmaAllocator allocator, std::uint32_t queueFamily)
		: m_device{ device },
		  m_allocator{ allocator }
	{
		m_cmdPool   = createCommandPool(device, queueFamily, false);
		m_cmdBuffer = allocateCommandBuffer(device, m_cmdPool);

		m_renderSemaphore  = createSemaphore(device);
		m_presentSemaphore = createSemaphore(device);
		m_renderFence      = createFence(device, true);

		VkBufferCreateInfo bufferCI
		{
			.sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
			.size{ sizeof(CameraUBOData) },
			.usage{ VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
		};
		VmaAllocationCreateInfo allocCI
		{
			.flags{ VMA_ALLOCATION_CREATE_MAPPED_BIT },
			.usage{ VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE },
			.requiredFlags{ VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT },
		};
		VmaAllocationInfo allocInfo{};
		vmaCreateBuffer(allocator, &bufferCI, &allocCI, &m_cameraUBO.buffer, &m_cameraUBO.alloc, &allocInfo);
		cameraUBOData = allocInfo.pMappedData;

		VkDescriptorPoolSize size
		{
			.type{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			.descriptorCount{ 1 },
		};
		VkDescriptorPoolCreateInfo poolCI
		{
			.sType{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO },
			.maxSets{ 1 },
			.poolSizeCount{ 1 },
			.pPoolSizes{ &size },
		};
		vkCreateDescriptorPool(device, &poolCI, nullptr, &m_descriptorPool);

		VkDescriptorSetAllocateInfo setAllocInfo
		{
			.sType{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO },
			.descriptorPool{ m_descriptorPool },
			.descriptorSetCount{ 1 },
			.pSetLayouts{ &m_descriptorSetLayout },
		};
		vkAllocateDescriptorSets(device, &setAllocInfo, &m_descriptorSet);

		VkDescriptorBufferInfo descriptorBufferInfo
		{
			.buffer{ m_cameraUBO.buffer },
			.offset{ 0 },
			.range{ VK_WHOLE_SIZE },
		};
		VkWriteDescriptorSet write
		{
			.sType{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
			.dstSet{ m_descriptorSet },
			.dstBinding{ 0 },
			.descriptorCount{ 1 },
			.descriptorType{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			.pBufferInfo{ &descriptorBufferInfo },
		};
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	Frame::Frame(Frame&& f) noexcept
	{
		move(std::move(f));
	}

	Frame& Frame::operator=(Frame&& f) noexcept
	{
		destroyObjects();

		move(std::move(f));

		return *this;
	}

	Frame::~Frame()
	{
		destroyObjects();
	}

	void Frame::waitFrame()
	{
		vkWaitForFences(m_device, 1, &m_renderFence, VK_TRUE, secondsToNanoseconds(1));
		vkResetFences(m_device, 1, &m_renderFence);
	}

	void Frame::execute(const RenderInfo& renderInfo)
	{
		std::uint32_t swapchainImageIndex{ acquireNextSwapchainImage(m_device, renderInfo.swapchain, m_presentSemaphore) };

		CameraUBOData ubo
		{
			.viewProj{ renderInfo.cameraProj * renderInfo.cameraView },
			.lightTransform{ renderInfo.lightProj * renderInfo.lightView },
		};
		std::memcpy(cameraUBOData, &ubo, sizeof(CameraUBOData));

		vkResetCommandPool(m_device, m_cmdPool, 0);
		beginCommandBuffer(m_cmdBuffer, true);

		shadowpass(renderInfo);

		renderpass(renderInfo, swapchainImageIndex);

		vkEndCommandBuffer(m_cmdBuffer);

		queueSubmit(renderInfo.queue, m_cmdBuffer, m_presentSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, m_renderSemaphore, m_renderFence);

		swapchainQueuePresent(renderInfo.queue, renderInfo.swapchain, m_renderSemaphore, swapchainImageIndex);
	}

	void Frame::shadowpass(const RenderInfo& renderInfo)
	{
		correctDepthAttachmentImageLayout(renderInfo.shadowImage.image, m_cmdBuffer);

		VkRenderingAttachmentInfo shadowMapAttachment
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO },
			.imageView{ renderInfo.shadowImageView },
			.imageLayout{ VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
			.resolveMode{ VK_RESOLVE_MODE_NONE },
			.loadOp{ VK_ATTACHMENT_LOAD_OP_CLEAR },
			.storeOp{ VK_ATTACHMENT_STORE_OP_STORE },
			.clearValue{ .depthStencil{ .depth{ 1.0f } } },
		};

		VkRenderingInfo renderingInfo
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_INFO },
			.renderArea
			{
				.offset{ 0, 0 },
				.extent{ renderInfo.shadowViewport },
			},
			.layerCount{ 1 },
			.colorAttachmentCount{ 0 },
			.pDepthAttachment{ &shadowMapAttachment },
		};

		vkCmdBeginRendering(m_cmdBuffer, &renderingInfo);

		vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo.shadowPipeline);

		VkDescriptorSet descriptorSets[2]
		{
			m_descriptorSet,
			renderInfo.descriptorSet
		};
		vkCmdBindDescriptorSets(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);

		constexpr VkDeviceSize offset{ 0 };
		vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &renderInfo.vertexBuffer.buffer, &offset);

		for (const auto& instance : renderInfo.renderObjectInstances)
		{
			for (const auto& mesh : renderInfo.renderObjects[instance.renderObject].meshes)
			{
				if (mesh.draw)
				{
					if (mesh.opaque)
					{
						PushConstants pushConstants{ instance.transform };
						vkCmdPushConstants(m_cmdBuffer, renderInfo.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants),
							&pushConstants);

						vkCmdBindIndexBuffer(m_cmdBuffer, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
						vkCmdDrawIndexed(m_cmdBuffer, mesh.indexCount, 1, 0, 0, 0);
					}
				}
			}
		}

		vkCmdEndRendering(m_cmdBuffer);

		prepareDepthImageForSampling(m_cmdBuffer, renderInfo.shadowImage.image);
	}

	void Frame::renderpass(const RenderInfo& renderInfo, std::uint32_t swapchainImageIndex)
	{
		prepareImageForColorAttachmentOutput(m_cmdBuffer, renderInfo.swapchainImages[swapchainImageIndex]);

		if (renderInfo.firstFrame)
		{
			correctColorAttachmentImageLayout(renderInfo.colorImage.image, m_cmdBuffer);
			correctDepthAttachmentImageLayout(renderInfo.depthImage.image, m_cmdBuffer);
		}

		VkRenderingAttachmentInfo colorAttachmentResolve
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO },
			.imageView{ renderInfo.colorImageView },
			.imageLayout{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			.resolveMode{ VK_RESOLVE_MODE_AVERAGE_BIT },
			.resolveImageView{ renderInfo.swapchainImageViews[swapchainImageIndex] },
			.resolveImageLayout{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			.loadOp{ VK_ATTACHMENT_LOAD_OP_CLEAR },
			.storeOp{ VK_ATTACHMENT_STORE_OP_STORE },
			.clearValue
			{
				.color{ 0.52f, 0.8f, 0.92f, 1.0f },
			},
		};

		VkRenderingAttachmentInfo depthAttachment
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO },
			.imageView{ renderInfo.depthImageView },
			.imageLayout{ VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
			.resolveMode{ VK_RESOLVE_MODE_NONE },
			.loadOp{ VK_ATTACHMENT_LOAD_OP_CLEAR },
			.storeOp{ VK_ATTACHMENT_STORE_OP_STORE },
			.clearValue{.depthStencil{.depth{ 1.0f } } },
		};

		VkRenderingAttachmentInfo colorAttachments[1]{ colorAttachmentResolve };

		VkRenderingInfo renderingInfo
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_INFO },
			.renderArea
			{
				.offset{ 0, 0 },
				.extent{ renderInfo.windowExtent },
			},
			.layerCount{ 1 },
			.viewMask{ 0 },
			.colorAttachmentCount{ 1 },
			.pColorAttachments{ colorAttachments },
			.pDepthAttachment{ &depthAttachment },
		};

		vkCmdBeginRendering(m_cmdBuffer, &renderingInfo);

		VkDescriptorSet descriptorSets[2]
		{
			m_descriptorSet,
			renderInfo.descriptorSet
		};
		vkCmdBindDescriptorSets(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);

		constexpr VkDeviceSize offset{ 0 };
		vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &renderInfo.vertexBuffer.buffer, &offset);

		vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo.skyboxPipeline);

		glm::mat4 view{ glm::mat3{ renderInfo.cameraView } };
		PushConstants pushConstant{ { renderInfo.cameraProj * view }, 0 };
		vkCmdPushConstants(m_cmdBuffer, renderInfo.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants), &pushConstant);

		vkCmdBindIndexBuffer(m_cmdBuffer, renderInfo.renderObjects[renderInfo.skyboxRenderObjectIndex].meshes[0].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_cmdBuffer, renderInfo.renderObjects[renderInfo.skyboxRenderObjectIndex].meshes[0].indexCount, 1, 0, 0, 0);

		vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderInfo.pipeline);

		// Meshes with transparency should be drawn last
		std::vector<QueuedMesh> meshQueue{};

		for (const auto& instance : renderInfo.renderObjectInstances)
		{
			for (const auto& mesh : renderInfo.renderObjects[instance.renderObject].meshes)
			{
				if (mesh.draw)
				{
					if (mesh.opaque)
					{
						PushConstants pushConstants{ instance.transform, mesh.textureIndex };
						vkCmdPushConstants(m_cmdBuffer, renderInfo.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants), &pushConstants);

						vkCmdBindIndexBuffer(m_cmdBuffer, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
						vkCmdDrawIndexed(m_cmdBuffer, mesh.indexCount, 1, 0, 0, 0);
					}
					else
					{
						meshQueue.push_back({
							.mesh{ mesh },
							.transform{ instance.transform },
							});
					}
				}
			}
		}
		
		for (const auto& queuedMesh : meshQueue)
		{
			PushConstants pushConstants{ queuedMesh.transform, queuedMesh.mesh.textureIndex };
			vkCmdPushConstants(m_cmdBuffer, renderInfo.pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(PushConstants), &pushConstants);

			vkCmdBindIndexBuffer(m_cmdBuffer, queuedMesh.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(m_cmdBuffer, queuedMesh.mesh.indexCount, 1, 0, 0, 0);
		}
		
		vkCmdEndRendering(m_cmdBuffer);

		prepareImageForPresentation(m_cmdBuffer, renderInfo.swapchainImages[swapchainImageIndex]);
	}

	void Frame::destroyObjects()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

			vmaDestroyBuffer(m_allocator, m_cameraUBO.buffer, m_cameraUBO.alloc);

			vkDestroyFence(m_device, m_renderFence, nullptr);
			vkDestroySemaphore(m_device, m_presentSemaphore, nullptr);
			vkDestroySemaphore(m_device, m_renderSemaphore, nullptr);

			vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
		}
	}

	void Frame::move(Frame&& f)
	{
		m_device = f.m_device;
		f.m_device = VK_NULL_HANDLE;
		m_allocator = f.m_allocator;

		m_cmdPool = f.m_cmdPool;
		m_cmdBuffer = f.m_cmdBuffer;

		m_renderSemaphore = f.m_renderSemaphore;
		m_presentSemaphore = f.m_presentSemaphore;
		m_renderFence = f.m_renderFence;

		m_cameraUBO = f.m_cameraUBO;
		cameraUBOData = f.cameraUBOData;

		m_descriptorPool = f.m_descriptorPool;
		m_descriptorSet = f.m_descriptorSet;
	}

}