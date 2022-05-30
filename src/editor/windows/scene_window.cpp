#include "windows.h"
#include <vultr_input.h>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ImGuizmo/ImGuizmo.h>
#include <math/decompose_transform.h>

namespace Vultr
{
	void scene_window_update(EditorWindowState *state, f64 dt)
	{
		if (state->playing)
			return;
		if (Input::mouse_down(Input::MOUSE_RIGHT))
		{
			Input::lock_mouse();

			static constexpr f32 speed = 2;
			static constexpr f32 sens  = 70;
			f32 delta                  = speed * (f32)dt;
			auto &transform            = state->editor_camera_transform;

			if (Input::key_down(Input::KEY_W))
				transform.position += forward(transform) * delta;
			if (Input::key_down(Input::KEY_S))
				transform.position -= forward(transform) * delta;
			if (Input::key_down(Input::KEY_D))
				transform.position += right(transform) * delta;
			if (Input::key_down(Input::KEY_A))
				transform.position -= right(transform) * delta;
			if (Input::key_down(Input::KEY_E))
				transform.position += Vec3(0, 1, 0) * delta;
			if (Input::key_down(Input::KEY_Q))
				transform.position -= Vec3(0, 1, 0) * delta;

			auto mouse_delta    = Input::mouse_delta();

			Quat rotation_horiz = glm::angleAxis(f32(sens * dt * -mouse_delta.x), Vec3(0, 1, 0));
			Quat rotation_vert  = glm::angleAxis(f32(sens * dt * -mouse_delta.y), right(transform));
			transform.rotation  = rotation_horiz * rotation_vert * transform.rotation;
		}
		else
		{
			Input::unlock_mouse();
		}
	}

	static void hot_reload_game_thread(EditorWindowState *state, Project *project, ProgressState *progress_state)
	{
		state->hot_reload_fence.acquire();
		progress_state->message  = "Hot-reloading gameplay DLL, please wait...";
		progress_state->total    = 100;
		progress_state->progress = 0;
		if let (auto scene, state->scene_path)
		{
			if (!serialize_current_scene(state))
				return;

			auto res = reload_game(project);
			end_progress_bar(state, progress_state);
			if (res)
			{
				world()->component_manager.destroy_component_arrays();
				world()->entity_manager = EntityManager();
				world()->component_manager.deregister_non_system_components();
				project->register_components();
				load_scene(state, state->scene_path.value());
			}
			else
			{
				display_error(state, "Hot Reload Gameplay DLL Failed", String(res.get_error().message));
			}
		}
		else
		{
			display_error(state, "No Scene Open", String("Cannot hot-reload gameplay DLL without an open scene. Please open a scene."));
		}
		state->hot_reload_fence.release();
	}

	static void hot_reload_game(EditorWindowState *state, Project *project)
	{
		auto *progress_state = begin_progress_bar(state, "Hot-reloading game");
		Platform::Thread thread(hot_reload_game_thread, state, project, progress_state);
		thread.detach();
	}

	static void play_game(EditorWindowState *state, Project *project)
	{
		ASSERT(!state->playing, "Cannot play game that is already playing!");
		if (!state->started)
		{
			if let (auto scene, state->scene_path)
			{
				if (!serialize_current_scene(state))
					return;

				if (!state->started)
				{
					state->game_memory = project->init();
				}
			}
			else
			{
				display_error(state, "No Scene Open", String("Cannot hot-reload gameplay DLL without an open scene. Please open a scene."));
			}
		}
		state->started = true;
		state->playing = true;
	}

	static void pause_game(EditorWindowState *state, Project *project)
	{
		ASSERT(state->started, "Cannot pause game that has not been started!");
		ASSERT(state->playing, "Cannot pause game that is already paused!");
		state->playing = false;
	}

	static void stop_game(EditorWindowState *state, Project *project)
	{
		ASSERT(state->started, "Cannot stop game that has not been started!");

		project->destroy(state->game_memory);
		load_scene(state, state->scene_path.value());

		state->playing = false;
		state->started = false;
	}

