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
#include "texture.hpp"

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
		VkExtent2D  windowExtent{};
		GLFWwindow* window{};

		VkInstance       instance{};
		VkPhysicalDevice physicalDevice{};

		std::uint32_t graphicsQueueFamily{};
		VkQueue       graphicsQueue{};
		VkDevice      device{};
		VmaAllocator  allocator{};

		VkSurfaceKHR             surface{};
		VkFormat                 swapchainImageFormat{};
		VkSwapchainKHR           swapchain{};
		std::vector<VkImage>     swapchainImages{};
		std::vector<VkImageView> swapchainImageViews{};
		VkSampleCountFlagBits    sampleCount{};

		Image       colorAttachmentImage{};
		VkImageView colorAttachmentImageView{};

		Image       depthAttachmentImage{};
		VkImageView depthAttachmentImageView{};

		VkExtent2D  shadowMapExtent{};
		Image       shadowMap{};
		VkImageView shadowMapView{};
		VkSampler   shadowMapSampler{};

		VkCommandPool   GPCmdPool{};
		VkCommandBuffer GPCmdBuffer{};
		VkFence         GPFence{};

		std::vector<Frame> framesInFlight{};

		VkDescriptorPool      globalDescriptorPool{};
		VkDescriptorSetLayout globalDescriptorSetLayout{};
		VkDescriptorSet       globalDescriptorSet{};

		VkPipelineLayout uberPipelineLayout{};
		VkPipeline       uberPipeline{};
		VkPipeline       shadowpassPipeline{};
		VkPipeline       skyboxPipeline{};

		std::vector<RenderObject> renderObjects{};
		Buffer                    vertexBuffer{};

		std::vector<Texture> textures{};

		Image       skybox{};
		VkImageView skyboxView{};
		VkSampler   skyboxSampler{};
		
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

		instance.instance       = createInstance(true);
		instance.physicalDevice = getPhysicalDevice(instance.instance);

		instance.graphicsQueueFamily = getGraphicsQueueFamily(instance.physicalDevice);
		instance.device              = createDevice(instance.physicalDevice, instance.graphicsQueueFamily, instance.graphicsQueue);
		instance.allocator           = createAllocator(instance.instance, instance.physicalDevice, instance.device);

		glfwCreateWindowSurface(instance.instance, instance.window, nullptr, &instance.surface);
		instance.swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
		instance.swapchain            = createSwapchain(instance.device, instance.surface, 4, instance.swapchainImageFormat, instance.windowExtent);
		instance.swapchainImages      = getSwapchainImages(instance.device, instance.swapchain);
		instance.swapchainImageViews  = createSwapchainImageViews(instance.device, instance.swapchainImages, instance.swapchainImageFormat);
		instance.sampleCount          = VK_SAMPLE_COUNT_8_BIT;

		instance.colorAttachmentImage     = createColorAttachmentImage(instance.allocator, instance.swapchainImageFormat, instance.windowExtent, instance.sampleCount);
		instance.colorAttachmentImageView = createColorAttachmentImageView(instance.device, instance.colorAttachmentImage.image, instance.swapchainImageFormat);

		instance.depthAttachmentImage     = createDepthAttachmentImage(instance.allocator, instance.windowExtent, instance.sampleCount);
		instance.depthAttachmentImageView = createDepthAttachmentImageView(instance.device, instance.depthAttachmentImage.image);

		instance.shadowMapExtent  = { 2048, 2048 };
		instance.shadowMap        = createDepthAttachmentImage(instance.allocator, instance.shadowMapExtent,
		                                VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		instance.shadowMapView    = createDepthAttachmentImageView(instance.device, instance.shadowMap.image);
		instance.shadowMapSampler = createShadowMapSampler(instance.device);

		instance.GPCmdPool   = createCommandPool(instance.device, instance.graphicsQueueFamily, true);
		instance.GPCmdBuffer = allocateCommandBuffer(instance.device, instance.GPCmdPool);
		instance.GPFence     = createFence(instance.device, false);

		Frame::init(instance.device);
		instance.framesInFlight.reserve(2);
		instance.framesInFlight.push_back({ instance.device, instance.allocator, instance.graphicsQueueFamily });
		instance.framesInFlight.push_back({ instance.device, instance.allocator, instance.graphicsQueueFamily });

		instance.globalDescriptorPool      = createDescriptorPool(instance.device);
		instance.globalDescriptorSetLayout = createDescriptorSetLayout(instance.device);
		instance.globalDescriptorSet       = allocateDescriptorSet(instance.device, instance.globalDescriptorPool, instance.globalDescriptorSetLayout);

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

		GraphicsPipelineCreateInfo skyboxPipelineCI
		{
			.device{ instance.device },
			.colorAttachmentCount{ 1 },
			.pColorAttachmentFormats{ &instance.swapchainImageFormat },
			.depthFormat{ VK_FORMAT_D32_SFLOAT },
			.pVertexShaderPath{ "shaders/skybox.vert.spv" },
			.pFragmentShaderPath{ "shaders/skybox.frag.spv" },
			.viewportExtent{ instance.windowExtent },
			.sampleCount{ instance.sampleCount },
			.pipelineLayout{ instance.uberPipelineLayout },
			.depthTestEnable{ false },
		};
		instance.skyboxPipeline = createGraphicsPipeline(skyboxPipelineCI);
	}

	void loadRenderObjects(Instance& instance)
	{
		std::vector<Vertex> vertices{};

		instance.renderObjects.reserve(4);
		instance.renderObjects.push_back({ "assets/forest.obj", vertices,instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence });
		instance.renderObjects.push_back({ "assets/skybox/obj.obj", vertices, instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence });

		instance.renderObjects[0].meshes[1].opaque = false;
		instance.renderObjects[0].meshes[2].opaque = false;
		instance.renderObjects[0].meshes[4].opaque = false;

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

		writeTextureSamplers(instance.device, instance.globalDescriptorSet, instance.textures);

		const char* paths[6]
		{
			"assets/skybox/px.png",
			"assets/skybox/nx.png",
			"assets/skybox/py.png",
			"assets/skybox/ny.png",
			"assets/skybox/pz.png",
			"assets/skybox/nz.png",
		};
		instance.skybox = loadSkybox(paths, instance.device, instance.allocator, instance.graphicsQueue, instance.GPCmdBuffer, instance.GPFence);
		instance.skyboxView = createSkyboxView(instance.device, instance.skybox.image);
		instance.skyboxSampler = createSkyboxSampler(instance.device);
		writeSkyboxSampler(instance.device, instance.globalDescriptorSet, instance.skyboxView, instance.skyboxSampler);

		instance.renderObjectInstances.push_back({ .renderObject{ 0 } });
		instance.renderObjectInstances[0].transform = glm::scale(instance.renderObjectInstances[0].transform, glm::vec3{ 100.0f });
		instance.renderObjectInstances[0].transform = glm::translate(instance.renderObjectInstances[0].transform, glm::vec3{ 0.0f, 0.0f, 0.0f });
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

			glm::mat4 lightView = glm::lookAt(glm::vec3(-400.0f, -200.0f, 600.0f),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightProj = glm::ortho(-1250.0f, 1250.0f, 1250.0f, -1250.0f, -1000.0f, 1000.0f);

			CameraUBOData ubo{ .viewProj{ proj * camera.getViewMatrix() }, .lightTransform{ lightProj * lightView } };

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
				.skyboxPipeline{ instance.skyboxPipeline },
				.pipelineLayout{ instance.uberPipelineLayout },
				.vertexBuffer{ instance.vertexBuffer },
				.renderObjects{ instance.renderObjects },
				.renderObjectInstances{ instance.renderObjectInstances },
				.descriptorSet{ instance.globalDescriptorSet },
				.cameraView{ camera.getViewMatrix() },
				.cameraProj{ proj },
				.lightView{ lightView },
				.lightProj{ lightProj },
				.skyboxRenderObjectIndex{ 1 },
			};

			instance.framesInFlight[frameNumber].waitFrame();

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

		vkDestroyPipeline(instance.device, instance.skyboxPipeline, nullptr);
		vkDestroyPipeline(instance.device, instance.shadowpassPipeline, nullptr);
		vkDestroyPipeline(instance.device, instance.uberPipeline, nullptr);
		vkDestroyPipelineLayout(instance.device, instance.uberPipelineLayout, nullptr);

		vmaDestroyBuffer(instance.allocator, instance.vertexBuffer.buffer, instance.vertexBuffer.alloc);

		instance.renderObjectInstances.clear();

		vkDestroySampler(instance.device, instance.skyboxSampler, nullptr);
		vkDestroyImageView(instance.device, instance.skyboxView, nullptr);
		vmaDestroyImage(instance.allocator, instance.skybox.image, instance.skybox.alloc);

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