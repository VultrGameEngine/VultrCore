#pragma once
#include <types/vector.h>
#include <types/error_or.h>
#include <vulkan/vulkan.h>

namespace Vultr
{
	namespace Platform
	{
		struct CmdBuffer;
	}

	namespace Vulkan
	{
		struct CommandPool
		{
			VkCommandPool command_pool = nullptr;
			Vector<VkCommandBuffer> command_buffers{};
			VkFence fence  = nullptr;
			u32 index      = 0;
			bool recording = false;
		};

		enum struct QueueType
		{
			GRAPHICS,
			COMPUTE,
		};

		struct Device;
		CommandPool init_cmd_pool(Device *d, QueueType queue_type);
		VkCommandBuffer begin_cmd_buffer(Device *d, CommandPool *cmd_pool);
		void end_cmd_buffer(VkCommandBuffer cmd, CommandPool *cmd_pool);
		void recycle_cmd_pool(Device *d, CommandPool *cmd_pool);
		void destroy_cmd_pool(Device *d, CommandPool *cmd_pool);
		struct Frame;
		void depend_resource(Platform::CmdBuffer *cmd, void *resource);
	} // namespace Vulkan

	namespace Platform
	{
		struct RenderContext;
		struct Framebuffer;
		struct CmdBuffer
		{
			Vulkan::Frame *frame                      = nullptr;
			VkFramebuffer out_framebuffer             = nullptr;
			u32 image_index                           = 0;
			u32 frame_index                           = 0;
			VkCommandBuffer cmd_buffer                = nullptr;
			RenderContext *render_context             = nullptr;
			Option<Framebuffer *> current_framebuffer = None;
		};
	} // namespace Platform

} // namespace Vultr
