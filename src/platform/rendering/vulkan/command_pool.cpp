#include "command_pool.h"
#include "swap_chain.h"
#include "render_context.h"

namespace Vultr
{
	namespace Vulkan
	{
		static void expand_cmd_pool(Device *d, CommandPool *cmd_pool)
		{
			VkCommandBufferAllocateInfo alloc_info{
				.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool        = cmd_pool->command_pool,
				.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1,
			};

			VkCommandBuffer new_cmd_buffer;
			VK_CHECK(vkAllocateCommandBuffers(d->device, &alloc_info, &new_cmd_buffer));
			cmd_pool->command_buffers.push_back(new_cmd_buffer);
		}

		CommandPool init_cmd_pool(Device *d, QueueType queue_type)
		{
			CommandPool cmd_pool{};

			u32 index;
			switch (queue_type)
			{
				case QueueType::GRAPHICS:
					index = d->queue_family_indices.graphics_family.value();
					break;
				case QueueType::COMPUTE:
					index = d->queue_family_indices.compute_family.value();
					break;
			}

			// TODO(Brandon): Maybe don't hard-code the graphics family queue.
			VkCommandPoolCreateInfo pool_info{
				.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags            = 0,
				.queueFamilyIndex = index,
			};

			VK_CHECK(vkCreateCommandPool(d->device, &pool_info, nullptr, &cmd_pool.command_pool));

			VkFenceCreateInfo fence_info{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = 0,
			};

			VK_CHECK(vkCreateFence(d->device, &fence_info, nullptr, &cmd_pool.fence));

			return cmd_pool;
		}

		VkCommandBuffer begin_cmd_buffer(Device *d, CommandPool *cmd_pool)
		{
			ASSERT(cmd_pool->index <= cmd_pool->command_buffers.size(), "Invalid command buffer index!");

			if (cmd_pool->index == cmd_pool->command_buffers.size())
			{
				expand_cmd_pool(d, cmd_pool);
			}

			auto cmd_buffer = cmd_pool->command_buffers[cmd_pool->index];

			VkCommandBufferBeginInfo begin_info{
				.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				.pInheritanceInfo = nullptr,
			};

			VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &begin_info));
			return cmd_buffer;
		}

		void end_cmd_buffer(VkCommandBuffer cmd, CommandPool *cmd_pool)
		{
			VK_CHECK(vkEndCommandBuffer(cmd));
			cmd_pool->index++;
		}

		void recycle_cmd_pool(Device *d, CommandPool *cmd_pool)
		{
			ASSERT(!cmd_pool->recording, "Cannot recycle command pool while still recording.");
			VK_CHECK(vkResetCommandPool(d->device, cmd_pool->command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
			cmd_pool->index = 0;
		}

		void destroy_cmd_pool(Device *d, CommandPool *cmd_pool)
		{
			ASSERT(cmd_pool != nullptr, "Cannot destroy nullptr command pool.");
			ASSERT(d != nullptr, "Cannot destroy with nullptr device.");

			vkDestroyFence(d->device, cmd_pool->fence, nullptr);

			vkDestroyCommandPool(d->device, cmd_pool->command_pool, nullptr);

			cmd_pool->command_pool = nullptr;
			cmd_pool->command_buffers.clear();
		}

		void depend_resource(Platform::CmdBuffer *cmd, void *resource)
		{
			auto *sc    = Vulkan::get_swapchain(cmd->render_context);
			auto *frame = cmd->frame;
			Platform::Lock lock(frame->mutex);
			if (!frame->in_use_resources.contains(resource))
				frame->in_use_resources.set<void *, Traits<void *>>(resource);
		}
	} // namespace Vulkan
} // namespace Vultr
