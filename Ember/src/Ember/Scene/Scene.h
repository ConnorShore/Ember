#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Core.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Asset/Asset.h"
#include "Ember/Asset/Model.h"
#include "Ember/Tools/EditorCamera.h"
#include "Ember/Asset/Prefab.h"
#include "Ember/Core/PoolManager.h"

#include <unordered_map>

namespace Ember {

	class Entity;

	enum class SceneState
	{
		Edit = 0,
		Play = 1,
		Pause = 2
	};

	class Scene : public Asset
	{
	public:
		Scene(const std::string& name, const std::string& filePath);
		Scene(UUID uuid, const std::string& name, const std::string& filePath);
		~Scene();

		static SharedPtr<Scene> CopyScene(SharedPtr<Scene> other);

		void OnAttach();
		void OnDetach();

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnUpdateRuntime(TimeStep delta);
		void OnUpdateEdit(TimeStep delta, EditorCamera& camera);
		void OnEvent(Event& event);

		void SetActiveCamera(Entity cameraEntity);

		void OnViewportResize(uint32_t width, uint32_t height);

        Entity AddEntity(const std::string& name = "");
		Entity AddEntity(UUID uuid, const std::string& name);
		Entity GetEntity(UUID uuid);
		Entity GetEntity(const std::string& name);
		Entity DuplicateEntity(Entity entity);

		SharedPtr<Prefab> CreatePrefab(Entity entity, const std::string& filepath);

		void SetEntityParent(UUID childUUID, Entity newParent);
		void RemoveParent(Entity child);

		template<IsCoreAsset T>
		inline void RegisterAsset(const SharedPtr<T>& asset);

		template<IsCoreAsset T>
		inline SharedPtr<T> GetAsset(const std::string& assetName);

		template<IsCoreAsset T>
		inline SharedPtr<T> GetAsset(UUID assetUUID);

		template<IsCoreAsset T>
		inline std::vector<SharedPtr<T>> GetAssetsOfType();

		std::vector<Entity> GetAllEntities() const;

		template<typename Driver, typename... Filters>
		std::vector<Entity> GetAllEntitiesWithComponents()
		{
			std::vector<Entity> entities;
			entities.reserve(m_Registry->GetActiveEntities<Driver>().size());

			auto view = m_Registry->Query<Driver, Filters...>();
			for (EntityID entity : view)
				entities.emplace_back(entity, const_cast<Scene*>(this));

			return entities;
		}

		void RemoveEntity(Entity entity);

		Entity InstantiateModel(const std::string& modelFile);
		Entity InstantiatePrefab(SharedPtr<Prefab> prefabAsset, const Vector3f* position);

		Entity GetEntityAtPixel(uint32_t x, uint32_t y);

		template<typename T, typename... Args>
		inline T& AttachComponent(const Entity& entity, Args&&... args);

		template<typename T>
		inline void DetachComponent(const Entity& entity);

		template<typename T>
		inline T& GetComponent(const Entity& entity);

		inline Registry& GetRegistry() { return *m_Registry; }
		inline PoolManager& GetPoolManager() { return *m_PoolManager; }

		inline const std::string& GetName() const { return m_Name; }

		inline void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }
		inline const std::string& GetFilePath() const { return m_FilePath; }

		inline bool IsRuntime() const { return m_IsRuntime; }

		void ResetAllPhysicsState();

		inline static AssetType GetStaticType() { return AssetType::Scene; }

	private:
		bool OnWindowResize(const WindowResizeEvent& event);
		void ProcessModelNode(Entity currentEntity, const ModelNode& node, const SharedPtr<Model>& model, UUID animatorEntityUUID);
		Entity DuplicateEntityRecursive(Entity entity, UUID newParentId, bool isRoot, UUID originalAnimatorUUID = Constants::InvalidUUID, UUID newAnimatorUUID = Constants::InvalidUUID);
		
		void RemoveEntityFromScene(Entity entity);
		void RemovePendingRemovals();

	private:
		ScopedPtr<Registry> m_Registry;
		ScopedPtr<PoolManager> m_PoolManager;

		std::unordered_map<UUID, EntityID> m_EntityUUIDMap;
		// TODO: May want a entity name map for faster lookup by name

		std::vector<Entity> m_PendingRemovals;

		bool m_IsRuntime = false;
	};

}

#include "Entity.h"
#include "Ember/Core/Application.h"

namespace Ember {

	template<typename T, typename... Args>
	inline T& Scene::AttachComponent(const Entity& entity, Args&&... args)
	{
		EntityID entityHandle = entity.GetEntityHandle();
		return m_Registry->AttachComponent<T>(entityHandle, std::forward<Args>(args)...);
	}

	template<typename T>
	inline void Scene::DetachComponent(const Entity& entity)
	{
		EntityID entityHandle = entity.GetEntityHandle();
		m_Registry->DetachComponent<T>(entityHandle);
	}

	template<typename T>
	inline T& Scene::GetComponent(const Entity& entity)
	{
		EntityID entityHandle = entity.GetEntityHandle();
		return m_Registry->GetComponent<T>(entityHandle);
	}

	template<IsCoreAsset T>
	inline void Scene::RegisterAsset(const SharedPtr<T>& asset)
	{
		Application::Instance().GetAssetManager().Register<T>(asset);
	}

	template<IsCoreAsset T>
	inline SharedPtr<T> Scene::GetAsset(const std::string& assetName)
	{
		return Application::Instance().GetAssetManager().GetAsset<T>(assetName);
	}

	template<IsCoreAsset T>
	inline SharedPtr<T> Scene::GetAsset(UUID assetUUID)
	{
		return Application::Instance().GetAssetManager().GetAsset<T>(assetUUID);
	}

	template<IsCoreAsset T>
	inline std::vector<SharedPtr<T>> Scene::GetAssetsOfType()
	{
		return Application::Instance().GetAssetManager().GetAssetsOfType<T>();
	}

}