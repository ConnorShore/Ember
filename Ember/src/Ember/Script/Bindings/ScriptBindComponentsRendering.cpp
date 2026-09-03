#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/System/ParticleSystem.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Scene/SceneManager.h"

namespace Ember {
	namespace {
		// Reads through to the effect asset; writing instances it first so one entity's tweak
		// doesn't leak into every other user of the same effect.
		template<typename T>
		static auto ParticleEffectProperty(T ParticleEffectProperties::* member)
		{
			return sol::property(
				[member](ComponentRef<ParticleEmitterComponent>& ref) -> T {
					auto effect = ref.Resolve().ResolveEffect();
					if (!effect)
						return T{};

					return effect->Properties.*member;
				},
				[member](ComponentRef<ParticleEmitterComponent>& ref, T value) {
					auto instance = ref.Resolve().GetInstanced("ScriptEffectInstance");
					if (instance)
						instance->Properties.*member = value;
				});
		}

		static void BurstFromEmitter(ComponentRef<ParticleEmitterComponent>& emitterRef, const Vector3f& position, uint32_t count, const Quaternion& worldRotation)
		{
			auto scene = Application::Instance().GetSceneManager().GetActiveScene();
			if (!scene)
				return;

			auto effect = emitterRef.Resolve().ResolveEffect();
			if (!effect)
				return;

			scene->GetParticleManager().EmitBurst(effect->Properties, position, count, worldRotation);
		}
	}

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
			"EffectHandle", RefProp(&ParticleEmitterComponent::EffectHandle),
			"IsActive", RefProp(&ParticleEmitterComponent::IsActive),
			"IsInstanced", RefMethod(&ParticleEmitterComponent::IsInstanced),
			"RevertToSourceEffect", RefMethod(&ParticleEmitterComponent::RevertToSourceEffect),
			"EmissionRate", ParticleEffectProperty(&ParticleEffectProperties::EmissionRate),
			"Velocity", ParticleEffectProperty(&ParticleEffectProperties::Velocity),
			"VelocityVariation", ParticleEffectProperty(&ParticleEffectProperties::VelocityVariation),
			"ColorBegin", ParticleEffectProperty(&ParticleEffectProperties::ColorBegin),
			"ColorEnd", ParticleEffectProperty(&ParticleEffectProperties::ColorEnd),
			"ColorIntensity", ParticleEffectProperty(&ParticleEffectProperties::ColorIntensity),
			"ScaleBegin", ParticleEffectProperty(&ParticleEffectProperties::ScaleBegin),
			"ScaleEnd", ParticleEffectProperty(&ParticleEffectProperties::ScaleEnd),
			"ScaleVariation", ParticleEffectProperty(&ParticleEffectProperties::ScaleVariation),
			"TextureHandle", ParticleEffectProperty(&ParticleEffectProperties::TextureHandle),
			"Lifetime", ParticleEffectProperty(&ParticleEffectProperties::Lifetime),
			"LifetimeVariation", ParticleEffectProperty(&ParticleEffectProperties::LifetimeVariation),
			"GravityMultiplier", ParticleEffectProperty(&ParticleEffectProperties::GravityMultiplier)
		);

		// Global Particles API for one-shot emission (impacts, explosions, etc.)
		// Burst(emitter, position, count)               -- emit in world-space (no rotation)
		// Burst(emitter, position, count, worldRotation) -- emit rotated into world-space
		sol::table particles = state.create_named_table("Particles");
		particles.set_function("Burst", sol::overload(
			[](ComponentRef<ParticleEmitterComponent>& emitterRef, const Vector3f& position, uint32_t count) {
				BurstFromEmitter(emitterRef, position, count, Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
			},
			[](ComponentRef<ParticleEmitterComponent>& emitterRef, const Vector3f& position, uint32_t count, const Quaternion& worldRotation) {
				BurstFromEmitter(emitterRef, position, count, worldRotation);
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