#include "efpch.h"
#include "Presets.h"
#include "EditorConstants.h"

namespace Ember {

	Entity Presets::CreateCharacterController(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Character_Controller");

		StaticMeshComponent mc(Constants::Assets::CapsuleMeshUUID);
		newEntity.AttachComponent<StaticMeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<MaterialComponent>(mtC);

		CharacterControllerComponent ccc;
		newEntity.AttachComponent<CharacterControllerComponent>(ccc);

		RigidBodyComponent rbc;
		rbc.Type = RigidBodyComponent::BodyType::Kinematic;
		newEntity.AttachComponent<RigidBodyComponent>(rbc);

		CapsuleColliderComponent colC;
		colC.AttachedBody = rbc.Body;
		newEntity.AttachComponent<CapsuleColliderComponent>(colC);

		// TODO: Add basic script for character movement (WASD + Jump)

		return newEntity;
	}

	Entity Presets::CreateCube(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Cube");

		StaticMeshComponent mc(Constants::Assets::CubeMeshUUID);
		newEntity.AttachComponent<StaticMeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<MaterialComponent>(mtC);

		return newEntity;
	}

	Entity Presets::CreateQuad(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Quad");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-90.0f), 0.0f, 0.0f);	// Make it face parallel to the ground by default

		StaticMeshComponent mc(Constants::Assets::QuadMeshUUID);
		newEntity.AttachComponent<StaticMeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<MaterialComponent>(mtC);

		return newEntity;
	}

	Entity Presets::CreateSphere(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Sphere");

		StaticMeshComponent mc(Constants::Assets::SphereMeshUUID);
		newEntity.AttachComponent<StaticMeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<MaterialComponent>(mtC);

		return newEntity;
	}

	Entity Presets::CreateCapsule(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Capsule");

		StaticMeshComponent mc(Constants::Assets::CapsuleMeshUUID);
		newEntity.AttachComponent<StaticMeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<MaterialComponent>(mtC);

		return newEntity;
	}

	Entity Presets::CreatePointLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Point_Light");
		
		PointLightComponent plc;
		newEntity.AttachComponent<PointLightComponent>(plc);

		auto lightTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::PointLightTexUUID);

		BillboardComponent bc;
		bc.TextureHandle = lightTexture->GetUUID();
		newEntity.AttachComponent<BillboardComponent>(bc);

		return newEntity;
	}

	Entity Presets::CreateDirectionalLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Directional_Light");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-50.0f), Math::Radians(30.0f), 0.0f);	// Make it point diagonally downwards by default

		DirectionalLightComponent dlc;
		newEntity.AttachComponent<DirectionalLightComponent>(dlc);


		auto lightTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::DirectionalLightTexUUID);

		BillboardComponent bc;
		bc.TextureHandle = lightTexture->GetUUID();
		bc.Size = 1.5f;
		newEntity.AttachComponent<BillboardComponent>(bc);

		return newEntity;
	}

	Ember::Entity Presets::CreateSpotLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Spot_Light");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-90.0f), 0.0f, 0.0f);	// Make it point strait downwards by default

		SpotLightComponent slc;
		newEntity.AttachComponent<SpotLightComponent>(slc);

		auto lightTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::SpotLightTexUUID);

		BillboardComponent bc;
		bc.TextureHandle = lightTexture->GetUUID();
		newEntity.AttachComponent<BillboardComponent>(bc);

		return newEntity;
	}

	Entity Presets::Create3DCamera(const SharedPtr<Scene>& scene, const Vector3f& position /*= Vector3f(0.0f) */, const Quaternion& orientation /*= Quaternion(1.0f, 0.0f, 0.0f, 0.0f*/)
	{
		Entity newEntity = scene->AddEntity("Camera_3D");
		auto& transform = newEntity.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = Math::ToEulerAngles(orientation);

		CameraComponent cc;
		newEntity.AttachComponent<CameraComponent>(cc);


		auto cameraTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(EditorConstants::Assets::CameraTexUUID);

		BillboardComponent bc;
		bc.TextureHandle = cameraTexture->GetUUID();
		newEntity.AttachComponent<BillboardComponent>(bc);

		return newEntity;
	}

	Entity Presets::CreatePostProcessVolume(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("PostProcessVolume");

		RigidBodyComponent rbc;
		rbc.Type = RigidBodyComponent::BodyType::Static;
		newEntity.AttachComponent<RigidBodyComponent>(rbc);

		BoxColliderComponent boxCol;
		boxCol.AttachedBody = rbc.Body;
		boxCol.IsTrigger = true;
		newEntity.AttachComponent<BoxColliderComponent>(boxCol);

		PostProcessVolumeComponent ppvc;
		newEntity.AttachComponent<PostProcessVolumeComponent>(ppvc);

		return newEntity;
	}

}