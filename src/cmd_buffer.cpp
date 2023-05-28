#include "cmd_buffer.hpp"

#include "volk/volk.h"

#include <cstdint>
#include <memory>

namespace Graphics
{

	CmdBuffer::CmdBuffer(const Device& device, std::uint32_t queueFamily)
		: m_device{ device }
		, m_queueFamily{ queueFamily }
	{
		m_pool = std::shared_ptr<VkCommandPool>(new VkCommandPool,
			[&](VkCommandPool* pool)
			{
				vkDestroyCommandPool(m_device.vkDevice(), *pool, nullptr);
				delete pool;
			});

		VkCommandPoolCreateInfo poolInfo{ .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO } };
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamily;

		vkCreateCommandPool(m_device.vkDevice(), &poolInfo, nullptr, m_pool.get());

		allocateCmdBuffer(*m_pool);
	}

	CmdBuffer::CmdBuffer(const Device& device, const CmdBuffer& buffer)
		: m_device{ device }
		, m_queueFamily{ buffer.m_queueFamily }
		, m_pool{ buffer.m_pool }
	{
		allocateCmdBuffer(*m_pool);
	}

	void CmdBuffer::allocateCmdBuffer(VkCommandPool cmdPool)
	{
		VkCommandBufferAllocateInfo allocInfo{ .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO } };
		allocInfo.commandPool = cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(m_device.vkDevice(), &allocInfo, &m_buffer);
	}

}