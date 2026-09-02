#include "ebpch.h"
#include "Scene.h"
#include "SceneSerializer.h"

#include "Ember/Asset/Serializers/AssetRegistrySerializer.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/Component/ComponentList.h"
#include "Ember/Core/Application.h"
#include "Ember/Core/ProjectManager.h"

#include "Ember/ECS/System/ScriptSystem.h"
#include "Ember/ECS/System/PhysicsSystem.h"
#include "Ember/ECS/System/RenderSystem.h"
#include "Ember/ECS/System/AnimationSystem.h"
#include "Ember/ECS/System/VisibilitySystem.h"
#include "Ember/ECS/System/TransformSystem.h"
#include "Ember/ECS/System/BoneSocketSystem.h"
#include "Ember/ECS/System/CharacterControllerSystem.h" 
#include "Ember/ECS/System/LifecycleSystem.h"
#include "Ember/ECS/System/ParticleSystem.h"
#include "Ember/ECS/System/AudioSystem.h"
#include "Ember/ECS/System/AISystem.h"
#include "Ember/ECS/System/UILayoutSystem.h"
#include "Ember/ECS/System/UIInputSystem.h"

#include "Ember/Script/ScriptEngine.h"

#include "Ember/Utils/StringUtils.h"

namespace Ember {

	namespace Utils {

		static void EraseUUID(std::vector<UUID>& uuids, UUID uuid)
		{
			uuids.erase(std::remove(uuids.begin(), uuids.end(), uuid), uuids.end());
		}

		// Depth-first, matching the order DuplicateEntityRecursive creates entities in, so a
		// parallel walk of the source and the copy pairs them up positionally.
		static void CollectSubtreeUUIDs(Scene* scene, Entity root, std::vector<UUID>& outUUIDs)
		{
			if (root == Constants::Entities::InvalidEntityID || !root.ContainsComponent<IDComponent>())
				return;

			outUUIDs.push_back(root.GetUUID());

			if (!root.ContainsComponent<RelationshipComponent>())
				return;

			// Copied by value: recursing calls AddEntity, which can reallocate the dense storage.
			auto children = root.GetComponent<RelationshipComponent>().Children;
			for (UUID childUUID : children)
				CollectSubtreeUUIDs(scene, scene->GetEntity(childUUID), outUUIDs);
		}

		// Entity references that point inside the duplicated subtree must follow the copy.
		// Without this a duplicated button still tints the original's graphic and navigates to
		// the original's siblings. References outside the subtree are intentionally left alone.
		static void RemapEntityReferences(Scene* scene, Entity root, const std::unordered_map<UUID, UUID>& remap)
		{
			std::vector<UUID> subtree;
			CollectSubtreeUUIDs(scene, root, subtree);

			auto remapOne = [&remap](UUID& handle)
				{
					auto it = remap.find(handle);
					if (it != remap.end())
						handle = it->second;
				};

			for (UUID uuid : subtree)
			{
				Entity entity = scene->GetEntity(uuid);
				if (entity == Constants::Entities::InvalidEntityID)
					continue;

				if (entity.ContainsComponent<UISelectableComponent>())
				{
					auto& selectable = entity.GetComponent<UISelectableComponent>();
					remapOne(selectable.TargetGraphicEntity);
					remapOne(selectable.NavigateUp);
					remapOne(selectable.NavigateDown);
					remapOne(selectable.NavigateLeft);
					remapOne(selectable.NavigateRight);
				}

				if (entity.ContainsComponent<UIToggleComponent>())
				{
					auto& toggle = entity.GetComponent<UIToggleComponent>();
					remapOne(toggle.CheckmarkEntity);
					remapOne(toggle.GroupEntity);
				}
			}
		}

		static void MoveUUIDToBack(std::vector<UUID>& uuids, UUID uuid)
		{
			EraseUUID(uuids, uuid);
			uuids.push_back(uuid);
		}

		static void MoveUUIDRelative(std::vector<UUID>& uuids, UUID movingUUID, UUID targetUUID, bool insertAfter)
		{
			if (movingUUID == targetUUID)
				return;

			EraseUUID(uuids, movingUUID);

			auto targetIt = std::find(uuids.begin(), uuids.end(), targetUUID);
			if (targetIt == uuids.end())
			{
				uuids.push_back(movingUUID);
				return;
			}

			if (insertAfter)
				++targetIt;

			uuids.insert(targetIt, movingUUID);
		}

		// Clears all runtime-only physics pointer fields on an entity so that the
		// PhysicsSystem will create fresh, independent objects for it.
		static void ResetPhysicsRuntimeState(Entity entity)
		{
			if (entity.ContainsComponent<RigidBodyComponent>())
				entity.GetComponent<RigidBodyComponent>().Body = nullptr;

			if (entity.ContainsComponent<BoxColliderComponent>())
			{
				auto& c = entity.GetComponent<BoxColliderComponent>();
				c.Shape = nullptr;
				c.Collider = nullptr;
				c.AttachedBody = nullptr;
				c.NeedsRebuild = false;
			}

			if (entity.ContainsComponent<SphereColliderComponent>())
			{
				auto& c = entity.GetComponent<SphereColliderComponent>();
				c.Shape = nullptr;
				c.Collider = nullptr;
				c.AttachedBody = nullptr;
				c.NeedsRebuild = false;
			}

			if (entity.ContainsComponent<CapsuleColliderComponent>())
			{
				auto& c = entity.GetComponent<CapsuleColliderComponent>();
				c.Shape = nullptr;
				c.Collider = nullptr;
				c.AttachedBody = nullptr;
				c.NeedsRebuild = false;
			}

			if (entity.ContainsComponent<ConvexMeshColliderComponent>())
			{
				auto& c = entity.GetComponent<ConvexMeshColliderComponent>();
				c.Shape = nullptr;
				c.Collider = nullptr;
				c.AttachedBody = nullptr;
				c.NeedsRebuild = false;
				c.PhysicsVertices.clear();
				c.RP3DVertexArray = nullptr;
			}

			if (entity.ContainsComponent<ConcaveMeshColliderComponent>())
			{
				auto& c = entity.GetComponent<ConcaveMeshColliderComponent>();
				c.Shape = nullptr;
				c.Collider = nullptr;
				c.AttachedBody = nullptr;
				c.NeedsRebuild = false;
				c.PhysicsVertices.clear();
				c.PhysicsIndices.clear();
				c.TriangleArray = nullptr;
				c.TriangleMesh = nullptr;
			}
		}

		static void InitializeAnimationPoseCaches(Scene* scene)
		{
			auto animationSystem = Application::Instance().GetSystemManager().GetSystem<AnimationSystem>();
			if (!animationSystem)
				return;

			View animatorView = scene->GetRegistry().ActiveQuery<AnimatorComponent>();
			for (EntityID entityID : animatorView)
			{
				Entity animatorEntity{ entityID, scene };
				auto& animator = animatorEntity.GetComponent<AnimatorComponent>();
				if (animator.LayerStates.empty())
					animator.LayerStates.emplace_back();
				animationSystem->SetStateToTimestamp(scene, animator.LayerStates[0].CurrentStateId, animatorEntity, animator.LayerStates[0].CurrentTime.Seconds());
			}
		}
	}

