#include <platform/platform.h>
#include <vultr.h>
#include "project/project.h"
#include <core/systems/render_system.h>
#include <filesystem/filestream.h>

int Vultr::vultr_main(Platform::EntryArgs *args)
{
	Vultr::init();
	int return_code = 0;
	if check (pwd(), auto cwd, auto cwd_err)
	{
		auto resource_dir = cwd / "res/";
		auto build_dir    = cwd / "build/";
		if (!exists(resource_dir))
			THROW("Resource directory does not exist!");

		if (!exists(build_dir))
			THROW("Build directory does not exist!");

		auto dll = build_dir / VULTR_GAMEPLAY_NAME;
		printf("Opening %s\n", dll.m_path.c_str());

		if check (Vultr::load_game(dll), auto project, auto err)
		{
			Vultr::open_borderless_windowed("Vultr Game Engine");

			project.init();

			auto *upload_context = Platform::init_upload_context(engine()->context);
			CHECK_UNWRAP(auto *example_mesh, Platform::load_mesh_file(upload_context, resource_dir / "cube.fbx"));

			Buffer buf;
			fread_all(build_dir / "shaders/basic_vert.spv", &buf);
			CHECK_UNWRAP(auto *example_vert, Platform::try_load_shader(engine()->context, buf, Platform::ShaderType::VERT));

			buf.clear();
			fread_all(build_dir / "shaders/basic_frag.spv", &buf);
			CHECK_UNWRAP(auto *example_frag, Platform::try_load_shader(engine()->context, buf, Platform::ShaderType::FRAG));

			auto *render_system = RenderSystem::init();
			while (!Platform::window_should_close(engine()->window))
			{
				Platform::poll_events(engine()->window);
				auto dt = Platform::update_window(engine()->window);
				RenderSystem::update(render_system);
				project.update();
			}

			Platform::destroy_shader(engine()->context, example_vert);
			Platform::destroy_shader(engine()->context, example_frag);
			Platform::destroy_mesh(upload_context, example_mesh);
			Platform::destroy_upload_context(upload_context);
			Platform::close_window(engine()->window);
		}
		else
		{
			fprintf(stderr, "Failed to load project file: %s\n", (str)err.message);
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
