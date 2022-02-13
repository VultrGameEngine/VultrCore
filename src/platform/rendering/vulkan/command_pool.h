#pragma once
#include <types/vector.h>
#include <types/error_or.h>
#include <vulkan/vulkan.h>

namespace Vultr
{
	namespace Vulkan
	{
		struct CommandPool
		{
			VkCommandPool command_pool = nullptr;
			Vector<VkCommandBuffer> command_buffers{};
			u32 index      = 0;
			bool recording = false;
		};

		struct Device;
		CommandPool init_cmd_pool(Device *d);
		VkCommandBuffer begin_cmd_buffer(Device *d, CommandPool *cmd_pool);
		void end_cmd_buffer(CommandPool *cmd_pool);
		void recycle_cmd_pool(Device *d, CommandPool *cmd_pool);
		void destroy_cmd_pool(Device *d, CommandPool *cmd_pool);

	} // namespace Vulkan

	namespace Platform
	{
		struct RenderContext;
		struct CmdBuffer
		{
			u32 image_index               = 0;
			Vulkan::CommandPool *cmd_pool = nullptr;
			VkCommandBuffer cmd_buffer    = nullptr;
			RenderContext *render_context = nullptr;
		};
	} // namespace Platform

} // namespace Vultr
