#include "instance.hpp"
#include "device.hpp"
#include "alloc.hpp"
#include "swapchain.hpp"

#include "attachment.hpp"

#include "cmd_buffer.hpp"
#include "sync.hpp"

#include "frame.hpp"

#include "descriptor.hpp"

#include "mesh.hpp"

#include "pipeline.hpp"

#include "camera.hpp"

#include "volk/volk.h"
#include "GLFW/glfw3.h"
#include "VMA/vk_mem_alloc.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cstdint>   // For std::memcpy
#include <cstring>   // For std::uint32_t
#include <exception>
#include <iostream>
#include <random>
#include <vector>

namespace Graphics
{

	struct Instance
	{
		VkExtent2D windowExtent{};
		GLFWwindow* window{};

		VkInstance instance{};
		VkPhysicalDevice physicalDevice{};

		std::uint32_t graphicsQueueFamily{};
		VkQueue graphicsQueue{};
		VkDevice device{};
		VmaAllocator allocator{};

		VkSurfaceKHR surface{};
		VkFormat swapchainImageFormat{};
		VkSwapchainKHR swapchain{};
		std::vector<VkImage> swapchainImages{};
		std::vector<VkImageView> swapchainImageViews{};
		VkSampleCountFlagBits sampleCount{};

		Image colorAttachmentImage{};
		VkImageView colorAttachmentImageView{};

		Image depthAttachmentImage{};
		VkImageView depthAttachmentImageView{};

		VkExtent2D shadowMapExtent{};
		Image shadowMap{};
		VkImageView shadowMapView{};
		VkSampler shadowMapSampler{};

		VkCommandPool GPCmdPool{};
		VkCommandBuffer GPCmdBuffer{};
		VkFence GPFence{};

		std::vector<Frame> framesInFlight{};

		VkDescriptorPool globalDescriptorPool{};
		VkDescriptorSetLayout globalDescriptorSetLayout{};
		VkDescriptorSet globalDescriptorSet{};

		VkPipelineLayout uberPipelineLayout{};
		VkPipeline uberPipeline{};
		VkPipeline shadowpassPipeline{};

		std::vector<RenderObject> renderObjects{};
		Buffer vertexBuffer{};

		std::vector<Texture> textures{};
		
		std::vector<RenderObjectInstance> renderObjectInstances{};
	};

