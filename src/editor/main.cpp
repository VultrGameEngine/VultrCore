#include <platform/platform.h>
#include <vultr.h>
#include <vultr_input.h>
#include "project/project.h"
#include "windows/windows.h"
#include "editor/runtime/runtime.h"
#include <core/systems/render_system.h>
#include <filesystem/filestream.h>
#include <vultr_resource_allocator.h>
#include <platform/rendering/vulkan/texture.h>
#include <platform/rendering/vulkan/vertex_buffer.h>
#include <platform/rendering/vulkan/index_buffer.h>

static void mesh_loader_thread(const Vultr::Project *project)
{
	auto *c         = Vultr::Platform::init_upload_context(Vultr::engine()->context);
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Mesh *>();
	while (true)
	{
		auto [uuid, is_kill_request] = allocator->wait_pop_load_queue();

		if (is_kill_request)
		{
			Vultr::Platform::destroy_upload_context(c);
			return;
		}

		auto path = Vultr::get_editor_optimized_path(project, uuid);

		Vultr::Buffer buf;
		{
			auto res = Vultr::try_fread_all(path, &buf);
			if (res.is_error())
			{
				allocator->add_loaded_resource_error(uuid, res.get_error());
				continue;
			}
		}

		auto res = Vultr::load_editor_optimized_mesh(c, buf);
		if (res.is_error())
		{
			allocator->add_loaded_resource_error(uuid, res.get_error());
			continue;
		}
		auto *mesh = res.value();
		if (allocator->add_loaded_resource(uuid, mesh).is_error())
		{
			Vultr::Platform::destroy_mesh(Vultr::engine()->context, mesh);
		}
	}
}

static void mesh_free_thread()
{
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Mesh *>();
	while (true)
	{
		auto *mesh = allocator->wait_pop_free_queue();

		if (mesh == (void *)-1)
		{
			return;
		}

		Vultr::Platform::destroy_mesh(Vultr::engine()->context, mesh);
	}
}

static void material_loader_thread(Vultr::Project *project)
{
	auto *c         = Vultr::Platform::init_upload_context(Vultr::engine()->context);
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Material *>();
	while (true)
	{
		auto [uuid, is_kill_request] = allocator->wait_pop_load_queue();

		if (is_kill_request)
		{
			Vultr::Platform::destroy_upload_context(c);
			return;
		}

		auto path = Vultr::get_editor_optimized_path(project, uuid);

		Vultr::String material_src;
		{
			auto res = Vultr::try_fread_all(path, &material_src);
			if (res.is_error())
			{
				allocator->add_loaded_resource_error(uuid, res.get_error());
				continue;
			}
		}

		auto shader_uuid = Vultr::Platform::parse_uuid(Vultr::split(material_src, "\n")[0]);
		auto shader      = Vultr::Resource<Vultr::Platform::Shader *>(shader_uuid);

		while (true)
		{
			// Need to lock so that the shader doesn't get freed from the main thread while we are trying to load the material.
			Vultr::Platform::Lock lock(project->shader_free_mutex);
			if (!shader.loaded())
				continue;

			if check (Vultr::Platform::try_load_material(c, shader, material_src), auto *mat, auto err)
			{
				if (!allocator->add_loaded_resource(uuid, mat).has_value())
				{
					Vultr::Platform::destroy_material(Vultr::engine()->context, mat);
				}
				break;
			}
			else
			{
				allocator->add_loaded_resource_error(uuid, err);
				break;
			}
		}
	}
}

static void material_free_thread()
{
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Material *>();
	while (true)
	{
		auto *mat = allocator->wait_pop_free_queue();

		if (mat == (void *)-1)
		{
			return;
		}

		Vultr::Platform::destroy_material(Vultr::engine()->context, mat);
	}
}

static void shader_loader_thread(const Vultr::Project *project)
{
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Shader *>();
	while (true)
	{

		auto [uuid, is_kill_request] = allocator->wait_pop_load_queue();

		auto path                    = Vultr::get_editor_optimized_path(project, uuid);

		Vultr::Buffer buf;
		{
			auto res = Vultr::try_fread_all(path, &buf);
			if (res.is_error())
			{
				allocator->add_loaded_resource_error(uuid, res.get_error());
				continue;
			}
		}

		auto res = Vultr::load_editor_optimized_shader(Vultr::engine()->context, buf);
		if (res.is_error())
		{
			allocator->add_loaded_resource_error(uuid, res.get_error());
			continue;
		}
		auto *shader = res.value();
		if (allocator->add_loaded_resource(uuid, shader).is_error())
		{
			Vultr::Platform::destroy_shader(Vultr::engine()->context, shader);
		}
	}
}

static void shader_free_thread()
{
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Shader *>();
	while (true)
	{
		auto *shader = allocator->wait_pop_free_queue();

		if (shader == (void *)-1)
		{
			return;
		}

		Vultr::Platform::destroy_shader(Vultr::engine()->context, shader);
	}
}

static void texture_loader_thread(const Vultr::Project *project)
{
	auto *c         = Vultr::Platform::init_upload_context(Vultr::engine()->context);
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Texture *>();
	while (true)
	{
		auto [uuid, is_kill_request] = allocator->wait_pop_load_queue();

		if (is_kill_request)
		{
			Vultr::Platform::destroy_upload_context(c);
			return;
		}

		auto path = Vultr::get_editor_optimized_path(project, uuid);

		Vultr::Buffer buf;
		{
			auto res = Vultr::try_fread_all(path, &buf);
			if (res.is_error())
			{
				allocator->add_loaded_resource_error(uuid, res.get_error());
				continue;
			}
		}

		auto res = Vultr::load_editor_optimized_texture(c, buf);
		if (res.is_error())
		{
			allocator->add_loaded_resource_error(uuid, res.get_error());
			continue;
		}
		auto *shader = res.value();
		if (allocator->add_loaded_resource(uuid, shader).is_error())
		{
			Vultr::Platform::destroy_texture(Vultr::engine()->context, shader);
		}
	}
}