	Scene::Scene(const std::string& name, const std::string& filePath)
		: Scene(UUID(), name, filePath)
	{
	}

	Scene::Scene(UUID uuid, const std::string& name, const std::string& filePath)
		: Asset(uuid, name, filePath, AssetType::Scene), m_Registry(ScopedPtr<Registry>::Create()), m_PoolManager(ScopedPtr<PoolManager>::Create())
	{

	}

	Scene::~Scene()
	{
	}

	void Scene::Clear()
	{
		m_PendingRemovals.clear();
		m_EntityUUIDMap.clear();
		m_EntityOrder.clear();
		m_PoolManager = ScopedPtr<PoolManager>::Create();
		m_Registry = ScopedPtr<Registry>::Create();
	}

	// Deep-copies the scene, preserving all UUIDs so relationships remain valid
	SharedPtr<Scene> Scene::CopyScene(SharedPtr<Scene> other)
	{
		auto newScene = SharedPtr<Scene>::Create(other->GetName(), other->GetFilePath());
		newScene->SetLoaded(other->IsLoaded());

		auto entities = other->GetAllEntities();
		for (Entity srcEntity : entities)
		{
			// 2. Create a new entity in THIS scene using the EXACT SAME UUID and Name
			UUID id = srcEntity.GetComponent<IDComponent>().ID;
			std::string name = srcEntity.GetName();

			// Add UI or Normal entity based on the RectTransformComponent presence
			Entity destEntity = newScene->AddEntity(id, name);

			// Copy by runtime component type so new component types are picked up automatically.
			// IDComponent and TagComponent are excluded: AddEntity above already set both.
			newScene->GetRegistry().CopyComponents(other->GetRegistry(), srcEntity.GetEntityHandle(),
				destEntity.GetEntityHandle(), Exclude<IDComponent, TagComponent>{});

			// Initialize navigation mesh runtime state for the new scene
			if (srcEntity.ContainsComponent<NavigationMeshComponent>())
			{
				auto& navMeshComp = destEntity.GetComponent<NavigationMeshComponent>();
				if (navMeshComp.NavMeshDataHandle != Constants::InvalidUUID)
				{
					SharedPtr<NavigationMeshData> navMeshAsset = Application::Instance().GetAssetManager().GetAsset<NavigationMeshData>(navMeshComp.NavMeshDataHandle);
					if (navMeshAsset)
						navMeshAsset->InitializeFromRawData();
					else
						EB_CORE_WARN("CopyScene: NavigationMeshComponent on entity '{}' has an invalid NavMeshDataHandle!", destEntity.GetName());
				}
				else
					EB_CORE_WARN("CopyScene: NavigationMeshComponent on entity '{}' has no NavMeshDataHandle!", destEntity.GetName());
			}

			// Reset physics runtime pointers so the new scene doesn't alias the source scene's physics objects
			Utils::ResetPhysicsRuntimeState(destEntity);
		}

		// Copy registry assets and systems to new scene

		return newScene;
	}

	void Scene::OnAttach()
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<PhysicsSystem>()->OnSceneAttach(this);

		// Editor mode doesn't tick AnimationSystem every frame, so initialize all
		// animator bone matrices once on attach to avoid stale identity skinning.
		Utils::InitializeAnimationPoseCaches(this);

