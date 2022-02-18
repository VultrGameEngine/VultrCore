#include "resource_system.h"
#include "render_system.h"
#include <vultr.h>
#include <filesystem/filestream.h>

namespace Vultr
{
	namespace ResourceSystem
	{
		static Component *component(void *component) { return static_cast<Component *>(component); }
		Component *init(RenderSystem::Component *render_system, const Path &resource_dir, const Path &build_path)
		{
			auto *c           = v_alloc<Component>();
			c->upload_context = Platform::init_upload_context(engine()->context);
			c->resource_dir   = resource_dir;

			Buffer buf;
			fread_all(build_path / "shaders/basic_vert.spv", &buf);
			CHECK_UNWRAP(auto *example_vert, Platform::try_load_shader(engine()->context, buf, Platform::ShaderType::VERT));

			buf.clear();
			fread_all(build_path / "shaders/basic_frag.spv", &buf);
			CHECK_UNWRAP(auto *example_frag, Platform::try_load_shader(engine()->context, buf, Platform::ShaderType::FRAG));

			Platform::GraphicsPipelineInfo info{
				.vert               = example_vert,
				.frag               = example_frag,
				.descriptor_layouts = Vector({render_system->camera_layout, render_system->material_layout}),
			};

			c->pipeline = Platform::init_pipeline(engine()->context, info);

			Platform::destroy_shader(engine()->context, example_vert);
			Platform::destroy_shader(engine()->context, example_frag);

			auto signature = signature_from_components<Mesh>();
			register_system(c, signature, entity_created, entity_destroyed);
			return c;
		}
		void entity_created(void *system, Entity entity)
		{
			auto *c   = component(system);
			auto mesh = get_component<Mesh>(entity);

			// TODO(Brandon): Handle this.
			ASSERT(mesh.source.has_value(), "Optional mesh source paths are not yet implemented!");

			CHECK_UNWRAP(auto *loaded_mesh, Platform::load_mesh_file(c->upload_context, c->resource_dir / mesh.source.value()));
			c->loaded_meshes.set(Traits<StringView>::hash(StringView(mesh.source.value_or(Path()).m_path)), loaded_mesh);
		}
		void entity_destroyed(void *system, Entity entity) {}
		void update(Component *system) {}
		void destroy(Component *c)
		{
			for (auto [hash, platform_mesh] : c->loaded_meshes)
			{
				Platform::destroy_mesh(c->upload_context, platform_mesh);
			}
			Platform::destroy_pipeline(engine()->context, c->pipeline);
			Platform::destroy_upload_context(c->upload_context);
			v_free(c);
		}
	} // namespace ResourceSystem
} // namespace Vultr
