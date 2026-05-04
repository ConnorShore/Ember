#include "efpch.h"
#include "Presets.h"
#include "EditorConstants.h"

namespace Ember {

	Entity Presets::CreateCharacterController(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Character_Controller");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::CapsuleMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<CharacterControllerComponent>();

		auto& rbc = newEntity.AttachComponent<RigidBodyComponent>();
		rbc.Type = RigidBodyComponent::BodyType::Kinematic;

		auto& colC = newEntity.AttachComponent<CapsuleColliderComponent>();
		colC.AttachedBody = rbc.Body;

		// TODO: Add basic script for character movement (WASD + Jump)

		return newEntity;
	}

	Entity Presets::CreateAICharacterController(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("AI_Character_Controller");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::CapsuleMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<CharacterControllerComponent>();
		newEntity.AttachComponent<AIPathComponent>();

		auto& rbc = newEntity.AttachComponent<RigidBodyComponent>();
		rbc.Type = RigidBodyComponent::BodyType::Kinematic;

		auto& colC = newEntity.AttachComponent<CapsuleColliderComponent>();
		colC.AttachedBody = rbc.Body;

		// TODO: Add basic script for AI pathfinding movement between waypoints

		return newEntity;
	}

	Entity Presets::CreateWaypoint(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Waypoint");
		newEntity.AttachComponent<WaypointComponent>();

		return newEntity;
	}

	Entity Presets::CreateCube(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Cube");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::CubeMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);

		return newEntity;
	}

	Entity Presets::CreateQuad(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Quad");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-90.0f), 0.0f, 0.0f);	// Make it face parallel to the ground by default

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::QuadMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);

		return newEntity;
	}

	Entity Presets::CreateSphere(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Sphere");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::SphereMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);

		return newEntity;
	}

	Entity Presets::CreateCapsule(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Capsule");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::CapsuleMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);

		return newEntity;
	}

	Entity Presets::CreatePointLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Point_Light");
		
		newEntity.AttachComponent<PointLightComponent>();

		auto lightTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::PointLightTexUUID);

		auto& bc = newEntity.AttachComponent<BillboardComponent>();
		bc.TextureHandle = lightTexture->GetUUID();

		return newEntity;
	}

	Entity Presets::CreateDirectionalLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Directional_Light");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-50.0f), Math::Radians(30.0f), 0.0f);	// Make it point diagonally downwards by default

		newEntity.AttachComponent<DirectionalLightComponent>();

		auto lightTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::DirectionalLightTexUUID);

		auto& bc = newEntity.AttachComponent<BillboardComponent>();
		bc.TextureHandle = lightTexture->GetUUID();
		bc.Size = 1.5f;

		return newEntity;
	}

	Ember::Entity Presets::CreateSpotLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Spot_Light");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-90.0f), 0.0f, 0.0f);	// Make it point strait downwards by default

		newEntity.AttachComponent<SpotLightComponent>();

		auto lightTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::SpotLightTexUUID);

		auto& bc = newEntity.AttachComponent<BillboardComponent>();
		bc.TextureHandle = lightTexture->GetUUID();

		return newEntity;
	}

	Entity Presets::Create3DCamera(const SharedPtr<Scene>& scene, const Vector3f& position /*= Vector3f(0.0f) */, const Quaternion& orientation /*= Quaternion(1.0f, 0.0f, 0.0f, 0.0f*/)
	{
		Entity newEntity = scene->AddEntity("Camera_3D");
		auto& transform = newEntity.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = Math::ToEulerAngles(orientation);

		newEntity.AttachComponent<CameraComponent>();

		auto cameraTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::CameraTexUUID);

		auto& bc = newEntity.AttachComponent<BillboardComponent>();
		bc.TextureHandle = cameraTexture->GetUUID();

		return newEntity;
	}

	Entity Presets::CreatePostProcessVolume(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("PostProcessVolume");

		auto& rbc = newEntity.AttachComponent<RigidBodyComponent>();
		rbc.Type = RigidBodyComponent::BodyType::Static;

		auto& boxCol = newEntity.AttachComponent<BoxColliderComponent>();
		boxCol.AttachedBody = rbc.Body;
		boxCol.IsTrigger = true;
		boxCol.Category = CollisionFilterPreset::VFX;
		boxCol.PreviewCollider = true;

		newEntity.AttachComponent<PostProcessVolumeComponent>();

		return newEntity;
	}

}