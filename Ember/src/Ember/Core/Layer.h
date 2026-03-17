#pragma once

#include "Core.h"
#include "Ember/Event/Event.h"
#include "Ember/Asset/AssetManager.h"

#include <string>

namespace Ember {

	class Layer
	{
	public:
		Layer(const std::string& name)
			: m_Name(name) { }
		virtual ~Layer() { }

		virtual void OnAttach() {}
		virtual void OnDetach() {};
		virtual void OnEvent(Event& event) {}
		virtual void OnUpdate(TimeStep delta) {};
		virtual void OnImGuiRender(TimeStep delta) {};

		inline const std::string& GetName() const { return m_Name; }

		inline void SetAssetManagerHandle(AssetManager* handle) { m_AssetManagerHandle = handle; }

	protected:
		template<IsCoreAsset T, typename... Args>
		SharedPtr<T> CreateAsset(Args&&... args)
		{
			EB_CORE_ASSERT(m_AssetManagerHandle, "AssetManager is not initialized for this layer!");
			return m_AssetManagerHandle->Create<T>(std::forward<Args>(args)...);
		}

		template<IsCoreAsset T>
		SharedPtr<T> LoadAsset(const std::string& filePath)
		{
			EB_CORE_ASSERT(m_AssetManagerHandle, "AssetManager is not initialized for this layer!");
			return m_AssetManagerHandle->Load<T>(filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> LoadAsset(const std::string& name, const std::string& filePath)
		{
			EB_CORE_ASSERT(m_AssetManagerHandle, "AssetManager is not initialized for this layer!");
			return m_AssetManagerHandle->Load<T>(name, filePath);
		}

		template<IsCoreAsset T>
		SharedPtr<T> GetAsset(const std::string& name)
		{
			EB_CORE_ASSERT(m_AssetManagerHandle, "AssetManager is not initialized for this layer!");
			return m_AssetManagerHandle->GetAsset<T>(name);
		}

	private:
		// TODO: Make m_Name debug only?
		std::string m_Name;
		AssetManager* m_AssetManagerHandle = nullptr;
	};

}