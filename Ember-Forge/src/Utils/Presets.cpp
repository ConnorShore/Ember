#include "efpch.h"
#include "Presets.h"
#include "EditorConstants.h"

#include <Ember/Core/Paths.h>
#include <Ember/Core/ProjectManager.h>
#include <Ember/Scene/Scene.h>

namespace Ember {

	Entity Presets::CreateFirstPersonCharacterController(const SharedPtr<Scene>& scene)
	{
		auto& assetManager = Application::Instance().GetAssetManager();

		// Load assets into project if they don't exist
		UUID characterMovementUUID = Constants::InvalidUUID;
		if (!assetManager.ContainsAssetWithName("CharacterMovement"))
		{
			auto scriptsDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Script);

			// Copy CharacterMovement.lua to existing scriptsDirectory
			std::filesystem::path sourcePath = Paths::EngineAssets() / "scripts/CharacterMovement.lua";
			std::filesystem::path destPath = scriptsDirectory / "CharacterMovement.lua";

			// Copy file if it doesn't exist, and throw error if copy fails. If it already exists, just load the asset file
			if (!std::filesystem::exists(destPath) && !std::filesystem::copy_file(sourcePath, destPath))
			{
				EB_CORE_ERROR("Failed to copy CharacterMovement.lua to project assets directory! Aborting controller creation!");
				return {};
			}

			characterMovementUUID = assetManager.Load<Script>(destPath.string(), false)->GetUUID();
		}
		else
			characterMovementUUID = assetManager.Load<Script>(assetManager.GetAsset<Script>("CharacterMovement")->GetFilePath(), false)->GetUUID();

		UUID freeLookUUID = Constants::InvalidUUID;
		if (!assetManager.ContainsAssetWithName("FreeLook"))
		{
			auto scriptsDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Script);

			// Copy FreeLook.lua to existing scriptsDirectory
			std::filesystem::path sourcePath = Paths::EngineAssets() / "scripts/FreeLook.lua";
			std::filesystem::path destPath = scriptsDirectory / "FreeLook.lua";

			// Copy file if it doesn't exist, and throw error if copy fails. If it already exists, just load the asset file
			if (!std::filesystem::exists(destPath) && !std::filesystem::copy_file(sourcePath, destPath))
			{
				EB_CORE_ERROR("Failed to copy FreeLook.lua to project assets directory! Aborting controller creation!");
				return {};
			}

