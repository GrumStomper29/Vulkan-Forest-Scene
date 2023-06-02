#pragma once

#include "device.hpp"

#include "volk/volk.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Graphics
{

	class CmdBuffer final
	{
	public:
		// Allocates a command buffer in a new command pool
		CmdBuffer(const Device& device, std::uint32_t queueFamily);
		// Allocates a command buffer in the same pool as buffer
		CmdBuffer(const Device& device, const CmdBuffer& buffer);

		CmdBuffer(CmdBuffer&) = delete;
		CmdBuffer& operator=(CmdBuffer&) = delete;

		void reset(VkCommandBufferResetFlags flags = 0);

		void begin(VkCommandBufferUsageFlags flags = 0);

		void end()
		{
			vkEndCommandBuffer(m_buffer);
		}

		VkResult submit(const std::vector<VkSemaphore>& waitSemaphores,
			const std::vector<VkPipelineStageFlags>& waitStages,
			const std::vector<VkSemaphore>& signalSemaphores,
			VkQueue queue,
			VkFence fence);

		VkCommandPool vkCommandPool() const noexcept
		{
			return *m_pool;
		}

		VkCommandBuffer& vkCommandBuffer() noexcept
		{
			return m_buffer;
		}
		VkCommandBuffer vkCommandBuffer() const noexcept
		{
			return m_buffer;
		}

		std::uint32_t queueFamily() const noexcept
		{
			return m_queueFamily;
		}

	private:
		void allocateCmdBuffer(VkCommandPool cmdPool);

		std::shared_ptr<VkCommandPool> m_pool{};
		VkCommandBuffer m_buffer{};
		
		const Device& m_device;

		std::uint32_t m_queueFamily{};
	};

}