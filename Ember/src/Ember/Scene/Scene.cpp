#include "ebpch.h"
#include "Scene.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Core/Application.h"

#include "Ember/ECS/System/ScriptSystem.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/RenderSystem.h"
#include "Ember/ECS/System/AnimationSystem.h"
#include "Ember/ECS/System/TransformSystem.h"

#include "Ember/Script/ScriptEngine.h"

namespace Ember {

	namespace Utils {

		// Uses a fold expression to copy each component type from src to dst if present
		template<typename... Component>
		static void CopyComponents(Entity src, Entity dst)
		{
			([&]()
				{
					if (src.ContainsComponent<Component>())
					{
						dst.AttachComponent<Component>(src.GetComponent<Component>());
					}
				}(), ...);
		}
	}

	Scene::Scene(const std::string& name)
		: m_Registry(ScopedPtr<Registry>::Create()), m_Name(name)
	{
	}

	Scene::~Scene()
	{
	}

	// Deep-copies the scene, preserving all UUIDs so relationships remain valid
	SharedPtr<Scene> Scene::CopyScene(SharedPtr<Scene> other)
	{
		auto newScene = SharedPtr<Scene>::Create(other->GetName());
		auto view = other->GetRegistry().Query<IDComponent>();
		for (auto entity : view)
		{
			// We need the raw entity handle from the other scene to extract its data
			Entity srcEntity = { entity, other.Ptr() };

			// 2. Create a new entity in THIS scene using the EXACT SAME UUID and Name
			UUID id = srcEntity.GetComponent<IDComponent>().ID;
			std::string name = srcEntity.GetName();
			Entity destEntity = newScene->AddEntity(id, name);

			// 3. Use your amazing fold expression to copy all the data!
			// NOTE: Do not copy IDComponent here, we just set it above.
			Utils::CopyComponents<
				TransformComponent,
				StaticMeshComponent,  // NEW
				SkinnedMeshComponent, // NEW
				MaterialComponent,
				SpriteComponent,
				CameraComponent,
				ScriptComponent,
				RigidBodyComponent,
				DirectionalLightComponent,
				SpotLightComponent,
				PointLightComponent,
				RelationshipComponent,
				AnimatorComponent
			>(srcEntity, destEntity);
		}

		// Copy registry assets and systems to new scene

		return newScene;
	}

	void Scene::OnRuntimeStart()
	{
		ScriptEngine::OnRuntimeStart();
	}

	void Scene::OnRuntimeStop()
	{
		ScriptEngine::OnRuntimeStop();
	}