			freeLookUUID = assetManager.Load<Script>(destPath.string(), false)->GetUUID();
		}
		else
			freeLookUUID = assetManager.Load<Script>(assetManager.GetAsset<Script>("FreeLook")->GetFilePath(), false)->GetUUID();

		// Build the entity with all the necessary components for a basic character controller
		// No mesh/material by default until render masks are implemented
		Entity newEntity = scene->AddEntity("FirstPersonCharacter");

		newEntity.AttachComponent<CharacterControllerComponent>();

		auto& rbc = newEntity.AttachComponent<RigidBodyComponent>();
		rbc.Type = RigidBodyComponent::BodyType::Kinematic;

		auto& colC = newEntity.AttachComponent<CapsuleColliderComponent>();
		colC.AttachedBody = rbc.Body;

		auto& movementScript = newEntity.AttachComponent<ScriptComponent>();
		movementScript.ScriptHandle = characterMovementUUID;

		// Head pivot (empty) child entity for free look
		Entity headPivot = newEntity.AddChild("HeadPivot");
		auto& headTransform = headPivot.GetComponent<TransformComponent>();
		headTransform.Position.y = 0.8f;
		headTransform.Position.z = -0.2f;

		// Add camera
		Entity cameraEntity = headPivot.AddChild("Camera");
		auto& cameraComponent = cameraEntity.AttachComponent<CameraComponent>();
		cameraComponent.IsActive = true;
		cameraComponent.Camera.SetPerspective(70.0f, 0.1f, 500.0f);

		// Add camera script
		auto& camScript = cameraEntity.AttachComponent<ScriptComponent>();
		camScript.ScriptHandle = freeLookUUID;

		return newEntity;
	}

	Entity Presets::CreateAICharacterController(const SharedPtr<Scene>& scene)
	{
		auto& assetManager = Application::Instance().GetAssetManager();

		UUID aiControllerUUID = Constants::InvalidUUID;
		if (!assetManager.ContainsAssetWithName("AIController"))
		{
			auto scriptsDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Script);

			// Copy AIController.lua to existing scriptsDirectory
			std::filesystem::path sourcePath = Paths::EngineAssets() / "scripts/AIController.lua";
			std::filesystem::path destPath = scriptsDirectory / "AIController.lua";

			// Copy file if it doesn't exist, and throw error if copy fails. If it already exists, just load the asset file
			if (!std::filesystem::exists(destPath) && !std::filesystem::copy_file(sourcePath, destPath))
			{
				EB_CORE_ERROR("Failed to copy AIController.lua to project assets directory! Aborting controller creation!");
				return {};
			}

			aiControllerUUID = assetManager.Load<Script>(destPath.string(), false)->GetUUID();
		}
		else
			aiControllerUUID = assetManager.Load<Script>(assetManager.GetAsset<Script>("AIController")->GetFilePath(), false)->GetUUID();

		Entity newEntity = scene->AddEntity("AICharacterController");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::CapsuleMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<CharacterControllerComponent>();
		newEntity.AttachComponent<AIPathComponent>();
		newEntity.AttachComponent<AIAgentComponent>();

		auto& rbc = newEntity.AttachComponent<RigidBodyComponent>();
		rbc.Type = RigidBodyComponent::BodyType::Kinematic;

		auto& colC = newEntity.AttachComponent<CapsuleColliderComponent>();
		colC.AttachedBody = rbc.Body;

		auto& aiControllerScript = newEntity.AttachComponent<ScriptComponent>();
		aiControllerScript.ScriptHandle = aiControllerUUID;

		return newEntity;
	}

	Entity Presets::CreateWaypoint(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Waypoint");
		newEntity.AttachComponent<WaypointComponent>();

		return newEntity;
	}

	Entity Presets::CreateNavigationGrid(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("NavigationGrid");
		newEntity.AttachComponent<NavigationGridComponent>();

		return newEntity;
	}

	Ember::Entity Presets::CreateNavigationMesh(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("NavigationSurface");
		newEntity.AttachComponent<NavigationMeshComponent>();

		return newEntity;
	}

	Entity Presets::CreateCube(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Cube");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::CubeMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<RigidBodyComponent>();
		newEntity.AttachComponent<BoxColliderComponent>();

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
		newEntity.AttachComponent<RigidBodyComponent>();
		newEntity.AttachComponent<SphereColliderComponent>();

		return newEntity;
	}

	Entity Presets::CreateCapsule(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Capsule");

		newEntity.AttachComponent<StaticMeshComponent>(Constants::Assets::CapsuleMeshUUID);
		newEntity.AttachComponent<MaterialComponent>(Constants::Assets::StandardGeometryMatUUID);
		newEntity.AttachComponent<RigidBodyComponent>();
		newEntity.AttachComponent<CapsuleColliderComponent>();

		return newEntity;
	}

	Entity Presets::CreatePointLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("PointLight");
		
		newEntity.AttachComponent<PointLightComponent>();

		return newEntity;
	}

	Entity Presets::CreateDirectionalLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("DirectionalLight");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-50.0f), Math::Radians(30.0f), 0.0f);	// Make it point diagonally downwards by default

		newEntity.AttachComponent<DirectionalLightComponent>();

		return newEntity;
	}

	Ember::Entity Presets::CreateSpotLight(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("SpotLight");
		newEntity.GetComponent<TransformComponent>().Rotation = Vector3f(Math::Radians(-90.0f), 0.0f, 0.0f);	// Make it point strait downwards by default

		newEntity.AttachComponent<SpotLightComponent>();

		return newEntity;
	}

	Entity Presets::Create3DCamera(const SharedPtr<Scene>& scene, const Vector3f& position /*= Vector3f(0.0f) */, const Quaternion& orientation /*= Quaternion(1.0f, 0.0f, 0.0f, 0.0f*/)
	{
		Entity newEntity = scene->AddEntity("Camera3D");
		auto& transform = newEntity.GetComponent<TransformComponent>();
		transform.Position = position;
		transform.Rotation = Math::ToEulerAngles(orientation);

		newEntity.AttachComponent<CameraComponent>();

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
		boxCol.PreviewCollider = true;

		newEntity.AttachComponent<PostProcessVolumeComponent>();

		return newEntity;
	}

	Entity Presets::CreateCanvas(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("Canvas");
		
		newEntity.AttachComponent<RectTransformComponent>();

		auto& canvasComp = newEntity.AttachComponent<CanvasComponent>();
		canvasComp.ReferenceResolution = scene->GetViewportSize();

		return newEntity;
	}

	Entity Presets::CreateUISprite(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("UISprite");
		newEntity.AttachComponent<RectTransformComponent>();
		newEntity.AttachComponent<SpriteComponent>();
		return newEntity;
	}

	Entity Presets::CreateUIText(const SharedPtr<Scene>& scene)
	{
		Entity newEntity = scene->AddEntity("UIText");
		newEntity.AttachComponent<RectTransformComponent>();
		newEntity.AttachComponent<TextComponent>();
		return newEntity;
	}

	// A button is a three-part assembly (background + interaction + label), so the preset builds
	// the whole thing - otherwise every button has to be wired by hand.
	Entity Presets::CreateUIButton(const SharedPtr<Scene>& scene)
	{
		Entity buttonEntity = scene->AddEntity("UIButton");

		auto& rect = buttonEntity.AttachComponent<RectTransformComponent>();
		rect.SizeDelta = { 200.0f, 60.0f };

		buttonEntity.AttachComponent<SpriteComponent>();
		buttonEntity.AttachComponent<UISelectableComponent>();
		buttonEntity.AttachComponent<UIButtonComponent>();

		Entity labelEntity = scene->AddEntity("Label");
		auto& labelRect = labelEntity.AttachComponent<RectTransformComponent>();

		// Stretch to fill the button so the label stays centred at any button size.
		labelRect.AnchorMin = { 0.0f, 0.0f };
		labelRect.AnchorMax = { 1.0f, 1.0f };
		labelRect.SizeDelta = { 0.0f, 0.0f };

		auto& label = labelEntity.AttachComponent<TextComponent>();
		label.Text = "Button";
		label.Color = { 0.0f, 0.0f, 0.0f, 1.0f };

		scene->SetEntityParent(labelEntity.GetUUID(), buttonEntity);
		return buttonEntity;
	}

	Entity Presets::CreateUIToggle(const SharedPtr<Scene>& scene)
	{
		Entity toggleEntity = scene->AddEntity("UIToggle");

		auto& rect = toggleEntity.AttachComponent<RectTransformComponent>();
		rect.SizeDelta = { 40.0f, 40.0f };

		toggleEntity.AttachComponent<SpriteComponent>();
		toggleEntity.AttachComponent<UISelectableComponent>();
		auto& toggle = toggleEntity.AttachComponent<UIToggleComponent>();

		Entity checkmarkEntity = scene->AddEntity("Checkmark");
		auto& checkRect = checkmarkEntity.AttachComponent<RectTransformComponent>();
		checkRect.AnchorMin = { 0.0f, 0.0f };
		checkRect.AnchorMax = { 1.0f, 1.0f };
		checkRect.SizeDelta = { -12.0f, -12.0f };

		auto& checkSprite = checkmarkEntity.AttachComponent<SpriteComponent>();
		checkSprite.Color = { 0.2f, 0.7f, 0.3f, 1.0f };

		scene->SetEntityParent(checkmarkEntity.GetUUID(), toggleEntity);
		toggle.CheckmarkEntity = checkmarkEntity.GetUUID();

		// The checkmark must not swallow clicks meant for the toggle behind it.
		checkmarkEntity.SetActive(toggle.IsOn, true);
		return toggleEntity;
	}

}