	void scene_window_draw(RenderSystem::Component *render_system, Project *project, EditorWindowState *state, EditorRuntime *runtime)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Game");
		if (state->hot_reload_fence.try_acquire())
		{
			if (ImGui::IsWindowHovered() && !Input::mouse_down(Input::MOUSE_RIGHT))
			{
				if (Input::key_down(Input::KEY_Q))
				{
					state->current_operation = ImGuizmo::OPERATION::TRANSLATE;
				}
				else if (Input::key_down(Input::KEY_W))
				{
					state->current_operation = ImGuizmo::OPERATION::ROTATE;
				}
				else if (Input::key_down(Input::KEY_E))
				{
					state->current_operation = ImGuizmo::OPERATION::SCALE;
				}
			}

			{
				auto alignment = ImVec2(0.5f, 0.5f);
				ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, alignment);

				static constexpr ImVec2 button_size         = ImVec2(80, 80);
				static constexpr ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;

				auto disabled_flag                          = [](bool expr) { return expr ? 0 : ImGuiSelectableFlags_Disabled; };

				if (ImGui::Selectable("Play", false, flags | disabled_flag(!state->playing || !state->started), button_size))
					play_game(state, project);

				ImGui::SameLine();
				if (ImGui::Selectable("Pause", false, flags | disabled_flag(state->started && state->playing), button_size))
					pause_game(state, project);

				ImGui::SameLine();
				if (ImGui::Selectable("Stop", false, flags | disabled_flag(state->started), button_size))
					stop_game(state, project);

				ImGui::SameLine();
				if (ImGui::Selectable("Reload", false, flags | disabled_flag(!state->started && !state->playing), button_size))
					hot_reload_game(state, project);

				ImGui::PopStyleVar();
			}

			ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
			auto output_texture        = Platform::imgui_get_texture_id(Platform::get_attachment_texture(runtime->render_system->output_framebuffer, 0));
			ImGui::Image(output_texture, viewport_panel_size);

			if (Platform::get_width(render_system->output_framebuffer) != viewport_panel_size.x || Platform::get_height(render_system->output_framebuffer) != viewport_panel_size.y)
			{
				RenderSystem::request_resize(render_system, max(viewport_panel_size.x, 1.0f), max(viewport_panel_size.y, 1.0f));
			}

			if (ImGui::BeginDragDropTarget())
			{
				const auto *payload = ImGui::AcceptDragDropPayload(resource_type_to_string(ResourceType::SCENE));

				if (payload != nullptr)
				{
					UUID asset_uuid;
					memcpy(asset_uuid.m_uuid, payload->Data, payload->DataSize);

					if let (auto file, project->asset_map.try_get(asset_uuid))
					{
						load_scene(state, file);
					}
					else
					{
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImVec2 top_left             = ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + (ImGui::GetWindowHeight() - viewport_panel_size.y));

			state->render_window_offset = Vec2(top_left.x, top_left.y);
			state->render_window_size   = Vec2(viewport_panel_size.x, viewport_panel_size.y);

			if (!state->playing)
			{
				ImVec2 bottom_left = ImVec2(top_left.x + viewport_panel_size.x, top_left.y + viewport_panel_size.y);
				ImGui::PushClipRect(top_left, bottom_left, true);

				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist();

				ImGuizmo::SetRect(top_left.x, top_left.y, viewport_panel_size.x, viewport_panel_size.y);

				if (state->selected_entity.has_value())
				{
					auto ent = state->selected_entity.value();
					if (has_component<Transform>(ent))
					{
						auto transform_mat = get_world_transform(ent);
						auto view_mat      = view_matrix(state->editor_camera_transform);
						auto camera_proj   = projection_matrix(state->editor_camera, viewport_panel_size.x, viewport_panel_size.y);

						bool snap          = Input::key_down(Input::KEY_SHIFT);
						f32 snap_value     = state->current_operation != ImGuizmo::OPERATION::ROTATE ? 0.5f : 45.0f;
						f32 snap_values[]  = {snap_value, snap_value, snap_value};

						ImGuizmo::Manipulate(glm::value_ptr(view_mat), glm::value_ptr(camera_proj), (ImGuizmo::OPERATION)state->current_operation, ImGuizmo::LOCAL, glm::value_ptr(transform_mat), nullptr,
											 snap ? snap_values : nullptr);

						if (ImGuizmo::IsUsing())
						{
							auto &transform = get_component<Transform>(ent);
							Math::decompose_transform(get_local_transform(transform_mat, ent), &transform.position, &transform.rotation, &transform.scale);
						}
					}
				}
				ImGui::PopClipRect();
			}
			state->hot_reload_fence.release();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
} // namespace Vultr
