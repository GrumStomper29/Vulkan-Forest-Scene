#pragma once

#include "alloc.hpp"
#include "mesh.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"
#include "glm/glm.hpp"

#include <cstdint>
#include <vector>

namespace Graphics
{

	struct CameraUBOData
	{
		glm::mat4 viewProj{ 1.0f };
	};

	struct PushConstants
	{
		glm::mat4 vertexTransform{};
		std::uint32_t textureIndex{};
	};

	class Frame
	{
	public:
		static void init(VkDevice device);
		static void cleanup(VkDevice device);

		Frame(VkDevice device, VmaAllocator allocator, std::uint32_t queueFamily);

		Frame(Frame&& f) noexcept;
		Frame& operator=(Frame&& f) noexcept;

		Frame(const Frame&) = delete;
		Frame& operator=(const Frame&) = delete;

		~Frame();

		void execute(VkQueue queue, VkSwapchainKHR swapchain, VkExtent2D windowExtent, const std::vector<VkImage>& swapchainImages, const std::vector<VkImageView>& swapchainImageViews, const Image& depthImage, VkImageView depthImageView, bool firstFrame, VkPipeline pipeline, VkPipelineLayout pipelineLayout, const Buffer& vertexBuffer, const std::vector<RenderObject>& renderables, VkDescriptorSet descriptorSet);

		void* cameraUBOData{};

		VkDescriptorSetLayout getDescriptorSetLayout() const
		{
			return m_descriptorSetLayout;
		}

	private:
		VkCommandPool   m_cmdPool{};
		VkCommandBuffer m_cmdBuffer{};

		VkSemaphore m_renderSemaphore{};
		VkSemaphore m_presentSemaphore{};
		VkFence     m_renderFence{};

		Buffer m_cameraUBO{};

		VkDescriptorPool      m_descriptorPool{};
		static VkDescriptorSetLayout m_descriptorSetLayout;
		VkDescriptorSet       m_descriptorSet{};

		// The class does not actually own these objects.
		VkDevice m_device{};
		VmaAllocator m_allocator{};

		void destroyObjects();
		void move(Frame&& f);
	};

}