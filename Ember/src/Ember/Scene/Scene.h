#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Core.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"
#include "Ember/Asset/Asset.h"
#include "Ember/Asset/Model.h"
#include "Ember/Tools/EditorCamera.h"
#include "Ember/Core/Application.h"

#include <unordered_map>

namespace Ember {

	class Entity;

	enum class SceneState
	{
		Edit = 0,
		Play = 1,
		Pause = 2
	};

	class Scene : public SharedResource
	{
	public:
		Scene(const std::string& name);
		~Scene();

		void OnUpdateRuntime(TimeStep delta);
		void OnUpdateEdit(TimeStep delta, EditorCamera& camera);
		void OnEvent(Event& event);

		void OnViewportResize(unsigned int width, unsigned int height);

		Entity AddEntity();
		Entity AddEntity(const std::string& name);
		Entity GetEntity(const std::string& tag);

		template<IsCoreAsset T>
		SharedPtr<T> GetAsset(const std::string& assetName)
		{
			return Application::Instance().GetAssetManager().GetAsset<T>(assetName);
		}

		template<IsCoreAsset T>
		std::vector<SharedPtr<T>> GetAssetsOfType()
		{
			return Application::Instance().GetAssetManager().GetAssetsOfType<T>();
		}

		std::vector<Entity> GetAllEntities() const;

		void RemoveEntity(Entity entity);

		Entity InstantiateModel(const SharedPtr<Model>& model, const std::string& name = "");

		Entity GetEntityAtPixel(unsigned int x, unsigned int y);

		template<typename T>
		inline void AttachComponent(const Entity& entity, T& component);

		template<typename T>
		inline void DetachComponent(const Entity& entity);

		template<typename T>
		inline T& GetComponent(const Entity& entity);

		inline Registry& GetRegistry() { return *m_Registry; }
		inline SceneState GetSceneState() const { return m_State; }
		inline void SetSceneState(SceneState state) { m_State = state; }

	private:
		bool OnWindowResize(const WindowResizeEvent& event);
		void ProcessModelNode(Entity currentEntity, const ModelNode& node, const SharedPtr<Model>& model);

	private:
		ScopedPtr<Registry> m_Registry;
		SceneState m_State = SceneState::Edit;

		std::unordered_map<std::string, EntityID> m_SceneEntities;

		std::string m_Name;
	};

}

#include "Entity.h"

namespace Ember {

	template<typename T>
	inline void Scene::AttachComponent(const Entity& entity, T& component)
	{
		EntityID entityHandle = entity.GetEntityHandle();
		m_Registry->AttachComponent<T>(entityHandle, component);
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

}