#pragma once
#include "material.h"
#include "mesh.h"
#include "transform.h"
#include "camera.h"
#include "directional_light.h"
#include <vultr_resource_allocator.h>

#include <utils/traits.h>
namespace Vultr
{
	template <>
	struct ReflTraits<Material> : public GenericTraits<Material>
	{
		static consteval StringView type_name() { return "Material"; }
		static consteval u32 type_id() { return string_hash(type_name(), type_name().length()); }
	};

	template <>
	struct ComponentTraits<Material> : public ReflTraits<Material>
	{
		static Vector<ComponentMember> members(Material *component)
		{
			Vector<ComponentMember> members = Vector({
				ComponentMember{
					.name = "source",
					.type = PrimitiveType::RESOURCE,
					.addr = &component->source,
				},
			});
			if (!component->source.loaded())
				return members;

			Platform::Material *mat  = component->source.value();
			Platform::Shader *shader = mat->source.value();

			const auto *reflection   = Platform::get_reflection_data(shader);

			for (auto &uniform_member : reflection->uniform_members)
			{
				PrimitiveType primitive_type;
				switch (uniform_member.type)
				{
					case Platform::UniformType::Vec2:
						primitive_type = PrimitiveType::VEC2;
						break;
					case Platform::UniformType::Vec3:
						primitive_type = PrimitiveType::VEC3;
						break;
					case Platform::UniformType::Vec4:
						primitive_type = PrimitiveType::COLOR;
						break;
					case Platform::UniformType::Mat3:
					case Platform::UniformType::Mat4:
						THROW("Not implemented!");
					case Platform::UniformType::f32:
						primitive_type = PrimitiveType::F32;
						break;
					case Platform::UniformType::f64:
						primitive_type = PrimitiveType::F64;
						break;
					case Platform::UniformType::s8:
						primitive_type = PrimitiveType::S8;
						break;
					case Platform::UniformType::s16:
						primitive_type = PrimitiveType::S16;
						break;
					case Platform::UniformType::s32:
						primitive_type = PrimitiveType::S32;
						break;
					case Platform::UniformType::s64:
						primitive_type = PrimitiveType::S64;
						break;
					case Platform::UniformType::u8:
						primitive_type = PrimitiveType::U8;
						break;
					case Platform::UniformType::u16:
						primitive_type = PrimitiveType::U16;
						break;
					case Platform::UniformType::u32:
						primitive_type = PrimitiveType::U32;
						break;
					case Platform::UniformType::u64:
						primitive_type = PrimitiveType::U64;
						break;
				}
				members.push_back(ComponentMember{
					.name = uniform_member.name,
					.type = primitive_type,
					.addr = &mat->uniform_data[uniform_member.offset],
				});
			}

			u32 binding = 1;
			for (auto &sampler : reflection->samplers)
			{
				members.push_back(ComponentMember{
					.name = sampler.name,
					.type = PrimitiveType::RESOURCE,
					.addr = &mat->samplers[binding - 1],
				});
				binding++;
			}
			return members;
		}
		static consteval u32 component_id() { return ReflTraits<Material>::type_id(); }
	};

	template <>
	struct ReflTraits<Transform> : public GenericTraits<Transform>
	{
		static consteval StringView type_name() { return "Transform"; }
		static consteval u32 type_id() { return string_hash(type_name(), type_name().length()); }
	};

	template <>
	struct ComponentTraits<Transform> : public ReflTraits<Transform>
	{
		static Vector<ComponentMember> members(Transform *component)
		{
			return Vector({
				ComponentMember{
					.name = "position",
					.type = PrimitiveType::VEC3,
					.addr = &component->position,
				},
				ComponentMember{
					.name = "rotation",
					.type = PrimitiveType::QUAT,
					.addr = &component->rotation,
				},
				ComponentMember{
					.name = "scale",
					.type = PrimitiveType::VEC3,
					.addr = &component->scale,
				},
			});
		}
		static consteval u32 component_id() { return ReflTraits<Transform>::type_id(); }
	};

	template <>
	struct ReflTraits<Mesh> : public GenericTraits<Mesh>
	{
		static consteval StringView type_name() { return "Mesh"; }
		static consteval u32 type_id() { return string_hash(type_name(), type_name().length()); }
	};

	template <>
	struct ComponentTraits<Mesh> : public ReflTraits<Mesh>
	{
		static Vector<ComponentMember> members(Mesh *component)
		{
			return Vector({
				ComponentMember{
					.name = "source",
					.type = PrimitiveType::RESOURCE,
					.addr = &component->source,
				},
			});
		}
		static consteval u32 component_id() { return ReflTraits<Transform>::type_id(); }
	};

	template <>
	struct ReflTraits<Camera> : public GenericTraits<Camera>
	{
		static consteval StringView type_name() { return "Camera"; }
		static consteval u32 type_id() { return string_hash(type_name(), type_name().length()); }
	};

	template <>
	struct ComponentTraits<Camera> : public ReflTraits<Camera>
	{
		static Vector<ComponentMember> members(Camera *component)
		{
			return Vector({
				ComponentMember{
					.name = "enabled",
					.type = PrimitiveType::BOOL,
					.addr = &component->enabled,
				},
				ComponentMember{
					.name = "fov",
					.type = PrimitiveType::F64,
					.addr = &component->fov,
				},
				ComponentMember{
					.name = "znear",
					.type = PrimitiveType::F64,
					.addr = &component->znear,
				},
				ComponentMember{
					.name = "zfar",
					.type = PrimitiveType::F64,
					.addr = &component->zfar,
				},
				ComponentMember{
					.name = "exposure",
					.type = PrimitiveType::F64,
					.addr = &component->exposure,
				},
				ComponentMember{
					.name = "bloom_intensity",
					.type = PrimitiveType::F64,
					.addr = &component->bloom_intensity,
				},
				ComponentMember{
					.name = "bloom_threshold",
					.type = PrimitiveType::F64,
					.addr = &component->bloom_threshold,
				},
				ComponentMember{
					.name = "bloom_quality",
					.type = PrimitiveType::F64,
					.addr = &component->bloom_quality,
				},
				ComponentMember{
					.name = "gamma_correction",
					.type = PrimitiveType::BOOL,
					.addr = &component->gamma_correction,
				},
			});
		}
		static consteval u32 component_id() { return ReflTraits<Transform>::type_id(); }
	};

	template <>
	struct ReflTraits<DirectionalLight> : public GenericTraits<DirectionalLight>
	{
		static consteval StringView type_name() { return "DirectionalLight"; }
		static consteval u32 type_id() { return string_hash(type_name(), type_name().length()); }
	};

	template <>
	struct ComponentTraits<DirectionalLight> : public ReflTraits<DirectionalLight>
	{
		static Vector<ComponentMember> members(DirectionalLight *component)
		{
			return Vector({
				ComponentMember{
					.name = "ambient",
					.type = PrimitiveType::COLOR,
					.addr = &component->ambient,
				},
				ComponentMember{
					.name = "diffuse",
					.type = PrimitiveType::COLOR,
					.addr = &component->diffuse,
				},
				ComponentMember{
					.name = "specular",
					.type = PrimitiveType::F32,
					.addr = &component->specular,
				},
				ComponentMember{
					.name = "intensity",
					.type = PrimitiveType::F32,
					.addr = &component->intensity,
				},
			});
		}
		static consteval u32 component_id() { return ReflTraits<Transform>::type_id(); }
	};
} // namespace Vultr
