#include "efpch.h"
#include "Presets.h"

namespace Ember {

	Entity Presets::CreateCube(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Cube");

		MeshComponent mc(Constants::Assets::CubeMeshUUID);
		newEntity.AttachComponent<MeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		mtC.GetInstanced("Cube_Material");
		newEntity.AttachComponent<MaterialComponent>(mtC);

		return newEntity;
	}

	Entity Presets::CreateQuad(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Quad");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-90.0f), 0.0f, 0.0f);	// Make it face parallel to the ground by default

		MeshComponent mc(Constants::Assets::QuadMeshUUID);
		newEntity.AttachComponent<MeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		mtC.GetInstanced("Quad_Material");
		newEntity.AttachComponent<MaterialComponent>(mtC);

		return newEntity;
	}

	Entity Presets::CreateSphere(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Sphere");

		MeshComponent mc(Constants::Assets::SphereMeshUUID);
		newEntity.AttachComponent<MeshComponent>(mc);

		MaterialComponent mtC(Constants::Assets::StandardGeometryMatUUID);
		mtC.GetInstanced("Sphere_Material");
		newEntity.AttachComponent<MaterialComponent>(mtC);

		return newEntity;
	}

	Entity Presets::CreatePointLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Point_Light");
		
		PointLightComponent plc;
		newEntity.AttachComponent<PointLightComponent>(plc);

		auto lightTexture = Application::Instance().GetAssetManager().Load<Texture>("Ember-Forge/assets/icons/PointLight.png");

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

		auto lightTexture = Application::Instance().GetAssetManager().Load<Texture>("Ember-Forge/assets/icons/DirectionalLight.png");

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

		auto lightTexture = Application::Instance().GetAssetManager().Load<Texture>("Ember-Forge/assets/icons/SpotLight.png");

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

		auto cameraTexture = Application::Instance().GetAssetManager().Load<Texture>("Ember-Forge/assets/icons/Camera.png");

		BillboardComponent bc;
		bc.TextureHandle = cameraTexture->GetUUID();
		newEntity.AttachComponent<BillboardComponent>(bc);

		return newEntity;
	}

}