#pragma once

#include "alloc.hpp"

#include "volk/volk.h"
#include "VMA/vk_mem_alloc.h"

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Graphics
{

	struct Vertex
	{
		glm::vec3 pos{};
		glm::vec3 norm{};
		glm::vec3 color{};
		glm::vec2 tex{};

		bool operator==(const Vertex& v) const
		{
			return pos == v.pos && norm == v.norm && color == v.color && tex == v.tex;
		}
	};

}

template<>
struct std::hash<Graphics::Vertex>
{
	size_t operator()(const Graphics::Vertex& v) const noexcept
	{
		size_t h1{ hash<glm::vec3>{}(v.pos) };
		size_t h2{ hash<glm::vec3>{}(v.norm) };
		size_t h3{ hash<glm::vec3>{}(v.color) };
		size_t h4{ hash<glm::vec2>{}(v.tex) };

		constexpr std::uint64_t multiplier{ 6364136223846793005 };
		constexpr std::uint64_t increment{ 1442695040888963407 };

		size_t finalHash{ h1 };
		finalHash = finalHash ^ (h2 * multiplier + increment);
		finalHash = finalHash ^ (h3 * multiplier + increment);
		finalHash = finalHash ^ (h4 * multiplier + increment);

		return finalHash;
	}
};

namespace Graphics
{

	Buffer createVertexBuffer(std::vector<Vertex>& vertices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

	Buffer createIndexBuffer(std::vector<std::uint32_t>& indices, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

	Image loadImage(const char* path, std::uint32_t& mipLevels, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

	class RenderObject
	{
	public:
		struct Mesh
		{
			int                                       material{};
			std::vector<std::uint32_t>                indices{};
			std::string                               diffusePath{};
			Buffer                                    indexBuffer{};
			std::uint32_t                             indexCount{};
			std::uint32_t                             textureIndex{};
			bool                                      draw{ true };
			std::unordered_map<Vertex, std::uint32_t> map{};
		};

		RenderObject(const char* path, std::vector<Vertex>& vertices, VkDevice device, 
			VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

		RenderObject(const RenderObject&) = delete;
		RenderObject& operator=(const RenderObject&) = delete;

		RenderObject(RenderObject&& r) noexcept;
		RenderObject& operator=(RenderObject&& r) noexcept;

		~RenderObject();

		std::vector<Mesh> meshes{};
		glm::mat4         transform{ 1.0f };
	private:
		// Not owned by the class
		VmaAllocator m_allocator{};

		void move(RenderObject&& r);
		void destroy();
	};

	class Texture
	{
	public:
		Texture(const char* path, VkDevice device, VmaAllocator allocator, VkQueue queue, VkCommandBuffer commandBuffer, VkFence fence);

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		Texture(Texture&& t);
		Texture& operator=(Texture&& t);

		~Texture();

		VkImageView vkImageView() const
		{
			return m_imageView;
		}
		VkSampler vkSampler() const
		{
			return m_sampler;
		}

	private:
		std::uint32_t m_mipLevels{};
		Image         m_image{};
		VkImageView   m_imageView{};
		VkSampler     m_sampler{};

		// Not owned by class
		VkDevice     m_device{};
		VmaAllocator m_allocator{};

		void move(Texture&& t);
		void destroy();
	};

}