		EB_CORE_INFO("Scene '{}' attached!", m_Name);
	}

	void Scene::OnDetach()
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<PhysicsSystem>()->OnSceneDetach(this);

		EB_CORE_INFO("Scene '{}' detached!", m_Name);
	}

	void Scene::OnRuntimeStart()
	{
		m_IsRuntime = true;

		// A session that ended while paused must not start the next one paused.
		m_Paused = false;

		// Initialize systems (TODO: Loop over them and make system use scene->IsRuntime() to decide what to do in their OnSceneAttach
		auto& systemManager = Application::Instance().GetSystemManager();
		Utils::InitializeAnimationPoseCaches(this);
		systemManager.GetSystem<PhysicsSystem>()->OnSceneAttach(this);
		systemManager.GetSystem<RenderSystem>()->OnSceneAttach(this);
		systemManager.GetSystem<AudioSystem>()->OnSceneAttach(this);
		systemManager.GetSystem<AISystem>()->OnSceneAttach(this);

		// Initialize Pools
		auto view = m_Registry->ActiveQuery<PoolConfigComponent>();
		for (auto entity : view)
		{
			auto& config = m_Registry->GetComponent<PoolConfigComponent>(entity);
			m_PoolManager->CreatePool(this, config.PoolID, config.PrefabHandle, config.Capacity, config.LoopEntities);
		}

		// Initialize animator blackboards from their controllers now that all assets are guaranteed to be loaded
		auto animatorView = m_Registry->ActiveQuery<AnimatorComponent>();
		for (auto entity : animatorView)
		{
			auto& animator = m_Registry->GetComponent<AnimatorComponent>(entity);
			// Only reinitialize if blackboard is empty (in case it wasn't initialized during deserialization)
			if (animator.Blackboard.Parameters.empty())
			{
				animator.InitializeBlackboardFromController();
			}
		}

		// Initialize nav meshes
		auto navMeshView = m_Registry->ActiveQuery<NavigationMeshComponent>();
		for (auto entity : navMeshView)
		{
			auto& navMeshComp = m_Registry->GetComponent<NavigationMeshComponent>(entity);
			if (navMeshComp.NavMeshDataHandle != Constants::InvalidUUID)
			{
				SharedPtr<NavigationMeshData> navMeshAsset = Application::Instance().GetAssetManager().GetAsset<NavigationMeshData>(navMeshComp.NavMeshDataHandle);
				if (navMeshAsset)
					navMeshAsset->InitializeFromRawData();
				else
					EB_CORE_WARN("Scene::OnRuntimeStart: NavigationMeshComponent on entity '{}' has an invalid NavMeshDataHandle!", Entity{ entity, this }.GetName());
			}
			else
				EB_CORE_WARN("Scene::OnRuntimeStart: NavigationMeshComponent on entity '{}' has no NavMeshDataHandle!", Entity{ entity, this }.GetName());
		}

		ScriptEngine::OnRuntimeStart(this);
	}

	void Scene::OnRuntimeStop()
	{
		m_IsRuntime = false;
		m_Paused = false;

		auto& systemManager = Application::Instance().GetSystemManager();
		systemManager.GetSystem<AudioSystem>()->OnSceneDetach(this);
		systemManager.GetSystem<VisibilitySystem>()->OnSceneDetach(this);

		m_PoolManager->DestroyPools();
		ScriptEngine::OnRuntimeStop(this);

		systemManager.GetSystem<PhysicsSystem>()->OnSceneDetach(this);

		auto entityView = m_Registry->Query<IDComponent>();
		for (auto entityID : entityView)
		{
			Entity entity{ entityID, this };
			Utils::ResetPhysicsRuntimeState(entity);
		}

		systemManager.GetSystem<ParticleSystem>()->GetParticleManager().Reset();
	}

	void Scene::OnUpdateRuntime(TimeStep delta)
	{
		EB_PROFILE_FUNCTION();

		auto& systemManager = Application::Instance().GetSystemManager();

		// Pausing starves the simulation systems of time while the presentation ones keep the real
		// delta, so menus, audio and rendering carry on over a frozen world. ScriptSystem is handed
		// the real delta too - it gates itself per entity off ScriptComponent::RunWhenPaused.
		const TimeStep simDelta = m_Paused ? TimeStep(0.0f) : delta;

		{
			EB_PROFILE_SCOPE("LifecycleSystem::OnUpdate");
			systemManager.GetSystem<LifecycleSystem>()->OnUpdate(simDelta, this);
		}
		{
			// Ahead of ScriptSystem so a script polling IsPointerOverUI() reads this frame,
			// not the last. It hit-tests against the previous frame's rects, as Unity does.
			EB_PROFILE_SCOPE("UIInputSystem::OnUpdate");
			systemManager.GetSystem<UIInputSystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("ScriptSystem::OnUpdate");
			systemManager.GetSystem<ScriptSystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("AISystem::OnUpdate");
			systemManager.GetSystem<AISystem>()->OnUpdate(simDelta, this);
		}
		{
			EB_PROFILE_SCOPE("CharacterControllerSystem::OnUpdate");
			systemManager.GetSystem<CharacterControllerSystem>()->OnUpdate(simDelta, this);
		}
		{
			// Runs before AnimationSystem so its off-screen relevance results are available this frame.
			// It culls against last frame's transforms (TransformSystem runs later) but dilates the
			// frustum, so the visible set stays a conservative superset of what actually renders.
			EB_PROFILE_SCOPE("VisibilitySystem::OnUpdate");
			systemManager.GetSystem<VisibilitySystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("AnimationSystem::OnUpdate");
			systemManager.GetSystem<AnimationSystem>()->OnUpdate(simDelta, this);
		}
		{
			EB_PROFILE_SCOPE("PhysicsSystem::OnUpdate");
			systemManager.GetSystem<PhysicsSystem>()->OnUpdate(simDelta, this);
		}
		{
			EB_PROFILE_SCOPE("ParticleSystem::OnUpdate");
			systemManager.GetSystem<ParticleSystem>()->OnUpdate(simDelta, this);
		}
		{
			EB_PROFILE_SCOPE("TransformSystem::OnUpdate");
			systemManager.GetSystem<TransformSystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("BoneSocketSystem::OnUpdate");
			systemManager.GetSystem<BoneSocketSystem>()->OnUpdate(simDelta, this);
		}
		{
			EB_PROFILE_SCOPE("UILayoutSystem::OnUpdate");
			systemManager.GetSystem<UILayoutSystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("RenderSystem::OnUpdate");
			systemManager.GetSystem<RenderSystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("AudioSystem::OnUpdate");
			systemManager.GetSystem<AudioSystem>()->OnUpdate(delta, this);
		}

		{
			EB_PROFILE_SCOPE("Scene::RemovePendingRemovals");
			RemovePendingRemovals();
		}
	}

	void Scene::OnUpdateEdit(TimeStep delta, const RenderPassSettings& settings)
	{
		EB_PROFILE_FUNCTION();

		auto& systemManager = Application::Instance().GetSystemManager();
		{
			EB_PROFILE_SCOPE("TransformSystem::OnUpdate");
			systemManager.GetSystem<TransformSystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("BoneSocketSystem::OnUpdate");
			systemManager.GetSystem<BoneSocketSystem>()->OnUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("PhysicsSystem::OnEditorUpdate");
			systemManager.GetSystem<PhysicsSystem>()->OnEditorUpdate(delta, this);
		}
		{
			EB_PROFILE_SCOPE("AISystem::OnEditorUpdate");
			systemManager.GetSystem<AISystem>()->OnEditorUpdate(delta, this);
		}

		// May need specific OnEditorUpdate method, we will need to see
		{
			EB_PROFILE_SCOPE("UILayoutSystem::OnUpdate");
			systemManager.GetSystem<UILayoutSystem>()->OnUpdate(delta, this);
		}

		{
			EB_PROFILE_SCOPE("RenderSystem::OnUpdate");
			systemManager.GetSystem<RenderSystem>()->OnUpdate(delta, this, settings);
		}

		{
			EB_PROFILE_SCOPE("Scene::RemovePendingRemovals");
			RemovePendingRemovals();
		}
	}

	void Scene::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(WindowResizeEvent, OnWindowResize);
	}

	void Scene::SetActiveCamera(Entity cameraEntity)
	{
		EB_CORE_ASSERT(cameraEntity.ContainsComponent<CameraComponent>(), "Entity '{}' does not have a CameraComponent!", cameraEntity.GetName());

		// Loop through and disable all camera, and enable the one we want
		auto view = m_Registry->ActiveQuery<CameraComponent>();
		for (auto entity : view)
		{
			auto& camera = m_Registry->GetComponent<CameraComponent>(entity);
			camera.IsActive = (entity == cameraEntity.GetEntityHandle());
		}
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return;

		EB_CORE_INFO("Viewport resized to {}x{} in scene '{}'", width, height, m_Name);
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

		auto uiLayoutSystem = systemManager.GetSystem<UILayoutSystem>();
		if (uiLayoutSystem)
		{
			uiLayoutSystem->OnViewportResize(this, width, height);
		}
	}

	Vector2f Scene::GetViewportSize() const
	{
		auto renderSystem = Application::Instance().GetSystemManager().GetSystem<RenderSystem>();
		return renderSystem ? renderSystem->GetViewportSize() : Vector2f(0.0f, 0.0f);
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
		m_EntityOrder.push_back(uuid);

		return entity;
	}

	Entity Scene::GetEntity(UUID uuid)
	{
		auto it = m_EntityUUIDMap.find(uuid);
		if (it != m_EntityUUIDMap.end())
			return { it->second, this };

		return Entity();
	}

	Entity Scene::GetEntityByHandle(EntityID entityID)
	{
		if (entityID == Constants::Entities::InvalidEntityID || entityID >= Constants::Entities::MaxEntities || !m_Registry)
			return Entity();

		if (!m_Registry->ContainsComponent<IDComponent>(entityID) || !m_Registry->ContainsComponent<TagComponent>(entityID))
			return Entity();

		UUID uuid = m_Registry->GetComponent<IDComponent>(entityID).ID;
		auto it = m_EntityUUIDMap.find(uuid);
		if (it == m_EntityUUIDMap.end() || it->second != entityID)
			return Entity();

		return { entityID, this };
	}

	Entity Scene::GetEntity(const std::string& name)
	{
		auto view = m_Registry->Query<TagComponent>();
		for (auto entity : view)
		{
			auto& tag = m_Registry->GetComponent<TagComponent>(entity);
			if (tag.Tag == name)
				return { entity, this };
		}
		return Entity();
	}

	bool Scene::TryGetEntityName(UUID uuid, std::string& outName)
	{
		auto it = m_EntityUUIDMap.find(uuid);
		if (it == m_EntityUUIDMap.end())
			return false;

		EntityID entityID = it->second;
		Entity entity = GetEntityByHandle(entityID);
		if (entity == Constants::Entities::InvalidEntityID)
			return false;

		outName = m_Registry->GetComponent<TagComponent>(entityID).Tag;
		return true;
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		std::vector<UUID> originalUUIDs;
		Utils::CollectSubtreeUUIDs(this, entity, originalUUIDs);

		Entity newEntity = DuplicateEntityRecursive(entity, entity.GetComponent<RelationshipComponent>().ParentHandle, true);

		std::vector<UUID> duplicatedUUIDs;
		Utils::CollectSubtreeUUIDs(this, newEntity, duplicatedUUIDs);

		// Both walks are depth-first over the same shape, so equal sizes means index i in one
		// corresponds to index i in the other.
		if (originalUUIDs.size() == duplicatedUUIDs.size())
		{
			std::unordered_map<UUID, UUID> remap;
			for (size_t i = 0; i < originalUUIDs.size(); i++)
				remap[originalUUIDs[i]] = duplicatedUUIDs[i];

			Utils::RemapEntityReferences(this, newEntity, remap);
		}

		return newEntity;
	}

	SharedPtr<Prefab> Scene::CreatePrefab(Entity entity, const std::string& filepath)
	{
		if (entity == Constants::Entities::InvalidEntityID || filepath.empty())
			return nullptr;

		// Serialize the entity and its children to a prefab file
		SceneSerializer serializer(this);
		if (!serializer.SerializePrefab(entity, filepath))
			return nullptr;

		// Load entity as a prefab asset
		auto& assetManager = Application::Instance().GetAssetManager();
		auto prefab = assetManager.Load<Prefab>(filepath);
		prefab->SetIsEngineAsset(false);

		// Save asset metadata for the prefab
		AssetRegistrySerializer assetSerializer(&Application::Instance().GetAssetManager());
		assetSerializer.Serialize(ProjectManager::GetActive()->GetAssetsFilePath().string());

		// Add prefab component to entity
		auto& pc = entity.AttachComponent<PrefabComponent>();
		pc.PrefabHandle = prefab->GetUUID();

		// Re-serialize now that the root carries its PrefabComponent. Without this the saved YAML has
		// no prefab link, so every instance placed from it is an orphan that a prefab edit can never
		// find - the asset's UUID is only known after the first save, hence the second pass.
		if (serializer.SerializePrefab(entity, filepath))
		{
			std::ifstream stream(filepath);
			prefab->YAMLData = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
		}

		return prefab;
	}

	void Scene::SetEntityParent(UUID childUUID, Entity newParent)
	{
		EB_CORE_ASSERT(newParent.ContainsComponent<RelationshipComponent>(), "New parent entity must have a RelationshipComponent!");
		auto childEntity = GetEntity(childUUID);
		if (childEntity == Constants::Entities::InvalidEntityID)
			return;

		auto& childRelationship = childEntity.GetComponent<RelationshipComponent>();
		UUID newParentUUID = newParent.GetUUID();
		if (childRelationship.ParentHandle == newParentUUID)
			return;

		// If the child had a previous parent, we need to remove it from that parent's children list
		if (childRelationship.ParentHandle != Constants::InvalidUUID)
		{
			Entity oldParent = GetEntity(childRelationship.ParentHandle);
			if (oldParent != Constants::Entities::InvalidEntityID)
			{
				auto& oldParentRelationship = oldParent.GetComponent<RelationshipComponent>();
				Utils::EraseUUID(oldParentRelationship.Children, childUUID);
			}
		}

		auto& parentRelationship = newParent.GetComponent<RelationshipComponent>();
		Utils::MoveUUIDToBack(parentRelationship.Children, childUUID);

		// Set new parent handle
		childRelationship.ParentHandle = newParentUUID;

		// Recompute the child's local transform relative to the new parent (if not in screen space)
		// NewLocal = Inverse(ParentWorld) * CurrentChildWorld
		auto& parentTransform = newParent.GetComponent<TransformComponent>();
		auto& childTransform = childEntity.GetComponent<TransformComponent>();

		Matrix4f parentInverseWorld = Math::Inverse(parentTransform.WorldTransform);
		Matrix4f currentChildWorld = childTransform.WorldTransform;

		// Calculate the exact local matrix needed to maintain the current world position
		Matrix4f newLocalTransform = parentInverseWorld * currentChildWorld;

		Vector3f outPos, outRot, outScale;
		Math::DecomposeTransform(newLocalTransform, outPos, outRot, outScale);

		// Directly assign the decomposed values! No addition!
		childTransform.Position = outPos;
		childTransform.Rotation = outRot;
		childTransform.Scale = outScale;

		// Usually the assignments above already make the transform dirty, but not always: re-parenting
		// under an identity parent can reproduce the exact same local TRS, and IsLocalDirty() would
		// then report clean while the world matrix is stale. Invalidate explicitly so the rebuild is
		// driven by the parent change itself rather than by a coincidence in the numbers.
		childTransform.InvalidateWorld();
	}

	void Scene::RemoveParent(Entity child)
	{
		if (child == Constants::Entities::InvalidEntityID)
			return;

		auto& relationship = child.GetComponent<RelationshipComponent>();
		if (relationship.ParentHandle != Constants::InvalidUUID)
		{
			auto parentEntity = GetEntity(relationship.ParentHandle);
			if (parentEntity != Constants::Entities::InvalidEntityID)
			{
				auto& parentRelationship = parentEntity.GetComponent<RelationshipComponent>();
				Utils::EraseUUID(parentRelationship.Children, child.GetUUID());
			}

			relationship.ParentHandle = Constants::InvalidUUID;

			// The child's local TRS now resolves against an identity parent instead of its old one,
			// which IsLocalDirty() cannot see - force the world transform to be rebuilt.
			child.GetComponent<TransformComponent>().InvalidateWorld();
		}
	}

	void Scene::ReorderEntity(UUID entityUUID, UUID targetUUID, bool insertAfter)
	{
		if (entityUUID == targetUUID)
			return;

		Entity entity = GetEntity(entityUUID);
		Entity target = GetEntity(targetUUID);
		if (entity == Constants::Entities::InvalidEntityID || target == Constants::Entities::InvalidEntityID)
			return;

		UUID targetParentUUID = target.GetComponent<RelationshipComponent>().ParentHandle;
		Entity targetParent;
		if (targetParentUUID != Constants::InvalidUUID)
		{
			if (targetParentUUID == entityUUID)
				return;

			targetParent = GetEntity(targetParentUUID);
			if (targetParent == Constants::Entities::InvalidEntityID)
				return;

			Entity current = targetParent;
			while (current != Constants::Entities::InvalidEntityID)
			{
				if (current.GetUUID() == entityUUID)
					return;

				UUID parentUUID = current.GetComponent<RelationshipComponent>().ParentHandle;
				if (parentUUID == Constants::InvalidUUID)
					break;

				current = GetEntity(parentUUID);
			}
		}

		auto& relationship = entity.GetComponent<RelationshipComponent>();
		if (relationship.ParentHandle != targetParentUUID)
		{
			if (targetParentUUID == Constants::InvalidUUID)
				RemoveParent(entity);
			else
				SetEntityParent(entityUUID, targetParent);
		}

		if (targetParentUUID == Constants::InvalidUUID)
		{
			Utils::MoveUUIDRelative(m_EntityOrder, entityUUID, targetUUID, insertAfter);
		}
		else
		{
			auto& siblings = targetParent.GetComponent<RelationshipComponent>().Children;
			Utils::MoveUUIDRelative(siblings, entityUUID, targetUUID, insertAfter);
		}
	}

	void Scene::MoveEntityToRootEnd(UUID entityUUID)
	{
		Entity entity = GetEntity(entityUUID);
		if (entity == Constants::Entities::InvalidEntityID)
			return;

		RemoveParent(entity);
		Utils::MoveUUIDToBack(m_EntityOrder, entityUUID);
	}

	// Recursively duplicates an entity and its children.
	// isRoot: true for the top-level call so it keeps the same parent; false for descendants.
	// originalAnimatorUUID/newAnimatorUUID track the UUID remapping for AnimatorComponent owners
	// so SkinnedMeshComponent handles are updated inline rather than repaired after the fact.
	Entity Scene::DuplicateEntityRecursive(Entity entity, UUID newParentId, bool isRoot, UUID originalAnimatorUUID, UUID newAnimatorUUID)
	{
		// Find a unique name for the duplicated entity by appending a number in parentheses if needed
		std::string baseName = StringUtils::GetBaseName(entity.GetName());
		int freeIndex = 1;
		std::string newName;
		do 
			newName = std::format("{}({})", baseName, freeIndex++);
		while (GetEntity(newName) != Constants::Entities::InvalidEntityID);

		Entity newEntity = AddEntity(newName);

		// Copy by runtime component type so new component types are picked up automatically, and
		// in the source's component order so the duplicate keeps its inspector layout. Identity is
		// owned by AddEntity above and the hierarchy by the parenting fix-up below.
		m_Registry->CopyComponents(entity.GetEntityHandle(), newEntity.GetEntityHandle(),
			Exclude<IDComponent, TagComponent, RelationshipComponent>{});

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

		// Reset all runtime-only physics state copied from the source entity.
		// Without this, the attach hooks saw non-null pointers and skipped creation,
		// leaving both entities sharing the same physics objects.
		Utils::ResetPhysicsRuntimeState(newEntity);

		// Set up the relationship ParentHandle BEFORE initializing physics so that
		// FindRigidBodyEntity can correctly climb the parent chain (e.g. a child
		// collider whose rigid body lives on an ancestor).
		auto oldRels = entity.GetComponent<RelationshipComponent>();

		auto& newRels = newEntity.AttachComponent<RelationshipComponent>();
		if (isRoot)
		{
			newRels.ParentHandle = oldRels.ParentHandle;

			if (newParentId != Constants::InvalidUUID)
			{
				Entity newParent = GetEntity(newParentId);
				if (newParent != Constants::Entities::InvalidEntityID)
					newParent.GetComponent<RelationshipComponent>().Children.push_back(newEntity.GetUUID());
			}
		}
		else
		{
			newRels.ParentHandle = newParentId;
		}

		// The duplicated entity now resolves against whatever parent it was given, so its copied
		// world transform is stale until the next transform pass rebuilds it.
		newEntity.GetComponent<TransformComponent>().InvalidateWorld();

		// Create fresh, independent physics objects for this entity
		auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
		physicsSystem->InitializeEntity(newEntity.GetEntityHandle(), this);

		// Collect the new child UUIDs locally: each recursion calls AddEntity, which attaches a
		// RelationshipComponent and can reallocate its dense storage, invalidating `newRels`.
		std::vector<UUID> duplicatedChildUUIDs;
		duplicatedChildUUIDs.reserve(oldRels.Children.size());

		for (UUID childUUID : oldRels.Children)
		{
			Entity childEntity = GetEntity(childUUID);
			if (childEntity != Constants::Entities::InvalidEntityID)
			{
				Entity duplicatedChild = DuplicateEntityRecursive(childEntity, newEntity.GetUUID(), false, originalAnimatorUUID, newAnimatorUUID);
				duplicatedChildUUIDs.push_back(duplicatedChild.GetUUID());
			}
		}

		// Re-fetch after the recursion; see above. `newRels` must not be touched past this point.
		if (!duplicatedChildUUIDs.empty())
		{
			auto& refreshedRelationship = newEntity.GetComponent<RelationshipComponent>();
			refreshedRelationship.Children.insert(refreshedRelationship.Children.end(),
				duplicatedChildUUIDs.begin(), duplicatedChildUUIDs.end());
		}

		return newEntity;
	}

	std::vector<Entity> Scene::GetAllEntities() const
	{
		std::vector<Entity> entities;
		entities.reserve(m_EntityUUIDMap.size());

		for (UUID uuid : m_EntityOrder)
		{
			auto it = m_EntityUUIDMap.find(uuid);
			if (it != m_EntityUUIDMap.end())
				entities.emplace_back(it->second, const_cast<Scene*>(this));
		}

		for (const auto& [uuid, id] : m_EntityUUIDMap)
		{
			if (std::find(m_EntityOrder.begin(), m_EntityOrder.end(), uuid) == m_EntityOrder.end())
				entities.emplace_back(id, const_cast<Scene*>(this));
		}

		return entities;
	}

	void Scene::ResetAllPhysicsState()
	{
		auto entityView = m_Registry->Query<IDComponent>();
		for (auto entityID : entityView)
		{
			Entity entity{ entityID, this };
			Utils::ResetPhysicsRuntimeState(entity);
		}
	}

	void Scene::RemoveEntityFromScene(Entity entity)
	{
		// Removing a parent recurses into its children, so a child queued for removal alongside its
		// own parent arrives here a second time with its slot already released.
		EntityID handle = entity.GetEntityHandle();
		if (handle == Constants::Entities::InvalidEntityID || handle >= Constants::Entities::MaxEntities)
			return;

		if (!m_Registry->ContainsComponent<IDComponent>(handle))
			return;

		if (m_EntityUUIDMap.find(entity.GetUUID()) == m_EntityUUIDMap.end())
			return;

		// Copy child UUIDs first to avoid iterating a component that gets modified during recursion
		std::vector<UUID> childUUIDs = entity.GetComponent<RelationshipComponent>().Children;
		for (UUID childUUID : childUUIDs)
		{
			Entity childEntity = GetEntity(childUUID);
			RemoveEntityFromScene(childEntity);
		}

		// If contains a RigidBodyComponent, detach all colliders first (while the body is still alive
		// so the component-detach hooks can safely call body->removeCollider), then destroy the body.
		if (entity.ContainsComponent<RigidBodyComponent>())
		{
			// Detach collider components so their hooks run before the body is destroyed
			if (entity.ContainsComponent<BoxColliderComponent>())
				entity.DetachComponent<BoxColliderComponent>();
			if (entity.ContainsComponent<SphereColliderComponent>())
				entity.DetachComponent<SphereColliderComponent>();
			if (entity.ContainsComponent<CapsuleColliderComponent>())
				entity.DetachComponent<CapsuleColliderComponent>();
			if (entity.ContainsComponent<ConvexMeshColliderComponent>())
				entity.DetachComponent<ConvexMeshColliderComponent>();
			if (entity.ContainsComponent<ConcaveMeshColliderComponent>())
				entity.DetachComponent<ConcaveMeshColliderComponent>();

			auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
			auto& rigidBody = entity.GetComponent<RigidBodyComponent>();
			physicsSystem->RemoveRigidBody(rigidBody);
		}

		// Remove from parent
		RemoveParent(entity);

		// Remove from ECS and our Map
		UUID entityUUID = entity.GetUUID();
		m_Registry->DestroyEntity(entity.GetEntityHandle());
		m_EntityUUIDMap.erase(entityUUID);
		Utils::EraseUUID(m_EntityOrder, entityUUID);
	}

	void Scene::RemoveEntity(Entity entity)
	{
		if (entity.ContainsComponent<PoolComponent>())
		{
			auto& poolComp = entity.GetComponent<PoolComponent>();
			m_PoolManager->ReturnToPool(entity, poolComp.PoolID);
			return;
		}

		m_PendingRemovals.push_back(entity);
	}

	void Scene::RemovePendingRemovals()
	{
		for (Entity entity : m_PendingRemovals)
			RemoveEntityFromScene(entity);

		m_PendingRemovals.clear();
	}

	void Scene::FlushPendingRemovals()
	{
		RemovePendingRemovals();
	}

	Entity Scene::GetEntityAtPixel(uint32_t x, uint32_t y)
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		auto renderSystem = systemManager.GetSystem<RenderSystem>();
		EntityID id = renderSystem->GetEntityIDAtPixel(x, y);
		return id != Constants::Entities::InvalidEntityID ? Entity(id, this) : Entity();
	}

	void Scene::ResetEntityToCoreComponents(Entity entity)
	{
		if (entity.GetEntityHandle() == Constants::Entities::InvalidEntityID)
			return;

		// Colliders must come off before the body they were registered with, mirroring the ordering
		// RemoveEntityFromScene relies on.
		if (entity.ContainsComponent<RigidBodyComponent>())
		{
			if (entity.ContainsComponent<BoxColliderComponent>())
				entity.DetachComponent<BoxColliderComponent>();
			if (entity.ContainsComponent<SphereColliderComponent>())
				entity.DetachComponent<SphereColliderComponent>();
			if (entity.ContainsComponent<CapsuleColliderComponent>())
				entity.DetachComponent<CapsuleColliderComponent>();
			if (entity.ContainsComponent<ConvexMeshColliderComponent>())
				entity.DetachComponent<ConvexMeshColliderComponent>();
			if (entity.ContainsComponent<ConcaveMeshColliderComponent>())
				entity.DetachComponent<ConcaveMeshColliderComponent>();

			auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
			if (physicsSystem)
				physicsSystem->RemoveRigidBody(entity.GetComponent<RigidBodyComponent>());

			entity.DetachComponent<RigidBodyComponent>();
		}

		// Everything else comes off generically, so a component added later cannot be forgotten here.
#define EMBER_DETACH_NON_CORE_COMPONENT(Component) \
		if constexpr (!std::is_same_v<Component, IDComponent> && !std::is_same_v<Component, TagComponent> \
			&& !std::is_same_v<Component, TransformComponent> && !std::is_same_v<Component, RelationshipComponent> \
			&& !std::is_same_v<Component, RigidBodyComponent>) \
		{ \
			if (entity.ContainsComponent<Component>()) \
				entity.DetachComponent<Component>(); \
		}

		EMBER_FOR_EACH_COMPONENT_ORDER_TYPE(EMBER_DETACH_NON_CORE_COMPONENT)
#undef EMBER_DETACH_NON_CORE_COMPONENT

		// DeserializeEntityNode appends children rather than assigning them, so a stale list would
		// double up on restore.
		if (entity.ContainsComponent<RelationshipComponent>())
		{
			auto& relationship = entity.GetComponent<RelationshipComponent>();
			relationship.Children.clear();
			relationship.ParentHandle = Constants::InvalidUUID;
		}
	}

	void Scene::SetRootEntityIndex(UUID entityUUID, size_t index)
	{
		Utils::EraseUUID(m_EntityOrder, entityUUID);

		index = std::min(index, m_EntityOrder.size());
		m_EntityOrder.insert(m_EntityOrder.begin() + index, entityUUID);
	}

	std::vector<Entity> Scene::FilterToHierarchyRoots(const std::vector<Entity>& entities)
	{
		std::unordered_set<UUID> candidates;
		candidates.reserve(entities.size());
		for (Entity entity : entities)
		{
			if (entity.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				candidates.insert(entity.GetUUID());
		}

		std::vector<Entity> roots;
		roots.reserve(entities.size());

		for (Entity entity : entities)
		{
			if (entity.GetEntityHandle() == Constants::Entities::InvalidEntityID)
				continue;

			bool ancestorPresent = false;
			Entity walker = entity;
			while (walker.ContainsComponent<RelationshipComponent>())
			{
				UUID parentUUID = walker.GetComponent<RelationshipComponent>().ParentHandle;
				if (parentUUID == Constants::InvalidUUID)
					break;

				if (candidates.contains(parentUUID))
				{
					ancestorPresent = true;
					break;
				}

				walker = GetEntity(parentUUID);
				if (walker.GetEntityHandle() == Constants::Entities::InvalidEntityID)
					break;
			}

			if (!ancestorPresent)
				roots.push_back(entity);
		}

		return roots;
	}

	bool Scene::GetWorldPositionAtPixel(uint32_t x, uint32_t y, Vector3f& outPosition)
	{
		auto& systemManager = Application::Instance().GetSystemManager();
		auto renderSystem = systemManager.GetSystem<RenderSystem>();
		return renderSystem && renderSystem->GetWorldPositionAtPixel(x, y, outPosition);
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

			rootEntity.AttachComponent<AnimatorComponent>(animator);
			animatorEntity = rootEntity.GetUUID(); // Save the UUID
		}

		ProcessModelNode(rootEntity, model->GetRootNode(), model, animatorEntity);
		return rootEntity;
	}

	static void InitializePrefabPhysics(EntityID entity, PhysicsSystem* physicsSystem, Scene* scene)
	{
		physicsSystem->InitializeEntity(entity, scene);

		auto& relationship = scene->GetRegistry().GetComponent<RelationshipComponent>(entity);
		for (UUID childUUID : relationship.Children)
		{
			Entity child = scene->GetEntity(childUUID);
			if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				InitializePrefabPhysics(child.GetEntityHandle(), physicsSystem, scene);
		}
	}

	static void SyncPrefabPhysicsTransforms(EntityID entity, Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		if (registry.ContainsComponent<RigidBodyComponent>(entity))
		{
			auto& rb = registry.GetComponent<RigidBodyComponent>(entity);
			if (rb.Body)
			{
				auto& transform = registry.GetComponent<TransformComponent>(entity);

				Vector3f worldPos, worldRot, worldScale;
				Math::DecomposeTransform(transform.WorldTransform, worldPos, worldRot, worldScale);
				Quaternion q = Math::ToQuaternion(worldRot);

				rb.Body->setTransform(rp3d::Transform(
					rp3d::Vector3(worldPos.x, worldPos.y, worldPos.z),
					rp3d::Quaternion(q.x, q.y, q.z, q.w)
				));
				rb.Body->setLinearVelocity(rp3d::Vector3(0.0f, 0.0f, 0.0f));
				rb.Body->setAngularVelocity(rp3d::Vector3(0.0f, 0.0f, 0.0f));
			}
		}

		auto& relationship = registry.GetComponent<RelationshipComponent>(entity);
		for (UUID childUUID : relationship.Children)
		{
			Entity child = scene->GetEntity(childUUID);
			if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				SyncPrefabPhysicsTransforms(child.GetEntityHandle(), scene);
		}
	}

	static void InitializePrefabScripts(EntityID entity, ScriptSystem* scriptSystem, Scene* scene)
	{
		Entity currentEntity(entity, scene);
		if (currentEntity.ContainsComponent<ScriptComponent>())
			scriptSystem->InitializeScriptForEntity(currentEntity);

		auto& relationship = scene->GetRegistry().GetComponent<RelationshipComponent>(entity);
		for (UUID childUUID : relationship.Children)
		{
			Entity child = scene->GetEntity(childUUID);
			if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				InitializePrefabScripts(child.GetEntityHandle(), scriptSystem, scene);
		}
	}

	static void InitializePrefabAIAgents(EntityID entity, AISystem* aiSystem, Scene* scene)
	{
		Entity currentEntity(entity, scene);
		if (currentEntity.ContainsComponent<AIAgentComponent>())
			aiSystem->ApplyAgentModeSettings(currentEntity, scene);

		auto& relationship = scene->GetRegistry().GetComponent<RelationshipComponent>(entity);
		for (UUID childUUID : relationship.Children)
		{
			Entity child = scene->GetEntity(childUUID);
			if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				InitializePrefabAIAgents(child.GetEntityHandle(), aiSystem, scene);
		}
	}

	static void InitializePrefabAnimationPoseCaches(EntityID entity, AnimationSystem* animationSystem, Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		if (animationSystem && registry.ContainsComponent<AnimatorComponent>(entity))
		{
			Entity animatorEntity{ entity, scene };
			auto& animator = registry.GetComponent<AnimatorComponent>(entity);

			// Initialize bone matrices immediately so newly instantiated skinned prefabs
			// render at correct size/pose even before the next AnimationSystem tick.
			if (animator.LayerStates.empty())
				animator.LayerStates.emplace_back();
			animationSystem->SetStateToTimestamp(scene, animator.LayerStates[0].CurrentStateId, animatorEntity, animator.LayerStates[0].CurrentTime.Seconds());
		}

		auto& relationship = registry.GetComponent<RelationshipComponent>(entity);
		for (UUID childUUID : relationship.Children)
		{
			Entity child = scene->GetEntity(childUUID);
			if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				InitializePrefabAnimationPoseCaches(child.GetEntityHandle(), animationSystem, scene);
		}
	}

	// Brings a freshly deserialized prefab hierarchy to the same runtime-ready state as a
	// scene-placed instance after CopyScene + OnRuntimeStart (physics, scripts, AI agents).
	static void FinalizeRuntimePrefabInstance(Scene* scene, Entity root, const Matrix4f& parentWorldTransform)
	{
		EB_PROFILE_FUNCTION();
		auto& systemManager = Application::Instance().GetSystemManager();

		// DeserializePrefab attaches components while physics hooks are live, which can create
		// bodies at the prefab's baked YAML transform. Tear those down and recreate below so
		// spawn position overrides and compound colliders match hand-placed scene entities.
		auto physicsSystem = systemManager.GetSystem<PhysicsSystem>();
		physicsSystem->TeardownHierarchyPhysics(root.GetEntityHandle(), scene);

		auto transformSystem = systemManager.GetSystem<TransformSystem>();
		// Force a full recompute of the freshly-instantiated prefab subtree.
		transformSystem->UpdateTransformTree(root.GetEntityHandle(), parentWorldTransform, true, scene);

		auto animationSystem = systemManager.GetSystem<AnimationSystem>();
		InitializePrefabAnimationPoseCaches(root.GetEntityHandle(), animationSystem.Ptr(), scene);

		InitializePrefabPhysics(root.GetEntityHandle(), physicsSystem.Ptr(), scene);
		SyncPrefabPhysicsTransforms(root.GetEntityHandle(), scene);

		if (scene->IsRuntime())
		{
			InitializePrefabScripts(root.GetEntityHandle(), systemManager.GetSystem<ScriptSystem>().Ptr(), scene);
			InitializePrefabAIAgents(root.GetEntityHandle(), systemManager.GetSystem<AISystem>().Ptr(), scene);
		}
	}

	namespace {

		// Path of names down from the subtree root ("" is the root, "Trim/Bolt" a grandchild), used to
		// line entities up across a refresh - their UUIDs are regenerated, so names are the only
		// stable handle. A prefab with duplicate sibling names resolves to the first match.
		void GatherByNamePath(Scene* scene, Entity entity, const std::string& prefix,
			std::unordered_map<std::string, Entity>& outByPath)
		{
			outByPath.emplace(prefix, entity);

			if (!entity.ContainsComponent<RelationshipComponent>())
				return;

			for (UUID childUUID : entity.GetComponent<RelationshipComponent>().Children)
			{
				Entity child = scene->GetEntity(childUUID);
				if (child.GetEntityHandle() == Constants::Entities::InvalidEntityID)
					continue;

				std::string childPath = prefix.empty() ? child.GetName() : prefix + "/" + child.GetName();
				GatherByNamePath(scene, child, childPath, outByPath);
			}
		}

		// Script property values are the one piece of per-instance data the engine already models as
		// overrides, so they are carried across a refresh rather than reverting to the prefab.
		struct SavedScriptOverrides
		{
			UUID ScriptHandle = Constants::InvalidUUID;
			std::unordered_map<std::string, ScriptProperty> Values;
		};

		std::unordered_map<std::string, SavedScriptOverrides> CaptureScriptOverrides(Scene* scene, Entity instanceRoot)
		{
			std::unordered_map<std::string, Entity> byPath;
			GatherByNamePath(scene, instanceRoot, "", byPath);

			std::unordered_map<std::string, SavedScriptOverrides> saved;
			for (auto& [path, entity] : byPath)
			{
				if (!entity.ContainsComponent<ScriptComponent>())
					continue;

				auto& script = entity.GetComponent<ScriptComponent>();
				if (script.UserPropertyOverrides.empty())
					continue;

				saved[path] = SavedScriptOverrides{ script.ScriptHandle, script.UserPropertyOverrides };
			}

			return saved;
		}

		void ReapplyScriptOverrides(Scene* scene, Entity instanceRoot,
			const std::unordered_map<std::string, SavedScriptOverrides>& saved)
		{
			if (saved.empty())
				return;

			std::unordered_map<std::string, Entity> byPath;
			GatherByNamePath(scene, instanceRoot, "", byPath);

			for (const auto& [path, savedOverrides] : saved)
			{
				auto it = byPath.find(path);
				if (it == byPath.end() || !it->second.ContainsComponent<ScriptComponent>())
					continue;

				auto& script = it->second.GetComponent<ScriptComponent>();

				// A different script means the saved values describe properties that no longer exist.
				if (script.ScriptHandle != savedOverrides.ScriptHandle)
					continue;

				// The instance's value wins; properties the prefab added since keep their new default.
				for (const auto& [name, property] : savedOverrides.Values)
					script.UserPropertyOverrides[name] = property;
			}
		}

	}

	uint32_t Scene::CountPrefabInstances(UUID prefabUUID) const
	{
		uint32_t count = 0;
		for (Entity entity : GetAllEntities())
		{
			if (entity.ContainsComponent<PrefabComponent>()
				&& entity.GetComponent<PrefabComponent>().PrefabHandle == prefabUUID)
				count++;
		}

		return count;
	}

	uint32_t Scene::RefreshPrefabInstances(const SharedPtr<Prefab>& prefab)
	{
		EB_PROFILE_FUNCTION();

		if (!prefab || prefab->YAMLData.empty())
			return 0;

		const UUID prefabUUID = prefab->GetUUID();

		// Gathered up front by UUID, because rebuilding one instance invalidates entity handles.
		std::vector<UUID> instanceRoots;
		for (Entity entity : GetAllEntities())
		{
			if (entity.ContainsComponent<PrefabComponent>()
				&& entity.GetComponent<PrefabComponent>().PrefabHandle == prefabUUID)
				instanceRoots.push_back(entity.GetUUID());
		}

		SceneSerializer serializer(this);
		uint32_t refreshed = 0;

		for (UUID instanceUUID : instanceRoots)
		{
			Entity instance = GetEntity(instanceUUID);

			// A nested instance is rebuilt along with its parent, so it may already be gone.
			if (instance.GetEntityHandle() == Constants::Entities::InvalidEntityID)
				continue;

			// Placement, identity, hierarchy position and script overrides survive a refresh.
			std::unordered_map<std::string, SavedScriptOverrides> savedOverrides = CaptureScriptOverrides(this, instance);
			std::string name = instance.GetName();
			TransformComponent savedTransform = instance.GetComponent<TransformComponent>();
			UUID parentUUID = Constants::InvalidUUID;
			if (instance.ContainsComponent<RelationshipComponent>())
				parentUUID = instance.GetComponent<RelationshipComponent>().ParentHandle;

			// The old subtree goes; only the root entity is reused.
			if (instance.ContainsComponent<RelationshipComponent>())
			{
				std::vector<UUID> oldChildren = instance.GetComponent<RelationshipComponent>().Children;
				for (UUID childUUID : oldChildren)
				{
					Entity child = GetEntity(childUUID);
					if (child.GetEntityHandle() != Constants::Entities::InvalidEntityID)
						RemoveEntity(child);
				}
				FlushPendingRemovals();
			}

			instance = GetEntity(instanceUUID);
			if (instance.GetEntityHandle() == Constants::Entities::InvalidEntityID)
				continue;

			ResetEntityToCoreComponents(instance);
			serializer.DeserializeEntitiesFromString(prefab->YAMLData, false, instance);

			// The prefab's own transform and name overwrote the instance's, so put them back.
			auto& transform = instance.GetComponent<TransformComponent>();
			transform.Position = savedTransform.Position;
			transform.Rotation = savedTransform.Rotation;
			transform.Scale = savedTransform.Scale;
			transform.InvalidateWorld();

			instance.GetComponent<TagComponent>().Tag = name;

			// Re-applied after the subtree exists, since the children are matched by name path.
			ReapplyScriptOverrides(this, instance, savedOverrides);

			Matrix4f parentWorldTransform = Matrix4f(1.0f);
			if (parentUUID != Constants::InvalidUUID)
			{
				Entity parent = GetEntity(parentUUID);
				if (parent.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				{
					instance.GetComponent<RelationshipComponent>().ParentHandle = parentUUID;

					auto& siblings = parent.GetComponent<RelationshipComponent>().Children;
					if (std::find(siblings.begin(), siblings.end(), instanceUUID) == siblings.end())
						siblings.push_back(instanceUUID);

					if (parent.ContainsComponent<TransformComponent>())
						parentWorldTransform = parent.GetComponent<TransformComponent>().WorldTransform;
				}
			}

			FinalizeRuntimePrefabInstance(this, instance, parentWorldTransform);
			refreshed++;
		}

		return refreshed;
	}

	Entity Scene::InstantiatePrefab(SharedPtr<Prefab> prefabAsset, const Vector3f* position)
	{
		EB_PROFILE_FUNCTION();
		SceneSerializer serializer(this);
		Entity root = serializer.DeserializePrefab(prefabAsset);

		if (position != nullptr)
		{
			auto& transform = root.GetComponent<TransformComponent>();
			transform.Position = *position;
		}

		FinalizeRuntimePrefabInstance(this, root, Matrix4f(1.0f));
		return root;
	}

	Entity Scene::InstantiatePrefab(SharedPtr<Prefab> prefabAsset, Entity parent, const Vector3f* position)
	{
		EB_PROFILE_FUNCTION();
		SceneSerializer serializer(this);
		Entity root = serializer.DeserializePrefab(prefabAsset);
		parent.AddChild(root);

		auto& childRelationshipComp = root.GetComponent<RelationshipComponent>();
		childRelationshipComp.ParentHandle = parent.GetUUID();

		if (position != nullptr)
		{
			auto& transform = root.GetComponent<TransformComponent>();
			transform.Position = *position;
		}

		Matrix4f parentWorldTransform = Matrix4f(1.0f);
		if (parent.ContainsComponent<TransformComponent>())
			parentWorldTransform = parent.GetComponent<TransformComponent>().WorldTransform;

		FinalizeRuntimePrefabInstance(this, root, parentWorldTransform);
		return root;
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