	void init(Instance& instance, VkExtent2D windowExtent, const char* windowTitle)
	{
		if (!glfwInit())
		{
			throw std::exception{ "failed to initialize GLFW" };
		}

		if (volkInitialize() != VK_SUCCESS)
		{
			throw std::exception{ "failed to initialize volk" };
		}

		instance.windowExtent = windowExtent;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		instance.window = glfwCreateWindow(windowExtent.width, windowExtent.height, windowTitle, nullptr, nullptr);
		if (!instance.window)
		{
			throw std::exception{ "failed to create window" };
		}

		instance.instance = createInstance(true);
		instance.physicalDevice = getPhysicalDevice(instance.instance);

		instance.graphicsQueueFamily = getGraphicsQueueFamily(instance.physicalDevice);
		instance.device = createDevice(instance.physicalDevice, instance.graphicsQueueFamily, instance.graphicsQueue);
		instance.allocator = createAllocator(instance.instance, instance.physicalDevice, instance.device);

		glfwCreateWindowSurface(instance.instance, instance.window, nullptr, &instance.surface);
		instance.swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
		instance.swapchain = createSwapchain(instance.device, instance.surface, 4, instance.swapchainImageFormat, instance.windowExtent);
		instance.swapchainImages = getSwapchainImages(instance.device, instance.swapchain);
		instance.swapchainImageViews = createSwapchainImageViews(instance.device, instance.swapchainImages, instance.swapchainImageFormat);
		instance.sampleCount = VK_SAMPLE_COUNT_8_BIT;

		instance.colorAttachmentImage = createColorAttachmentImage(instance.allocator, instance.swapchainImageFormat, instance.windowExtent, instance.sampleCount);
		instance.colorAttachmentImageView = createColorAttachmentImageView(instance.device, instance.colorAttachmentImage.image, instance.swapchainImageFormat);

		instance.depthAttachmentImage = createDepthAttachmentImage(instance.allocator, instance.windowExtent, instance.sampleCount);
		instance.depthAttachmentImageView = createDepthAttachmentImageView(instance.device, instance.depthAttachmentImage.image);

		instance.shadowMapExtent = { 1024, 1024 };
		instance.shadowMap = createDepthAttachmentImage(instance.allocator, instance.shadowMapExtent,
			VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		instance.shadowMapView = createDepthAttachmentImageView(instance.device, instance.shadowMap.image);
		instance.shadowMapSampler = createShadowMapSampler(instance.device);

		instance.GPCmdPool = createCommandPool(instance.device, instance.graphicsQueueFamily, true);
		instance.GPCmdBuffer = allocateCommandBuffer(instance.device, instance.GPCmdPool);
		instance.GPFence = createFence(instance.device, false);

		Frame::init(instance.device);
		instance.framesInFlight.reserve(2);
		instance.framesInFlight.push_back({ instance.device, instance.allocator, instance.graphicsQueueFamily });
		instance.framesInFlight.push_back({ instance.device, instance.allocator, instance.graphicsQueueFamily });

		instance.globalDescriptorPool = createDescriptorPool(instance.device);
		instance.globalDescriptorSetLayout = createDescriptorSetLayout(instance.device);
		instance.globalDescriptorSet = allocateDescriptorSet(instance.device, instance.globalDescriptorPool, instance.globalDescriptorSetLayout);

		writeShadowMapSampler(instance.device, instance.shadowMapView, instance.shadowMapSampler, instance.globalDescriptorSet);

		VkDescriptorSetLayout setLayouts[2]
		{
			Frame::getDescriptorSetLayout(),
			instance.globalDescriptorSetLayout
		};
		instance.uberPipelineLayout = createPipelineLayout(instance.device, 2, setLayouts);

		GraphicsPipelineCreateInfo uberPipelineCI
		{
			.device{ instance.device },
			.colorAttachmentCount{ 1 },
			.pColorAttachmentFormats{ &instance.swapchainImageFormat },
			.depthFormat{ VK_FORMAT_D32_SFLOAT },
			.pVertexShaderPath{ "shaders/uber.vert.spv" },
			.pFragmentShaderPath{ "shaders/uber.frag.spv" },
			.viewportExtent{ instance.windowExtent },
			.sampleCount{ instance.sampleCount },
			.pipelineLayout{ instance.uberPipelineLayout },
		};
		instance.uberPipeline = createGraphicsPipeline(uberPipelineCI);

		GraphicsPipelineCreateInfo shadowpassPipelineCI
		{
			.device{ instance.device },
			.colorAttachmentCount{ 0 },
			.depthFormat{ VK_FORMAT_D32_SFLOAT },
			.pVertexShaderPath{ "shaders/shadow.vert.spv" },
			.pFragmentShaderPath{ "shaders/shadow.frag.spv" },
			.viewportExtent{ instance.shadowMapExtent },
			.sampleCount{ VK_SAMPLE_COUNT_1_BIT },
			.pipelineLayout{ instance.uberPipelineLayout },
		};
		instance.shadowpassPipeline = createGraphicsPipeline(shadowpassPipelineCI);
	}

	void loadRenderObjects(Instance& instance)
	{
		std::vector<Vertex> vertices{};

		instance.renderObjects.reserve(3);
		instance.renderObjects.push_back({ "assets/field/field.obj", vertices,instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence });
		instance.renderObjects.push_back({ "assets/tree/tree.obj", vertices, instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence });
		instance.renderObjects.push_back({ "assets/Grass_Block.obj", vertices, instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence });

		instance.renderObjects[1].meshes[1].opaque = false;

		instance.vertexBuffer = createVertexBuffer(vertices, instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence);

		for (auto& renderObject : instance.renderObjects)
		{
			for (auto& mesh : renderObject.meshes)
			{
				if (!mesh.diffusePath.empty())
				{
					mesh.textureIndex = instance.textures.size();
					std::string path{ "assets/" + mesh.diffusePath };
					instance.textures.push_back({ path.c_str(), instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence });
				}
				else
				{
					// Texture ID 1001 means no texture
					mesh.textureIndex = 1001;
				}
			}
		}

		std::vector<VkDescriptorImageInfo> imageInfos(instance.textures.size());
		std::vector<VkWriteDescriptorSet> writes(instance.textures.size());
		for (int i{ 0 }; i < instance.textures.size(); ++i)
		{
			imageInfos[i] =
			{
				.sampler{ instance.textures[i].vkSampler() },
				.imageView{ instance.textures[i].vkImageView() },
				.imageLayout{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			};
			writes[i] =
			{
				.sType{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET },
				.dstSet{ instance.globalDescriptorSet },
				.dstBinding{ 0 },
				.dstArrayElement{ static_cast<std::uint32_t>(i) },
				.descriptorCount{ 1 },
				.descriptorType{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
				.pImageInfo{ &imageInfos[i] },
			};
		}
		vkUpdateDescriptorSets(instance.device, writes.size(), writes.data(), 0, nullptr);

		instance.renderObjectInstances.push_back({ .renderObject{ 0 } });
		instance.renderObjectInstances[0].transform = glm::scale(instance.renderObjectInstances[0].transform, glm::vec3{ 1.0f });
		instance.renderObjectInstances[0].transform = glm::translate(instance.renderObjectInstances[0].transform, glm::vec3{ 0.0f, 0.0f, 0.0f });

		float treeHeights[7][7]{};

		// Far right column
		treeHeights[0][0] = 1.5f;
		treeHeights[0][1] = 1.0f;
		treeHeights[0][2] = 1.0f;
		treeHeights[0][3] = 1.1f;
		treeHeights[0][4] = 0.8f;
		treeHeights[0][5] = 1.3f;
		treeHeights[0][6] = 1.4f;

		treeHeights[1][0] = 0.6f;
		treeHeights[1][1] = -1.8f;
		treeHeights[1][2] = -1.2f;
		treeHeights[1][3] = -1.6f;
		treeHeights[1][4] = -1.9f;
		treeHeights[1][5] = 0.5f;
		treeHeights[1][6] = 1.5f;

		treeHeights[2][0] = 0.4f;
		treeHeights[2][1] = -3.5f;
		treeHeights[2][2] = -3.3f;
		treeHeights[2][3] = -4.1f;
		treeHeights[2][4] = -3.0f;
		treeHeights[2][5] = 0.0f;
		treeHeights[2][6] = 1.5f;

		treeHeights[3][0] = 0.4f;
		treeHeights[3][1] = -1.3f;
		treeHeights[3][2] = -1.3f;
		treeHeights[3][3] = -1.0f;
		treeHeights[3][4] = -1.2f;
		treeHeights[3][5] = -0.4f;
		treeHeights[3][6] = 1.5f;

		treeHeights[4][0] = 0.3f;
		treeHeights[4][1] = -0.4f;
		treeHeights[4][2] = 1.1f;
		treeHeights[4][3] = 1.1f;
		treeHeights[4][4] = 0.5f;
		treeHeights[4][5] = 0.2f;
		treeHeights[4][6] = 1.4f;

		treeHeights[5][0] = 0.5f;
		treeHeights[5][1] = 0.0f;
		treeHeights[5][2] = 0.8f;
		treeHeights[5][3] = 1.2f;
		treeHeights[5][4] = 1.2f;
		treeHeights[5][5] = 0.8f;
		treeHeights[5][6] = 1.4f;

		treeHeights[6][0] = 1.5f;
		treeHeights[6][1] = 1.1f;
		treeHeights[6][2] = 0.8f;
		treeHeights[6][3] = 1.2f;
		treeHeights[6][4] = 1.3f;
		treeHeights[6][5] = 1.3f;
		treeHeights[6][6] = 1.5f;

		std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution die360{ 1, 360 };
		std::uniform_int_distribution die7080{ 10, 15 };

		for (float n{ -3.0f }; n < 4.0f; ++n)
		{
			for (float i{ -3.0f }; i < 4.0f; ++i)
			{
				instance.renderObjectInstances.push_back({ .renderObject{ 1 } });

				glm::mat4 transform{ 1.0f };
				glm::vec3 scale
				{
					static_cast<float>(die7080(rng)) / 10.0f,
					static_cast<float>(die7080(rng)) / 10.0f,
					static_cast<float>(die7080(rng)) / 10.0f,
				};
				transform = glm::scale(transform, glm::vec3{ 7.0f, 7.0f, 7.0f });
				transform = glm::translate(transform, glm::vec3{ i * 5.0f, treeHeights[static_cast<int>(i) + 3][static_cast<int>(n) + 3], n * 5.0f });
				transform = glm::scale(transform, scale);

				float rot{ static_cast<float>(die360(rng)) };

				transform = glm::rotate(transform, glm::radians(rot), glm::vec3{ 0.0f, 1.0f, 0.0f });
				instance.renderObjectInstances.back().transform = transform;

				instance.renderObjectInstances.push_back({ .renderObject{ 2 } });
			}
		}
	}

	void run(Instance& instance)
	{
		Camera camera{ {0.0f, -3.0f, -10.0f }, { 0.0f, -90.0f } };

		int frameNumber{ 0 };
		bool firstFrame{ true };

		float lastFrame{ 0.0f };
		float deltaTime{ 0.0f };

		const glm::mat4 proj{ glm::perspective(glm::radians(90.0f), static_cast<float>(instance.windowExtent.width) / instance.windowExtent.height, 0.1f, 20000.0f) };

		while (!glfwWindowShouldClose(instance.window))
		{
			float current{ static_cast<float>(glfwGetTime()) };
			deltaTime = current - lastFrame;
			lastFrame = current;

			float camMoveSpeed{ 6.0f * deltaTime };
			float camRotateSpeed{ 100.0f * deltaTime };

			if (glfwGetKey(instance.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camMoveSpeed *= 10.0f;

			if (glfwGetKey(instance.window, GLFW_KEY_W) == GLFW_PRESS) camera.move(0.0f, 0.0f, -camMoveSpeed);
			if (glfwGetKey(instance.window, GLFW_KEY_S) == GLFW_PRESS) camera.move(0.0f, 0.0f, camMoveSpeed);
			if (glfwGetKey(instance.window, GLFW_KEY_A) == GLFW_PRESS) camera.move(-camMoveSpeed, 0.0f, 0.0f);
			if (glfwGetKey(instance.window, GLFW_KEY_D) == GLFW_PRESS) camera.move(camMoveSpeed, 0.0f, 0.0f);
			if (glfwGetKey(instance.window, GLFW_KEY_E) == GLFW_PRESS) camera.move(0.0f, -camMoveSpeed, 0.0f);
			if (glfwGetKey(instance.window, GLFW_KEY_Q) == GLFW_PRESS) camera.move(0.0f, camMoveSpeed, 0.0f);

			if (glfwGetKey(instance.window, GLFW_KEY_UP) == GLFW_PRESS)    camera.rotate(camRotateSpeed, 0.0f);
			if (glfwGetKey(instance.window, GLFW_KEY_DOWN) == GLFW_PRESS)  camera.rotate(-camRotateSpeed, 0.0f);
			if (glfwGetKey(instance.window, GLFW_KEY_LEFT) == GLFW_PRESS)  camera.rotate(0.0f, -camRotateSpeed);
			if (glfwGetKey(instance.window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.rotate(0.0f, camRotateSpeed);

			glm::mat4 lightView = glm::lookAt(glm::vec3(1.0f, -100.0f, 1.0f),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightProj = glm::ortho(-250.0f, 250.0f, 250.0f, -250.0f, -1000.0f, 1000.0f);

			CameraUBOData ubo{ .viewProj{ proj * camera.getViewMatrix() }, .lightTransform{ lightProj * lightView } };

			instance.framesInFlight[frameNumber].waitFrame();

			std::memcpy(instance.framesInFlight[frameNumber].cameraUBOData, &ubo, sizeof(CameraUBOData));

			RenderInfo renderInfo
			{
				.queue{ instance.graphicsQueue },
				.swapchain{ instance.swapchain },
				.windowExtent{ instance.windowExtent },
				.shadowViewport{ instance.shadowMapExtent },
				.swapchainImages{ instance.swapchainImages },
				.swapchainImageViews{ instance.swapchainImageViews },
				.colorImage{ instance.colorAttachmentImage },
				.colorImageView{ instance.colorAttachmentImageView },
				.depthImage{ instance.depthAttachmentImage },
				.depthImageView{ instance.depthAttachmentImageView },
				.shadowImage{ instance.shadowMap },
				.shadowImageView{ instance.shadowMapView },
				.firstFrame{ firstFrame },
				.pipeline{ instance.uberPipeline },
				.shadowPipeline{ instance.shadowpassPipeline },
				.pipelineLayout{ instance.uberPipelineLayout },
				.vertexBuffer{ instance.vertexBuffer },
				.renderObjects{ instance.renderObjects },
				.renderObjectInstances{ instance.renderObjectInstances },
				.descriptorSet{ instance.globalDescriptorSet },
			};

			instance.framesInFlight[frameNumber].execute(renderInfo);

			glfwPollEvents();

			frameNumber = ++frameNumber % 2;
			firstFrame = false;

			//glfwSetWindowShouldClose(instance.window, GLFW_TRUE);
		}
	}

	void cleanup(Instance& instance)
	{
		vkDeviceWaitIdle(instance.device);

		vkDestroyPipeline(instance.device, instance.shadowpassPipeline, nullptr);
		vkDestroyPipeline(instance.device, instance.uberPipeline, nullptr);
		vkDestroyPipelineLayout(instance.device, instance.uberPipelineLayout, nullptr);

		vmaDestroyBuffer(instance.allocator, instance.vertexBuffer.buffer, instance.vertexBuffer.alloc);

		instance.renderObjectInstances.clear();
		instance.textures.clear();
		instance.renderObjects.clear();

		vkDestroyDescriptorSetLayout(instance.device, instance.globalDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(instance.device, instance.globalDescriptorPool, nullptr);

		instance.framesInFlight.clear();
		Frame::cleanup(instance.device);

		vkDestroyFence(instance.device, instance.GPFence, nullptr);
		vkDestroyCommandPool(instance.device, instance.GPCmdPool, nullptr);

		vkDestroySampler(instance.device, instance.shadowMapSampler, nullptr);
		vkDestroyImageView(instance.device, instance.shadowMapView, nullptr);
		vmaDestroyImage(instance.allocator, instance.shadowMap.image, instance.shadowMap.alloc);

		vkDestroyImageView(instance.device, instance.depthAttachmentImageView, nullptr);
		vmaDestroyImage(instance.allocator, instance.depthAttachmentImage.image, instance.depthAttachmentImage.alloc);

		vkDestroyImageView(instance.device, instance.colorAttachmentImageView, nullptr);
		vmaDestroyImage(instance.allocator, instance.colorAttachmentImage.image, instance.colorAttachmentImage.alloc);

		for (VkImageView& swapchainImageView : instance.swapchainImageViews)
		{
			vkDestroyImageView(instance.device, swapchainImageView, nullptr);
		}
		vkDestroySwapchainKHR(instance.device, instance.swapchain, nullptr);
		vkDestroySurfaceKHR(instance.instance, instance.surface, nullptr);
		vmaDestroyAllocator(instance.allocator);
		vkDestroyDevice(instance.device, nullptr);
		vkDestroyInstance(instance.instance, nullptr);

		glfwDestroyWindow(instance.window);

		glfwTerminate();
	}

}

int main()
{
	Graphics::Instance graphicsInstance{};

	Graphics::init(graphicsInstance, VkExtent2D{ 1600, 900 }, "Vulkan Forest Scene");
	Graphics::loadRenderObjects(graphicsInstance);

	Graphics::run(graphicsInstance);

	Graphics::cleanup(graphicsInstance);

	return 0;
}