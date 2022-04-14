#include "windows.h"
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo/ImGuizmo.h>
#include <math/decompose_transform.h>

namespace Vultr
{
	void scene_window_update(EditorWindowState *state, f64 dt)
	{
		if (Platform::mouse_down(engine()->window, Platform::Input::MouseButton::MOUSE_RIGHT))
		{
			Platform::lock_cursor(engine()->window);

			static constexpr f32 speed = 2;
			static constexpr f32 sens  = 100000;
			f32 delta                  = speed * (f32)dt;
			auto &transform            = state->editor_camera_transform;

			if (Platform::key_down(engine()->window, Platform::Input::KEY_W))
				transform.position += forward(transform) * delta;
			if (Platform::key_down(engine()->window, Platform::Input::KEY_S))
				transform.position -= forward(transform) * delta;
			if (Platform::key_down(engine()->window, Platform::Input::KEY_D))
				transform.position += right(transform) * delta;
			if (Platform::key_down(engine()->window, Platform::Input::KEY_A))
				transform.position -= right(transform) * delta;
			if (Platform::key_down(engine()->window, Platform::Input::KEY_E))
				transform.position += Vec3(0, 1, 0) * delta;
			if (Platform::key_down(engine()->window, Platform::Input::KEY_Q))
				transform.position -= Vec3(0, 1, 0) * delta;

			auto mouse_delta    = Platform::get_mouse_delta(engine()->window);

			f64 aspect_ratio    = (f64)Platform::get_window_width(engine()->window) / (f64)Platform::get_window_width(engine()->window);
			Quat rotation_horiz = glm::angleAxis(f32(sens * dt * -mouse_delta.x * aspect_ratio), Vec3(0, 1, 0));
			Quat rotation_vert  = glm::angleAxis(f32(sens * dt * -mouse_delta.y), right(transform));
			transform.rotation  = rotation_horiz * rotation_vert * transform.rotation;
		}
		else
		{
			Platform::unlock_cursor(engine()->window);
		}
	}

	void scene_window_draw(EditorWindowState *state, EditorRuntime *runtime)
	{
		ImGui::Begin("Game");
		ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
		auto output_texture        = Platform::imgui_get_texture_id(Platform::get_attachment_texture(runtime->render_system->output_framebuffer, 0));
		ImGui::Image(output_texture, viewport_panel_size);

		if (Platform::mouse_down(engine()->window, Platform::Input::MOUSE_RIGHT))
		{
			if (Platform::key_down(engine()->window, Platform::Input::KEY_Q))
			{
				state->current_operation = ImGuizmo::OPERATION::TRANSLATE;
			}
			else if (Platform::key_down(engine()->window, Platform::Input::KEY_W))
			{
				state->current_operation = ImGuizmo::OPERATION::ROTATE;
			}
			else if (Platform::key_down(engine()->window, Platform::Input::KEY_E))
			{
				state->current_operation = ImGuizmo::OPERATION::SCALE;
			}
		}

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();

		f32 window_width  = (f32)ImGui::GetWindowWidth();
		f32 window_height = (f32)ImGui::GetWindowHeight();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, window_width, window_height);

		if (state->selected_entity.has_value())
		{
			auto &transform    = get_component<Transform>(state->selected_entity.value());
			auto transform_mat = model_matrix(transform);
			auto view_mat      = view_matrix(state->editor_camera_transform);
			auto camera_proj   = projection_matrix(state->editor_camera, window_width, window_height);

			ImGuizmo::Manipulate(glm::value_ptr(view_mat), glm::value_ptr(camera_proj), (ImGuizmo::OPERATION)state->current_operation, ImGuizmo::LOCAL, glm::value_ptr(transform_mat), nullptr, nullptr);

			if (ImGuizmo::IsUsing())
			{
				Vec3 translation;
				Vec3 rotation;
				Vec3 scale;
				Math::decompose_transform(transform_mat, translation, rotation, scale);

				Vec3 deltaRotation = rotation - glm::eulerAngles(transform.rotation);
				transform.position = translation;
				transform.rotation = Quat(rotation);
				transform.scale    = scale;
			}
		}

