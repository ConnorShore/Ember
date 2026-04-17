#include "ebpch.h"
#include "CharacterControllerSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Physics/Collision.h"

namespace Ember {

	void CharacterControllerSystem::OnAttach()
	{
		EB_CORE_INFO("Character Controller System attached!");
	}

	void CharacterControllerSystem::OnDetach()
	{
		EB_CORE_INFO("Character Controller System detached!");
	}

	void CharacterControllerSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto& registry = scene->GetRegistry();
		auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
		auto view = registry.Query<CharacterControllerComponent, TransformComponent, RigidBodyComponent, CapsuleColliderComponent>();

		for (EntityID e : view)
		{
			Entity entity = Entity(e, scene);
			auto [controller, transform, rb, collider] = registry.GetComponents<CharacterControllerComponent, TransformComponent, RigidBodyComponent, CapsuleColliderComponent>(e);

			Quaternion currentRotation = Math::ToQuaternion(transform.Rotation);

			// 1. Grounded Check (Subtracted height, using Environment filter)
			Vector3f feetPos = transform.Position - Vector3f(0.0f, collider.Height * 0.5f, 0.0f);
			controller.IsGrounded = Collision::CheckOverlapSphere(feetPos, collider.Radius * 0.9f, CollisionFilterPreset::Environment);


			// 2. Apply Persistent Physics (Gravity)
			if (!controller.IsGrounded)
			{
				// Player is falling! Pull their persistent velocity down.
				// (Assuming physicsSystem->GetGravity() returns something like 9.8f)
				controller.Velocity.y -= (physicsSystem->GetSettings().GravityStrength * controller.GravityMultiplier * (float)delta);
			}
			else if (controller.Velocity.y < 0.0f)
			{
				// Player is on the ground. We don't want gravity to keep building up to -9000,
				// but we want a tiny bit of downward force so they stick to ramps as they walk down them.
				controller.Velocity.y = -2.0f;
			}

			// 3. Combine Input (Requested) with Physics (Velocity)
			Vector3f currentFrameDisplacement = controller.RequestedMovement + (controller.Velocity * (float)delta);

			// 4. The Move & Slide Loop
			int maxIterations = 3;
			for (int i = 0; i < maxIterations; i++)
			{
				// A. Propose the move
				transform.Position += currentFrameDisplacement;
				rb.Body->setTransform(rp3d::Transform(
					rp3d::Vector3(transform.Position.x, transform.Position.y, transform.Position.z),
					rp3d::Quaternion(currentRotation.x, currentRotation.y, currentRotation.z, currentRotation.w)
				));

				// B. Test for hits
				CollisionCallbackData collisionData = physicsSystem->TestCollision(entity);
				if (!collisionData.HasHit)
				{
					break; // Freedom! We moved without hitting anything.
				}

				// C. We hit a wall! The displacement for this micro-step is consumed.
				currentFrameDisplacement = Vector3f(0.0f);

				for (const auto& contact : collisionData.Contacts)
				{
					// Push the player instantly out of the wall
					transform.Position += contact.Normal * contact.PenetrationDepth;

					// Adjust our persistent velocity so we slide against the wall/ramp 
					// instead of smashing into it and losing all momentum.
					float velDot = Math::Dot(controller.Velocity, contact.Normal);
					if (velDot < 0.0f)
					{
						controller.Velocity -= contact.Normal * velDot;
					}
				}
			}

			// Reset requested movement so Lua must supply fresh input next frame
			controller.RequestedMovement = Vector3f(0.0f);
		}
	}

	void CharacterControllerSystem::Move(Entity entity, const Vector3f& displacement, const SharedPtr<PhysicsSystem>& physicsSystem)
	{
		// TODO: Move code here once working to clean up the update method
	}

}