	void Scene::OnUpdateRuntime(TimeStep delta)
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<ScriptSystem>()->OnUpdate(delta, this);
		systemManager.GetSystem<AnimationSystem>()->OnUpdate(delta, this);
		systemManager.GetSystem<PhysicsSystem>()->OnUpdate(delta, this);
		systemManager.GetSystem<TransformSystem>()->OnUpdate(delta, this);
		systemManager.GetSystem<RenderSystem>()->OnUpdate(delta, this);
	}

	void Scene::OnUpdateEdit(TimeStep delta, EditorCamera& camera)
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<TransformSystem>()->OnUpdate(delta, this);
		//systemManager.GetSystem<AnimationSystem>()->OnUpdate(delta, this); // TODO: Remove from here, just using for testing
		systemManager.GetSystem<RenderSystem>()->OnUpdate(delta, this, camera, Math::Inverse(camera.GetViewMatrix()));
	}

	void Scene::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(WindowResizeEvent, OnWindowResize);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		auto view = m_Registry->Query<CameraComponent>();
		for (auto entity : view)
		{
			auto& camera = m_Registry->GetComponent<CameraComponent>(entity);
				camera.Camera.SetViewportSize(width, height);
		}

		// Notify the RenderSystem of the viewport resize so it can adjust framebuffer sizes accordingly
		auto renderSystem = systemManager.GetSystem<RenderSystem>();
		if (renderSystem)
		{
			renderSystem->OnViewportResize(width, height);
		}
	}

	Entity Scene::AddEntity(const std::string& name)
	{
		return AddEntity(UUID(), name);
	}

	Entity Scene::AddEntity(UUID uuid, const std::string& name)
	{
		EntityID handle = m_Registry->CreateEntity();
		Entity entity = { handle, this };

		auto& id = entity.AttachComponent<IDComponent>();
		id.ID = uuid;

		auto& tag = entity.AttachComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		entity.AttachComponent<TransformComponent>();
		entity.AttachComponent<RelationshipComponent>();

		m_EntityUUIDMap[uuid] = handle;

		return entity;
	}

	Entity Scene::GetEntity(UUID uuid)
	{
		auto it = m_EntityUUIDMap.find(uuid);
		if (it != m_EntityUUIDMap.end())
			return { it->second, this };

		return Entity();
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		return DuplicateEntityRecursive(entity, entity.GetComponent<RelationshipComponent>().ParentHandle, true);
	}

	void Scene::SetEntityParent(UUID childUUID, Entity newParent)
	{
		EB_CORE_ASSERT(newParent.ContainsComponent<RelationshipComponent>(), "New parent entity must have a RelationshipComponent!");
		auto& parentRelationship = newParent.GetComponent<RelationshipComponent>();
		parentRelationship.Children.push_back(childUUID);

		auto childEntity = GetEntity(childUUID);
		auto& childRelationship = childEntity.GetComponent<RelationshipComponent>();

		// If the child had a previous parent, we need to remove it from that parent's children list
		if (childRelationship.ParentHandle != Constants::InvalidUUID)
		{
			Entity oldParent = GetEntity(childRelationship.ParentHandle);
			if (oldParent != Constants::Entities::InvalidEntityID)
			{
				auto& oldParentRelationship = oldParent.GetComponent<RelationshipComponent>();
				oldParentRelationship.Children.erase(std::remove(oldParentRelationship.Children.begin(), oldParentRelationship.Children.end(), childUUID), oldParentRelationship.Children.end());
			}
		}

		// Set new parent handle
		childRelationship.ParentHandle = newParent.GetUUID();

		// Recompute the child's local transform relative to the new parent
		auto& parentTransform = newParent.GetComponent<TransformComponent>();
		auto& childTransform = childEntity.GetComponent<TransformComponent>();
		auto parentInverseWorldTransform = Math::Inverse(parentTransform.GetWorldTransform());
		Vector3f outPos, outRot, outScale;
		Math::DecomposeTransform(parentInverseWorldTransform, outPos, outRot, outScale);

		childTransform.Position = outPos + childTransform.Position;
		childTransform.Rotation = outRot + childTransform.Rotation;
		childTransform.Scale = outScale * childTransform.Scale;
	}

	void Scene::RemoveParent(Entity child)
	{
		auto& relationship = child.GetComponent<RelationshipComponent>();
		if (relationship.ParentHandle != Constants::InvalidUUID)
		{
			auto parentEntity = GetEntity(relationship.ParentHandle);
			auto& parentRelationship = parentEntity.GetComponent<RelationshipComponent>();
			parentRelationship.Children.erase(std::remove(parentRelationship.Children.begin(), parentRelationship.Children.end(), child.GetUUID()), parentRelationship.Children.end());

			relationship.ParentHandle = Constants::InvalidUUID;
		}
	}

	// Recursively duplicates an entity and its children.
	// isRoot: true for the top-level call so it keeps the same parent; false for descendants.
	// originalAnimatorUUID/newAnimatorUUID track the UUID remapping for AnimatorComponent owners
	// so SkinnedMeshComponent handles are updated inline rather than repaired after the fact.
	Entity Scene::DuplicateEntityRecursive(Entity entity, UUID newParentId, bool isRoot, UUID originalAnimatorUUID, UUID newAnimatorUUID)
	{
		std::string name = entity.GetName();
		Entity newEntity = AddEntity(name + " (Copy)");

		Utils::CopyComponents<
			TransformComponent,
			StaticMeshComponent, 
			SkinnedMeshComponent,
			MaterialComponent,
			SpriteComponent,
			CameraComponent,
			ScriptComponent,
			RigidBodyComponent,
			DirectionalLightComponent,
			SpotLightComponent,
			PointLightComponent,
			AnimatorComponent
		>(entity, newEntity);

		// Clear runtime cache for skinned mesh component so new skeleton UUID is used
		if (newEntity.ContainsComponent<SkinnedMeshComponent>())
		{
			newEntity.GetComponent<SkinnedMeshComponent>().RuntimeAnimatorID = Constants::Entities::InvalidEntityID;
		}

		// If this entity owns the animator, establish the old->new UUID mapping
		if (newEntity.ContainsComponent<AnimatorComponent>())
		{
			originalAnimatorUUID = entity.GetUUID();
			newAnimatorUUID = newEntity.GetUUID();
		}

		// Remap AnimatorEntityHandle to point at the new animator entity, not the original
		if (newEntity.ContainsComponent<SkinnedMeshComponent>() && originalAnimatorUUID != Constants::InvalidUUID)
		{
			auto& mesh = newEntity.GetComponent<SkinnedMeshComponent>();
			if (mesh.AnimatorEntityHandle == originalAnimatorUUID)
				mesh.AnimatorEntityHandle = newAnimatorUUID;
		}

		if (entity.ContainsComponent<RelationshipComponent>())
		{
			auto oldRels = entity.GetComponent<RelationshipComponent>();
			RelationshipComponent newRels;

			if (isRoot)
			{
				newRels.ParentHandle = oldRels.ParentHandle;

				if (newParentId != Constants::InvalidUUID)
				{
					Entity newParent = GetEntity(newParentId);
					if (newParent)
						newParent.GetComponent<RelationshipComponent>().Children.push_back(newEntity.GetUUID());
				}
			}
			else
			{
				newRels.ParentHandle = newParentId;
			}

			for (UUID childUUID : oldRels.Children)
			{
				Entity childEntity = GetEntity(childUUID);
				if (childEntity != Constants::Entities::InvalidEntityID)
				{
					Entity duplicatedChild = DuplicateEntityRecursive(childEntity, newEntity.GetUUID(), false, originalAnimatorUUID, newAnimatorUUID);
					newRels.Children.push_back(duplicatedChild.GetUUID());
				}
			}

			newEntity.AttachComponent(newRels);
		}

		return newEntity;
	}

	std::vector<Entity> Scene::GetAllEntities() const
	{
		std::vector<Entity> entities;
		entities.reserve(m_EntityUUIDMap.size());

		for (const auto& [uuid, id] : m_EntityUUIDMap)
			entities.emplace_back(id, const_cast<Scene*>(this));

		return entities;
	}

	void Scene::RemoveEntity(Entity entity)
	{
		EB_CORE_ASSERT(m_EntityUUIDMap.find(entity.GetUUID()) != m_EntityUUIDMap.end(), "Scene does not contain entity!");

		// Remove children first
		for (auto child : entity.GetAllChildren())
			RemoveEntity(child);

		// Remove from ECS and our Map
		UUID entityUUID = entity.GetUUID();
		m_Registry->DestroyEntity(entity.GetEntityHandle());
		m_EntityUUIDMap.erase(entityUUID);
	}

	Entity Scene::GetEntityAtPixel(uint32_t x, uint32_t y)
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		auto renderSystem = systemManager.GetSystem<RenderSystem>();
		EntityID id = renderSystem->GetEntityIDAtPixel(x, y);
		return id != Constants::Entities::InvalidEntityID ? Entity(id, this) : Entity();
	}

	Entity Scene::InstantiateModel(const std::string& modelFile)
	{
		std::string modelName = std::filesystem::path(modelFile).stem().string();
		auto& am = Application::Instance().GetAssetManager();

		SharedPtr<Model> model = am.GetAsset<Model>(modelName);

		Entity rootEntity = AddEntity(model->GetName());
		UUID animatorEntity = Constants::InvalidUUID;

		if (model->GetSkeletonHandle() != Constants::InvalidUUID)
		{
			AnimatorComponent animator;
			animator.SkeletonHandle = model->GetSkeletonHandle();

			// Temp testing
			auto anim = am.GetAsset<Animation>("Anim_0");
			animator.CurrentAnimationHandle = anim->GetUUID();

			rootEntity.AttachComponent<AnimatorComponent>(animator);
			animatorEntity = rootEntity.GetUUID(); // Save the UUID
		}

		ProcessModelNode(rootEntity, model->GetRootNode(), model, animatorEntity);
		return rootEntity;
	}

	bool Scene::OnWindowResize(const WindowResizeEvent& event)
	{
		OnViewportResize(event.GetWidth(), event.GetHeight());
		return false;
	}

	void Scene::ProcessModelNode(Entity currentEntity, const ModelNode& node, const SharedPtr<Model>& model, UUID animatorEntityUUID)
	{
		auto& transform = currentEntity.GetComponent<TransformComponent>();
		Math::DecomposeTransform(node.LocalTransform, transform.Position, transform.Rotation, transform.Scale);

		// Helper to attach the correct mesh component
		auto attachMesh = [&](Entity e, const MeshMaterialNode& mNode) {
			auto meshAsset = mNode.MeshAsset;
			bool isSkinned = DynamicPointerCast<SkinnedMesh>(meshAsset) != nullptr;
			if (isSkinned) 
			{
				SkinnedMeshComponent skinnedMeshComp(meshAsset->GetUUID(), animatorEntityUUID);
				e.AttachComponent<SkinnedMeshComponent>(skinnedMeshComp);
			}
			else 
			{
				StaticMeshComponent staticMeshComp(meshAsset->GetUUID());
				e.AttachComponent<StaticMeshComponent>(staticMeshComp);
			}

			UUID materialId = model->GetAllMaterials()[mNode.MaterialIndex]->GetUUID();
			MaterialComponent matComp(materialId);
			e.AttachComponent<MaterialComponent>(matComp);
			};

		if (node.Meshes.size() == 1)
		{
			attachMesh(currentEntity, node.Meshes[0]);
		}
		else if (node.Meshes.size() > 1)
		{
			for (size_t i = 0; i < node.Meshes.size(); i++)
			{
				Entity meshPartEntity = AddEntity(node.Name + "_Part" + std::to_string(i));
				auto& partRc = meshPartEntity.GetComponent<RelationshipComponent>();
				partRc.ParentHandle = currentEntity.GetUUID();
				currentEntity.GetComponent<RelationshipComponent>().Children.push_back(meshPartEntity.GetUUID());

				attachMesh(meshPartEntity, node.Meshes[i]);
			}
		}

		for (const auto& childNode : node.ChildNodes)
		{
			Entity childEntity = AddEntity(childNode.Name);
			auto& childRc = childEntity.GetComponent<RelationshipComponent>();
			childRc.ParentHandle = currentEntity.GetUUID();
			currentEntity.GetComponent<RelationshipComponent>().Children.push_back(childEntity.GetUUID());

			ProcessModelNode(childEntity, childNode, model, animatorEntityUUID);
		}
	}

}