		ImGui::End();
	}
	template <typename T>
	static String serialize_bytes(const byte *src, u32 width)
	{
		String res{};
		for (u32 i = 0; i < width; i++)
		{
			T val = *reinterpret_cast<const T *>(src + sizeof(T) * i);
			if constexpr (is_same<T, f32> || is_same<T, f64>)
			{
				res += serialize_f64(val);
			}
			else if (is_same<T, u8> || is_same<T, u16> || is_same<T, u32> || is_same<T, u64>)
			{
				res += serialize_u64(val);
			}
			else if (is_same<T, s8> || is_same<T, s16> || is_same<T, s32> || is_same<T, s64>)
			{
				res += serialize_s64(val);
			}
			if (i != width - 1)
				res += ",";
		}
		return res;
	}

	static String serialize_member(const byte *uniform_data, const Platform::UniformMember &member)
	{
		auto offset     = member.offset;
		const byte *src = uniform_data + offset;
		switch (member.type)
		{
			case Platform::UniformType::Vec2:
				return serialize_bytes<f32>(src, 2);
			case Platform::UniformType::Vec3:
				return serialize_bytes<f32>(src, 3);
			case Platform::UniformType::Vec4:
				return serialize_bytes<f32>(src, 4);
			case Platform::UniformType::Mat3:
				return serialize_bytes<f32>(src, 3 * 3);
			case Platform::UniformType::Mat4:
				return serialize_bytes<f32>(src, 4 * 4);
			case Platform::UniformType::f32:
				return serialize_bytes<f32>(src, 1);
			case Platform::UniformType::f64:
				return serialize_bytes<f64>(src, 1);
			case Platform::UniformType::s8:
				return serialize_bytes<s8>(src, 1);
			case Platform::UniformType::s16:
				return serialize_bytes<s16>(src, 1);
			case Platform::UniformType::s32:
				return serialize_bytes<s32>(src, 1);
			case Platform::UniformType::s64:
				return serialize_bytes<s64>(src, 1);
			case Platform::UniformType::u8:
				return serialize_bytes<u8>(src, 1);
			case Platform::UniformType::u16:
				return serialize_bytes<u16>(src, 1);
			case Platform::UniformType::u32:
				return serialize_bytes<u32>(src, 1);
			case Platform::UniformType::u64:
				return serialize_bytes<u64>(src, 1);
		}
		return {};
	}

	static ErrorOr<void> serialize_material(const Path &editor_res_path, const Resource<Platform::Material *> &material)
	{
		auto *mat_allocator     = resource_allocator<Platform::Material *>();
		auto *shader_allocator  = resource_allocator<Platform::Shader *>();
		auto *texture_allocator = resource_allocator<Platform::Texture *>();
		TRY_UNWRAP(auto *mat, material.try_value());
		TRY_UNWRAP(auto *shader, mat->source.try_value());
		auto shader_path = shader_allocator->get_resource_path(ResourceId(mat->source).id);

		String out_buf{};
		out_buf += shader_path.string();

		auto *reflection = Platform::get_reflection_data(shader);
		for (auto &uniform_member : reflection->uniform_members)
		{
			out_buf += "\n" + uniform_member.name + ":" + serialize_member(mat->uniform_data, uniform_member);
		}

		u32 i = 0;
		for (auto &sampler : reflection->samplers)
		{
			auto sampler_path = texture_allocator->get_resource_path(ResourceId(mat->samplers[i]).id);

			out_buf += "\n" + sampler.name + ":" + sampler_path.string();
			i++;
		}

		auto mat_path = mat_allocator->get_resource_path(ResourceId(material).id);
		TRY(try_fwrite_all(editor_res_path / mat_path, out_buf, StreamWriteMode::OVERWRITE));

		return Success;
	}

	static ErrorOr<void> serialize_editor_buffer(const Path &editor_res_path, const ResourceId &id, EditorBuffer *buf)
	{
		switch (buf->editor_buffer_type)
		{
			case EditorBufferType::MATERIAL:
				return serialize_material(editor_res_path, Resource<Platform::Material *>(id));
		}
		return Success;
	}

	void entity_hierarchy_window_draw(Project *project, EditorWindowState *state)
	{
		ImGui::Begin("Hierarchy");
		for (Entity entity = 1; entity < MAX_ENTITIES; entity++)
		{
			if (!entity_exists(entity))
				continue;

			if (ImGui::Selectable("Entity " + String(entity), state->selected_entity == entity))
			{
				if (state->selected_entity.has_value() && has_component<Material>(state->selected_entity.value()))
				{
					auto &mat_component = get_component<Material>(state->selected_entity.value());
					if (state->open_editor_buffers.contains(mat_component.source))
					{
						if (mat_component.source.loaded())
						{
							auto &buf = state->open_editor_buffers.get(mat_component.source);
							auto res  = serialize_material(project->resource_dir, mat_component.source);
							if (res.is_error())
								fprintf(stderr, "Something went wrong saving material %s", res.get_error().message.c_str());
						}
						state->open_editor_buffers.remove(mat_component.source);
					}
				}
				state->selected_entity = entity;
				if (has_component<Material>(entity))
				{
					auto &mat_component = get_component<Material>(state->selected_entity.value());
					state->open_editor_buffers.set(mat_component.source, {EditorBufferType::MATERIAL});
				}
			}
		}
		ImGui::End();
	}
	void component_inspector_window_draw(EditorWindowState *state)
	{
		ImGui::Begin("Inspector");
		if (state->selected_entity)
		{
			auto info = world()->component_manager.get_component_information(state->selected_entity.value());
			for (auto [component_name, members] : info)
			{
				if (ImGui::CollapsingHeader(component_name))
				{
					ImGui::PushID(component_name);
					for (auto member : members)
					{
						switch (member.type)
						{
							case PrimitiveType::U8:
								ImGui::DragScalar(member.name, ImGuiDataType_U8, member.addr, 1);
								break;
							case PrimitiveType::U16:
								ImGui::DragScalar(member.name, ImGuiDataType_U16, member.addr, 1);
								break;
							case PrimitiveType::U32:
								ImGui::DragScalar(member.name, ImGuiDataType_U32, member.addr, 1);
								break;
							case PrimitiveType::U64:
								ImGui::DragScalar(member.name, ImGuiDataType_U64, member.addr, 1);
								break;
							case PrimitiveType::S8:
								ImGui::DragScalar(member.name, ImGuiDataType_S8, member.addr, 1);
								break;
							case PrimitiveType::S16:
								ImGui::DragScalar(member.name, ImGuiDataType_S16, member.addr, 1);
								break;
							case PrimitiveType::S32:
								ImGui::DragScalar(member.name, ImGuiDataType_S32, member.addr, 1);
								break;
							case PrimitiveType::S64:
								ImGui::DragScalar(member.name, ImGuiDataType_S64, member.addr, 1);
								break;
							case PrimitiveType::F32:
								ImGui::DragScalar(member.name, ImGuiDataType_Float, member.addr, 0.02f);
								break;
							case PrimitiveType::F64:
								ImGui::DragScalar(member.name, ImGuiDataType_Double, member.addr, 0.02f);
								break;
							case PrimitiveType::CHAR:
								ImGui::Text("%s Char %c", member.name.c_str(), *static_cast<char *>(member.addr));
								break;
							case PrimitiveType::BYTE:
								ImGui::Text("%s Byte %u", member.name.c_str(), *static_cast<byte *>(member.addr));
								break;
							case PrimitiveType::BOOL:
								ImGui::Checkbox(member.name, static_cast<bool *>(member.addr));
								break;
							case PrimitiveType::STRING_VIEW:
								ImGui::Text("%s String %s", member.name.c_str(), static_cast<StringView *>(member.addr)->c_str());
								break;
							case PrimitiveType::VOID_PTR:
								ImGui::Text("%s void * %p", member.name.c_str(), member.addr);
								break;
							case PrimitiveType::VEC2:
							{
								Vec2 *vec2 = static_cast<Vec2 *>(member.addr);
								ImGui::Text("%s", member.name.c_str());
								ImGui::SameLine();

								ImGui::PushID((member.name + ".x").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec2->x, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".y").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec2->y, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();
								break;
							}
							case PrimitiveType::VEC3:
							{
								Vec3 *vec3 = static_cast<Vec3 *>(member.addr);
								ImGui::PushID((member.name + ".x").c_str());
								ImGui::Text("%s", member.name.c_str());
								ImGui::SameLine();
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec3->x, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".y").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec3->y, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".z").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec3->z, 0.02f);
								ImGui::PopID();
								break;
							}
							case PrimitiveType::VEC4:
							{
								Vec4 *vec4 = static_cast<Vec4 *>(member.addr);
								ImGui::PushID((member.name + ".x").c_str());
								ImGui::Text("%s", member.name.c_str());
								ImGui::SameLine();
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec4->x, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".y").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec4->y, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".z").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec4->z, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".w").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &vec4->w, 0.02f);
								ImGui::PopID();
								break;
							}
							case PrimitiveType::COLOR:
							{
								f32 *val = glm::value_ptr(*static_cast<Vec4 *>(member.addr));
								ImGui::ColorEdit4(member.name, val);
								break;
							}
							case PrimitiveType::QUAT:
							{
								Quat *quat = static_cast<Quat *>(member.addr);
								ImGui::PushID((member.name + ".x").c_str());
								ImGui::Text("%s", member.name.c_str());
								ImGui::SameLine();
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &quat->x, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".y").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &quat->y, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".z").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &quat->z, 0.02f);
								ImGui::SameLine();
								ImGui::PopID();

								ImGui::PushID((member.name + ".w").c_str());
								ImGui::SetNextItemWidth(150);
								ImGui::DragFloat("", &quat->w, 0.02f);
								ImGui::PopID();
								break;
							}
							case PrimitiveType::PATH:
								break;
							case PrimitiveType::OPTIONAL_PATH:
								ImGui::Text("OPTIONAL_PATH");
								break;
							case PrimitiveType::OTHER:
								break;
							case PrimitiveType::STRING:
								break;
							case PrimitiveType::RESOURCE:
								ImGui::Text("RESOURCE");
								break;
						}
					}
					if (ImGui::Button("Remove"))
					{
					}

					ImGui::PopID();
				}
			}
		}
		ImGui::End();
	}

	void update_windows(EditorWindowState *state, f64 dt) { scene_window_update(state, dt); }

	void render_windows(Platform::CmdBuffer *cmd, Project *project, EditorWindowState *state, EditorRuntime *runtime, f64 dt)
	{
		Platform::imgui_begin_frame(cmd, runtime->imgui_c);

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
										ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("VultrDockspace", &state->dockspace_open, window_flags);
		ImGui::PopStyleVar(3);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		auto dockspace = ImGui::GetID("HUB_DockSpace");
		ImGui::DockSpace(dockspace, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoResize);

		ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
		scene_window_draw(state, runtime);

		ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
		entity_hierarchy_window_draw(project, state);

		ImGui::SetNextWindowDockID(dockspace, ImGuiCond_FirstUseEver);
		component_inspector_window_draw(state);

		ImGui::End();

		Platform::imgui_end_frame(cmd, runtime->imgui_c);
	}
} // namespace Vultr