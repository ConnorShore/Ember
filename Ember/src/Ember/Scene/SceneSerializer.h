#pragma once

#include "Ember/Core/Core.h"

#include "Ember/Asset/Prefab.h"

#include "Scene.h"
#include <string>
#include <unordered_map>

namespace Ember {

	class SceneSerializer
	{
	public:
		SceneSerializer(const SharedPtr<Scene>& scene) : m_Scene(scene) {}
		~SceneSerializer() = default;

		// Scene Operations
		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

		// Prefab Operations
		bool SerializePrefab(Entity prefabRoot, const std::string& filepath);
		Entity DeserializePrefab(SharedPtr<Prefab> prefab);

	private:
		void SerializeEntityNode(ryml::NodeRef& entityNode, Entity entity);
		void DeserializeEntityNode(ryml::NodeRef& entityNode, Entity deserializedEntity, const std::unordered_map<uint64_t, UUID>& uuidRemap);

	private:
		SharedPtr<Scene> m_Scene;
	};

}