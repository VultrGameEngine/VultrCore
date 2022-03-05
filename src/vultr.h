#pragma once
#include "vultr_memory.h"
#include "vultr_ecs.h"
#include "vultr_engine.h"

namespace Vultr
{
	static constexpr str VULTR_GAMEPLAY_NAME = "libGameplay.so";
	void init();
	void open_window(Platform::DisplayMode display_mode, str name);
	void destroy();

	typedef void (*UseGameMemoryApi)(GameMemory *m);

	// TODO(Brandon): Update these with actual parameters.
	typedef void *(*VultrInitApi)();
	typedef void (*VultrUpdateApi)(void *);
} // namespace Vultr

VULTR_API void use_game_memory(void *m);
VULTR_API void vultr_init(void);
VULTR_API void vultr_update(void);
