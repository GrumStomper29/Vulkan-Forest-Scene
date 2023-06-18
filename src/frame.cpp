#include "frame.hpp"

#include "alloc.hpp"
#include "cmd_buffer.hpp"
#include "sync.hpp"
#include "swapchain.hpp"
#include "mesh.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"
#include "glm/glm.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace Graphics
{

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
			.usage{ VMA_MEMORY_USAGE_AUTO },
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

	void Frame::execute(VkQueue queue, VkSwapchainKHR swapchain, VkExtent2D windowExtent, const std::vector<VkImage>& swapchainImages, const std::vector<VkImageView>& swapchainImageViews, const Image& depthImage, VkImageView depthImageView, bool firstFrame, VkPipeline pipeline, VkPipelineLayout pipelineLayout, const Buffer& vertexBuffer, const std::vector<Renderable>& renderables, PushConstants pushConstants)
	{
		vkWaitForFences(m_device, 1, &m_renderFence, VK_TRUE, secondsToNanoseconds(1));
		vkResetFences(m_device, 1, &m_renderFence);

		std::uint32_t swapchainImageIndex{ acquireNextSwapchainImage(m_device, swapchain, m_presentSemaphore) };

		vkResetCommandPool(m_device, m_cmdPool, 0);
		beginCommandBuffer(m_cmdBuffer, true);

		prepareImageForColorAttachmentOutput(m_cmdBuffer, swapchainImages[swapchainImageIndex]);

		if (firstFrame)
		{
			correctDepthImageLayout(depthImage.image, m_cmdBuffer);
		}

		VkRenderingAttachmentInfo colorAttachment
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO },
			.imageView{ swapchainImageViews[swapchainImageIndex] },
			.imageLayout{ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			.resolveMode{ VK_RESOLVE_MODE_NONE },
			.loadOp{ VK_ATTACHMENT_LOAD_OP_CLEAR },
			.storeOp{ VK_ATTACHMENT_STORE_OP_STORE },
			.clearValue
			{
				.color{ 0.6f, 0.0f, 0.9f, 1.0f },
			},
		};

		VkRenderingAttachmentInfo depthAttachment
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO },
			.imageView{ depthImageView },
			.imageLayout{ VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
			.resolveMode{ VK_RESOLVE_MODE_NONE },
			.loadOp{ VK_ATTACHMENT_LOAD_OP_CLEAR },
			.storeOp{ VK_ATTACHMENT_STORE_OP_STORE },
			.clearValue{.depthStencil{.depth{ 1.0f } } },
		};

		VkRenderingInfo renderingInfo
		{
			.sType{ VK_STRUCTURE_TYPE_RENDERING_INFO },
			.renderArea
			{
				.offset{ 0, 0 },
				.extent{ windowExtent },
			},
			.layerCount{ 1 },
			.viewMask{ 0 },
			.colorAttachmentCount{ 1 },
			.pColorAttachments{ &colorAttachment },
			.pDepthAttachment{ &depthAttachment },
		};

		vkCmdBeginRendering(m_cmdBuffer, &renderingInfo);

		vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		vkCmdPushConstants(m_cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);
		vkCmdBindDescriptorSets(m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

		constexpr VkDeviceSize offset{ 0 };
		vkCmdBindVertexBuffers(m_cmdBuffer, 0, 1, &vertexBuffer.buffer, &offset);

		for (const auto& renderable : renderables)
		{
			vkCmdBindIndexBuffer(m_cmdBuffer, renderable.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(m_cmdBuffer, renderable.indexCount, 1, 0, 0, 0);
		}

		vkCmdEndRendering(m_cmdBuffer);

		prepareImageForPresentation(m_cmdBuffer, swapchainImages[swapchainImageIndex]);

		vkEndCommandBuffer(m_cmdBuffer);

		queueSubmit(queue, m_cmdBuffer, m_presentSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, m_renderSemaphore, m_renderFence);

		swapchainQueuePresent(queue, swapchain, m_renderSemaphore, swapchainImageIndex);
	}

	void Frame::destroyObjects()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
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
		f.m_cmdPool = VK_NULL_HANDLE;
		m_cmdBuffer = f.m_cmdBuffer;
		f.m_cmdBuffer = VK_NULL_HANDLE;

		m_renderSemaphore = f.m_renderSemaphore;
		f.m_renderSemaphore = VK_NULL_HANDLE;
		m_presentSemaphore = f.m_presentSemaphore;
		f.m_presentSemaphore = VK_NULL_HANDLE;
		m_renderFence = f.m_renderFence;
		f.m_renderFence = VK_NULL_HANDLE;

		m_cameraUBO = f.m_cameraUBO;
		f.m_cameraUBO = {};
		cameraUBOData = f.cameraUBOData;

		m_descriptorPool = f.m_descriptorPool;
		f.m_descriptorPool = VK_NULL_HANDLE;
		m_descriptorSetLayout = f.m_descriptorSetLayout;
		f.m_descriptorSetLayout = VK_NULL_HANDLE;
		m_descriptorSet = f.m_descriptorSet;
		f.m_descriptorSet = VK_NULL_HANDLE;
	}

}