static void texture_free_thread()
{
	auto *allocator = Vultr::resource_allocator<Vultr::Platform::Texture *>();
	while (true)
	{
		auto *texture = allocator->wait_pop_free_queue();

		if (texture == (void *)-1)
		{
			return;
		}

		Vultr::Platform::destroy_texture(Vultr::engine()->context, texture);
	}
}

int Vultr::vultr_main(Platform::EntryArgs *args)
{
	Vultr::init();
	int return_code = 0;
	if check (pwd(), auto cwd, auto cwd_err)
	{
		auto resource_dir = cwd / "res/";
		auto build_dir    = cwd / "build/";
		ASSERT(exists(resource_dir), "Resource directory does not exist!");
		ASSERT(exists(build_dir), "Build directory does not exist!");

		Vultr::open_windowed("Vultr Game Engine");
		Project project;
		auto res = Vultr::load_game(build_dir, resource_dir, &project);
		if (res)
		{
			Vultr::init_resource_allocators();

			Platform::Thread mesh_loading_thread(mesh_loader_thread, &project);
			mesh_loading_thread.detach();
			Platform::Thread mesh_freeing_thread(mesh_free_thread);
			mesh_freeing_thread.detach();
			Platform::Thread material_loading_thread(material_loader_thread, &project);
			material_loading_thread.detach();
			Platform::Thread material_freeing_thread(material_free_thread);
			material_freeing_thread.detach();
			Platform::Thread shader_loading_thread(shader_loader_thread, &project);
			shader_loading_thread.detach();
			Platform::Thread shader_freeing_thread(shader_free_thread);
			shader_freeing_thread.detach();
			Platform::Thread texture_loading_thread(texture_loader_thread, &project);
			texture_loading_thread.detach();
			Platform::Thread texture_freeing_thread(texture_free_thread);
			texture_freeing_thread.detach();

			EditorRuntime runtime{};
			runtime.render_system  = RenderSystem::init();
			runtime.upload_context = Vultr::Platform::init_upload_context(Vultr::engine()->context);
			runtime.imgui_c        = Platform::init_imgui(engine()->window, runtime.upload_context, EditorResources::GET_ROBOTO_TTF(), EditorResources::ROBOTO_TTF_LEN, 15);

			EditorWindowState state;
			begin_resource_import(&project, &state);

			project.register_components();

			init_windows(&runtime, &project, &state);

			while (!Platform::window_should_close(engine()->window))
			{
				reload_necessary_assets(&project);
				Platform::poll_events(engine()->window);
				Input::update_input(Input::input_manager(), state.render_window_offset, state.render_window_size);
				auto dt = Platform::update_window(engine()->window);

				if (state.hot_reload_fence.try_acquire())
				{
					if (state.playing)
						project.update(state.game_memory, dt);

					update_windows(&state, dt);
					state.hot_reload_fence.release();
				}

				if check (Platform::begin_cmd_buffer(engine()->window), auto *cmd, auto _)
				{
					if (state.hot_reload_fence.try_acquire())
					{
						if (state.playing)
						{
							RenderSystem::update(cmd, runtime.render_system);
						}
						else
						{
							RenderSystem::update(state.editor_camera, state.editor_camera_transform, cmd, runtime.render_system);
						}
						state.hot_reload_fence.release();
					}

					Platform::begin_window_framebuffer(cmd);
					render_windows(cmd, runtime.render_system, &project, &state, &runtime, dt);
					Platform::end_framebuffer(cmd);

					Platform::end_cmd_buffer(cmd);
				}
				else
				{
					RenderSystem::reinitialize(runtime.render_system);
				}
			}

			if (state.hot_reload_fence.try_acquire())
			{
				if (state.started)
					project.destroy(state.game_memory);

				world()->component_manager.deregister_non_system_components();
				state.hot_reload_fence.release();
			}

			Platform::wait_idle(engine()->context);
			resource_allocator<Platform::Mesh *>()->kill_loading_threads();
			resource_allocator<Platform::Mesh *>()->kill_freeing_threads();
			resource_allocator<Platform::Material *>()->kill_loading_threads();
			resource_allocator<Platform::Material *>()->kill_freeing_threads();
			resource_allocator<Platform::Shader *>()->kill_loading_threads();
			resource_allocator<Platform::Shader *>()->kill_freeing_threads();
			resource_allocator<Platform::Texture *>()->kill_loading_threads();
			resource_allocator<Platform::Texture *>()->kill_freeing_threads();

			destroy_windows(&state);
			RenderSystem::destroy(runtime.render_system);
			Platform::destroy_imgui(engine()->context, runtime.imgui_c);
			Platform::destroy_upload_context(runtime.upload_context);
			Platform::close_window(engine()->window);
		}
		else
		{
			fprintf(stderr, "Failed to load project file: %s\n", (str)res.get_error().message.c_str());
			return_code = 1;
		}
	}
	else
	{
		fprintf(stderr, "Failed to get current working directory: %s\n", cwd_err.message.c_str());
		return_code = 1;
	}

	Vultr::destroy();

	return return_code;
}
