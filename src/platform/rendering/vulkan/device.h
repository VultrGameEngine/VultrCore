#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "constants.h"
#include <types/vector.h>
#include "../../platform_impl.h"
#include <vma/vk_mem_alloc.h>

namespace Vultr
{
	namespace Vulkan
	{
		struct Device
		{
			VkInstance instance              = nullptr;
			VkPhysicalDevice physical_device = nullptr;
			VkPhysicalDeviceProperties properties{};

			VkDevice device        = nullptr;
			VkSurfaceKHR surface   = nullptr;

			VkQueue graphics_queue = nullptr;
			Platform::Mutex graphics_queue_mutex{};
			VkQueue present_queue                    = nullptr;

			VkDebugUtilsMessengerEXT debug_messenger = nullptr;

			VmaAllocator allocator                   = nullptr;

			Device()                                 = default;
			Device(const Device &other)
				: instance(other.instance), physical_device(other.physical_device), properties(other.properties), device(other.device), surface(other.surface), graphics_queue(other.graphics_queue),
				  present_queue(other.present_queue), debug_messenger(other.debug_messenger), allocator(other.allocator)
			{
			}
			Device &operator=(const Device &other)
			{
				instance        = other.instance;
				physical_device = other.physical_device;
				properties      = other.properties;
				device          = other.device;
				surface         = other.surface;
				graphics_queue  = other.graphics_queue;
				present_queue   = other.present_queue;
				debug_messenger = other.debug_messenger;
				allocator       = other.allocator;
				return *this;
			}
		};

		Device init_device(const Platform::Window *window, bool debug, PFN_vkDebugUtilsMessengerCallbackEXT debug_cb);
		VkFormat get_supported_depth_format(Device *d);
		void destroy_device(Device *d);

		void graphics_queue_submit(Device *d, u32 submit_count, const VkSubmitInfo *p_submits, VkFence fence);

		// void map_memory(Device *d, void *)

		struct QueueFamilyIndices
		{
			Option<u32> graphics_family = None;
			Option<u32> present_family  = None;
		};

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			Vector<VkSurfaceFormatKHR> formats;
			Vector<VkPresentModeKHR> present_modes;
		};

		QueueFamilyIndices find_queue_families(Device *d);
		SwapChainSupportDetails query_swap_chain_support(Device *d);
		size_t min_ubo_alignment(Device *d);

		void wait_idle(Device *d);

	} // namespace Vulkan
} // namespace Vultr
