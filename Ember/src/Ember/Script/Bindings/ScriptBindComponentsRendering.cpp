#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/System/ParticleSystem.h"

namespace Ember {
	void BindRenderingComponents(sol::state& state)
	{
		// Bound as resolving handles (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<SpriteComponent>>("SpriteRendererComponent",
			"Color", RefProp(&SpriteComponent::Color),
			"TextureHandle", RefProp(&SpriteComponent::TextureHandle),
			"IsBillboard", RefProp(&SpriteComponent::IsBillboard),
			"LockYAxis", RefProp(&SpriteComponent::LockYAxis),
			"NineSliceBorder", RefProp(&SpriteComponent::NineSliceBorder)
		);

		state.new_usertype<ComponentRef<StaticMeshComponent>>("StaticMeshComponent",
			"MeshHandle", RefProp(&StaticMeshComponent::MeshHandle)
		);

		state.new_usertype<ComponentRef<SkinnedMeshComponent>>("SkinnedMeshComponent",
			"MeshHandle", RefProp(&SkinnedMeshComponent::MeshHandle),
			"AnimatorEntityHandle", RefProp(&SkinnedMeshComponent::AnimatorEntityHandle)
		);

		state.new_usertype<ComponentRef<MaterialComponent>>("MaterialComponent",
			"MaterialHandle", RefProp(&MaterialComponent::MaterialHandle),
			"GetInstanced", RefMethod(&MaterialComponent::GetInstanced),
			"CloneMaterial", RefMethod(&MaterialComponent::CloneMaterial)
		);

		state.new_usertype<ComponentRef<OutlineComponent>>("OutlineComponent",
			"Color", RefProp(&OutlineComponent::Color),
			"Thickness", RefProp(&OutlineComponent::Thickness)
		);

		state.new_usertype<ComponentRef<TextComponent>>("TextComponent",
			"Text", RefProp(&TextComponent::Text),
			"Color", RefProp(&TextComponent::Color),
			"FontSize", RefProp(&TextComponent::FontSize),
			"HorizontalAlignment", RefProp(&TextComponent::HorizontalAlignment),
			"VerticalAlignment", RefProp(&TextComponent::VerticalAlignment)
		);

		state.new_usertype<ComponentRef<ParticleEmitterComponent>>("ParticleEmitterComponent",
			"EmissionRate", RefProp(&ParticleEmitterComponent::EmissionRate),
			"Velocity", RefProp(&ParticleEmitterComponent::Velocity),
			"VelocityVariation", RefProp(&ParticleEmitterComponent::VelocityVariation),
			"ColorBegin", RefProp(&ParticleEmitterComponent::ColorBegin),
			"ColorEnd", RefProp(&ParticleEmitterComponent::ColorEnd),
			"ScaleBegin", RefProp(&ParticleEmitterComponent::ScaleBegin),
			"ScaleEnd", RefProp(&ParticleEmitterComponent::ScaleEnd),
			"ScaleVariation", RefProp(&ParticleEmitterComponent::ScaleVariation),
			"TextureHandle", RefProp(&ParticleEmitterComponent::TextureHandle),
			"Lifetime", RefProp(&ParticleEmitterComponent::Lifetime),
			"LifetimeVariation", RefProp(&ParticleEmitterComponent::LifetimeVariation),
			"GravityMultiplier", RefProp(&ParticleEmitterComponent::GravityMultiplier),
			"IsActive", RefProp(&ParticleEmitterComponent::IsActive)
		);

		// Global Particles API for one-shot emission (impacts, explosions, etc.)
		// Burst(emitter, position, count)               -- emit in world-space (no rotation)
		// Burst(emitter, position, count, worldRotation) -- emit rotated into world-space
		sol::table particles = state.create_named_table("Particles");
		particles.set_function("Burst", sol::overload(
			[](ComponentRef<ParticleEmitterComponent>& emitterRef, const Vector3f& position, uint32_t count) {
				auto particleSystem = Application::Instance().GetSystem<ParticleSystem>();
				if (particleSystem)
					particleSystem->GetParticleManager().EmitBurst(emitterRef.Resolve(), position, count);
			},
			[](ComponentRef<ParticleEmitterComponent>& emitterRef, const Vector3f& position, uint32_t count, const Quaternion& worldRotation) {
				auto particleSystem = Application::Instance().GetSystem<ParticleSystem>();
				if (particleSystem)
					particleSystem->GetParticleManager().EmitBurst(emitterRef.Resolve(), position, count, worldRotation);
			}
		));

		state.new_usertype<ComponentRef<CameraComponent>>("CameraComponent",
			"IsActive", RefProp(&CameraComponent::IsActive),
			"ProjectionType", sol::property(
				[](ComponentRef<CameraComponent>& r) { return r.Resolve().Camera.GetProjectionType(); },
				[](ComponentRef<CameraComponent>& r, Camera::ProjectionType type) { r.Resolve().Camera.SetProjectionType(type); }
			),
			"FieldOfView", sol::property(
				[](ComponentRef<CameraComponent>& r) { return r.Resolve().Camera.GetPerspectiveProps().FieldOfView; },
				[](ComponentRef<CameraComponent>& r, float fov) {
					auto& c = r.Resolve();
					auto& props = c.Camera.GetPerspectiveProps();
					c.Camera.SetPerspective(fov, props.NearClip, props.FarClip);
				}
			),
			"PerspectiveNear", sol::property(
				[](ComponentRef<CameraComponent>& r) { return r.Resolve().Camera.GetPerspectiveProps().NearClip; },
				[](ComponentRef<CameraComponent>& r, float nearClip) {
					auto& c = r.Resolve();
					auto& props = c.Camera.GetPerspectiveProps();
					c.Camera.SetPerspective(props.FieldOfView, nearClip, props.FarClip);
				}
			),
			"PerspectiveFar", sol::property(
				[](ComponentRef<CameraComponent>& r) { return r.Resolve().Camera.GetPerspectiveProps().FarClip; },
				[](ComponentRef<CameraComponent>& r, float farClip) {
					auto& c = r.Resolve();
					auto& props = c.Camera.GetPerspectiveProps();
					c.Camera.SetPerspective(props.FieldOfView, props.NearClip, farClip);
				}
			),
			"OrthographicSize", sol::property(
				[](ComponentRef<CameraComponent>& r) { return r.Resolve().Camera.GetOrthographicProps().Size; },
				[](ComponentRef<CameraComponent>& r, float size) {
					auto& c = r.Resolve();
					auto& props = c.Camera.GetOrthographicProps();
					c.Camera.SetOrthographic(size, props.NearClip, props.FarClip);
				}
			),
			"OrthographicNear", sol::property(
				[](ComponentRef<CameraComponent>& r) { return r.Resolve().Camera.GetOrthographicProps().NearClip; },
				[](ComponentRef<CameraComponent>& r, float nearClip) {
					auto& c = r.Resolve();
					auto& props = c.Camera.GetOrthographicProps();
					c.Camera.SetOrthographic(props.Size, nearClip, props.FarClip);
				}
			),
			"OrthographicFar", sol::property(
				[](ComponentRef<CameraComponent>& r) { return r.Resolve().Camera.GetOrthographicProps().FarClip; },
				[](ComponentRef<CameraComponent>& r, float farClip) {
					auto& c = r.Resolve();
					auto& props = c.Camera.GetOrthographicProps();
					c.Camera.SetOrthographic(props.Size, props.NearClip, farClip);
				}
			)
		);
	}
}