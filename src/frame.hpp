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
		glm::mat4 lightTransform{ 1.0f };
	};

	struct PushConstants
	{
		glm::mat4 vertexTransform{};
		std::uint32_t textureIndex{};
	};

	struct ShadowPassPushConstants
	{
		glm::mat4 vertexTransform{};
		glm::mat4 lightTransform{};
	};

	struct RenderInfo
	{
		VkQueue queue{};
		VkSwapchainKHR swapchain{};
		VkExtent2D windowExtent{};
		VkExtent2D shadowViewport{};
		const std::vector<VkImage>& swapchainImages{};
		const std::vector<VkImageView>& swapchainImageViews{};
		const Image& colorImage{};
		VkImageView colorImageView{};
		const Image& depthImage{};
		VkImageView depthImageView{};
		const Image& shadowImage{};
		VkImageView shadowImageView{};
		bool firstFrame{};
		VkPipeline pipeline{};
		VkPipeline shadowPipeline{};
		VkPipelineLayout pipelineLayout{};
		VkPipelineLayout shadowPipelineLayout{};
		const Buffer& vertexBuffer{};
		const std::vector<RenderObject>& renderObjects{};
		const std::vector<RenderObjectInstance>& renderObjectInstances{};
		VkDescriptorSet descriptorSet{};
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

		void waitFrame();
		void execute(const RenderInfo& renderInfo);

		void* cameraUBOData{};

		static VkDescriptorSetLayout getDescriptorSetLayout()
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