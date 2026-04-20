#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindRenderingComponents(sol::state& state)
	{
		state.new_usertype<SpriteComponent>("SpriteRendererComponent",
			"Color", &SpriteComponent::Color,
			"TextureHandle", &SpriteComponent::TextureHandle
		);

		state.new_usertype<StaticMeshComponent>("StaticMeshComponent",
			"MeshHandle", &StaticMeshComponent::MeshHandle
		);

		state.new_usertype<SkinnedMeshComponent>("SkinnedMeshComponent",
			"MeshHandle", &SkinnedMeshComponent::MeshHandle,
			"AnimatorEntityHandle", &SkinnedMeshComponent::AnimatorEntityHandle
		);

		state.new_usertype<MaterialComponent>("MaterialComponent",
			"MaterialHandle", &MaterialComponent::MaterialHandle,
			"GetInstanced", &MaterialComponent::GetInstanced,
			"CloneMaterial", &MaterialComponent::CloneMaterial
		);

		state.new_usertype<OutlineComponent>("OutlineComponent",
			"Color", &OutlineComponent::Color,
			"Thickness", &OutlineComponent::Thickness
		);

		state.new_usertype<BillboardComponent>("BillboardComponent",
			"Tint", &BillboardComponent::Tint,
			"TextureHandle", &BillboardComponent::TextureHandle,
			"Size", &BillboardComponent::Size,
			"IsSpherical", &BillboardComponent::Spherical,
			"IsStaticSize", &BillboardComponent::StaticSize
		);

		state.new_usertype<TextComponent>("TextComponent",
			"Text", &TextComponent::Text,
			"Color", &TextComponent::Color
		);

		state.new_usertype<CameraComponent>("CameraComponent",
			"IsActive", &CameraComponent::IsActive,
			"ProjectionType", sol::property(
				[](CameraComponent& c) { return c.Camera.GetProjectionType(); },
				[](CameraComponent& c, Camera::ProjectionType type) { c.Camera.SetProjectionType(type); }
			),
			"FieldOfView", sol::property(
				[](CameraComponent& c) { return c.Camera.GetPerspectiveProps().FieldOfView; },
				[](CameraComponent& c, float fov) {
					auto& props = c.Camera.GetPerspectiveProps();
					c.Camera.SetPerspective(fov, props.NearClip, props.FarClip);
				}
			),
			"PerspectiveNear", sol::property(
				[](CameraComponent& c) { return c.Camera.GetPerspectiveProps().NearClip; },
				[](CameraComponent& c, float nearClip) {
					auto& props = c.Camera.GetPerspectiveProps();
					c.Camera.SetPerspective(props.FieldOfView, nearClip, props.FarClip);
				}
			),
			"PerspectiveFar", sol::property(
				[](CameraComponent& c) { return c.Camera.GetPerspectiveProps().FarClip; },
				[](CameraComponent& c, float farClip) {
					auto& props = c.Camera.GetPerspectiveProps();
					c.Camera.SetPerspective(props.FieldOfView, props.NearClip, farClip);
				}
			),
			"OrthographicSize", sol::property(
				[](CameraComponent& c) { return c.Camera.GetOrthographicProps().Size; },
				[](CameraComponent& c, float size) {
					auto& props = c.Camera.GetOrthographicProps();
					c.Camera.SetOrthographic(size, props.NearClip, props.FarClip);
				}
			),
			"OrthographicNear", sol::property(
				[](CameraComponent& c) { return c.Camera.GetOrthographicProps().NearClip; },
				[](CameraComponent& c, float nearClip) {
					auto& props = c.Camera.GetOrthographicProps();
					c.Camera.SetOrthographic(props.Size, nearClip, props.FarClip);
				}
			),
			"OrthographicFar", sol::property(
				[](CameraComponent& c) { return c.Camera.GetOrthographicProps().FarClip; },
				[](CameraComponent& c, float farClip) {
					auto& props = c.Camera.GetOrthographicProps();
					c.Camera.SetOrthographic(props.Size, props.NearClip, farClip);
				}
			)
		);
	}
}