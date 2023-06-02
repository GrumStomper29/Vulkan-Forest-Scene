#pragma once

#include "cmd_buffer.hpp"

#include "volk/volk.h"

namespace Graphics
{
	
	void prepareImageForColorAttachmentOutput(VkImage image, const CmdBuffer& cmdBuffer);
	
	void prepareImageForPresentation(VkImage image, const CmdBuffer& cmdBuffer);

}