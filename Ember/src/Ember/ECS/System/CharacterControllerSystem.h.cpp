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
		auto view = registry.ActiveQuery<CharacterControllerComponent, TransformComponent, RigidBodyComponent, CapsuleColliderComponent>();

		for (EntityID e : view)
		{
			Entity entity = Entity(e, scene);
			auto [controller, transform, rb, collider] = registry.GetComponents<CharacterControllerComponent, TransformComponent, RigidBodyComponent, CapsuleColliderComponent>(e);

			Quaternion currentRotation = Math::ToQuaternion(transform.Rotation);

			// Sync the current position and rotation into the physics body before any queries
			// so the grounded check and collision tests see an up-to-date kinematic body.
			rb.Body->setTransform(rp3d::Transform(
				rp3d::Vector3(transform.Position.x, transform.Position.y, transform.Position.z),
				rp3d::Quaternion(currentRotation.x, currentRotation.y, currentRotation.z, currentRotation.w)
			));

			// 1. Grounded check (Overlap is best for edges/ledges)
			const float groundCheckMargin = 0.05f;
			float checkRadius = collider.Radius;
			float yOffset = (collider.Height * 0.5f) - checkRadius + groundCheckMargin;
			Vector3f feetPos = transform.Position - Vector3f(0.0f, yOffset, 0.0f);

			auto overlapData = Collision::CheckOverlapSphereWithData(feetPos, checkRadius, entity, CollisionFilterPreset::Environment);
			controller.IsGrounded = overlapData;
			controller.GroundEntity = overlapData ? overlapData.Hits[0].EntityID : Constants::Entities::InvalidEntityID;

			// 2. Normal Check (Raycast to get the actual slope angle)
			Vector3f floorNormal = Vector3f(0.0f, 1.0f, 0.0f);
			if (controller.IsGrounded)
			{
				Vector3 characterBottom = transform.Position - Vector3f(0.0f, collider.Height * 0.5f, 0.0f);

				// Cast slightly further than step height to ensure we hit the ramp we are standing on
				auto hit = physicsSystem->CastRay(characterBottom, Vector3f(0.0f, -1.0f, 0.0f), controller.MaxStepHeight + 0.1f);
				if (hit.Hit)
				{
					// NOTE: Assuming your raycast hit struct has a 'Normal' property. 
					// Change this if your struct names it differently (e.g., hit.SurfaceNormal)
					floorNormal = hit.SurfaceNormal;
				}
			}

			// Apply Gravity
			if (!controller.IsGrounded)
			{
				controller.Velocity.y -= (physicsSystem->GetSettings().GravityStrength * controller.GravityMultiplier * (float)delta);
			}
			else if (controller.Velocity.y < 0.0f)
			{
				controller.Velocity = { 0.0f, 0.0f, 0.0f };
			}

			// Combine Input (Requested) with Physics (Velocity)
			Vector3f currentFrameDisplacement = controller.RequestedMovement + (controller.Velocity * (float)delta);
			float intendedDistance = Math::Length(currentFrameDisplacement);

			// --- TRUE SLOPE PROJECTION ---
			// If we are moving and on the ground, bend the vector to match the ramp angle
			if (controller.IsGrounded && intendedDistance > 0.0001f)
			{
				float floorAngle = Math::Degrees(acos(Math::Dot(floorNormal, Vector3f(0.0f, 1.0f, 0.0f))));
				if (floorAngle <= controller.MaxSlopeAngle)
				{
					// Get a vector pointing exactly to the right of our movement direction
					Vector3f right = Math::Cross(Vector3f(0.0f, 1.0f, 0.0f), currentFrameDisplacement);

					// Cross that Right vector with the Floor Normal to get a vector parallel to the ramp
					Vector3f slopeDir = Math::Cross(right, floorNormal);

					if (Math::Length(slopeDir) > 0.0001f)
					{
						// Set our movement to exactly match the ramp, retaining our original speed!
						currentFrameDisplacement = Math::Normalize(slopeDir) * intendedDistance;
					}
				}
			}

			// Apply movement EXACTLY ONCE before the depenetration loop
			transform.Position += currentFrameDisplacement;

			// The Move & Slide Depenetration Loop
			int maxIterations = 3;
			for (int i = 0; i < maxIterations; i++)
			{
				rb.Body->setTransform(rp3d::Transform(
					rp3d::Vector3(transform.Position.x, transform.Position.y, transform.Position.z),
					rp3d::Quaternion(currentRotation.x, currentRotation.y, currentRotation.z, currentRotation.w)
				));

				CollisionCallbackData collisionData = Collision::TestCollision(entity);
				if (!collisionData.HasHit)
					break; // No hit, we are safely out of the geometry!

				for (const auto& contact : collisionData.Contacts)
				{
					float angle = Math::Degrees(acos(Math::Dot(contact.Normal, Vector3f(0.0f, 1.0f, 0.0f))));
					bool isWalkableSlope = angle <= controller.MaxSlopeAngle;

					// Smart Depenetration
					if (isWalkableSlope)
					{
						// Floor: Only push straight UP
						float pushUpAmount = contact.PenetrationDepth / contact.Normal.y;
						transform.Position.y += pushUpAmount;
					}
					else
					{
						// Wall: Push out normally in 3D
						transform.Position += contact.Normal * contact.PenetrationDepth;

						// Kill persistent velocity against walls so you don't "stick"
						float velDot = Math::Dot(controller.Velocity, contact.Normal);
						if (velDot < 0.0f) {
							controller.Velocity -= contact.Normal * velDot;
						}
					}
				}
			}

			// Snap to ground if on slope
			if (controller.IsGrounded && controller.Velocity.y <= 0.0f)
			{
				Vector3 characterBottom = transform.Position - Vector3f(0, collider.Height * 0.5f, 0);
				auto hit = physicsSystem->CastRay(characterBottom, Vector3f(0, -1, 0), controller.MaxStepHeight);
				if (hit.Hit)
				{
					// Snap the character's Y position to the floor, completely eliminating the bounce!
					Vector3 newPos = transform.Position;
					newPos.y = hit.CollisionPoint.y + (collider.Height * 0.5f);
					transform.Position = newPos;
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