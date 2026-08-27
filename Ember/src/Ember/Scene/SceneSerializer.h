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
		bool SerializeCooked(const std::string& filepath);
		bool DeserializeCooked(const std::string& filepath);

		// Prefab Operations
		bool SerializePrefab(Entity prefabRoot, const std::string& filepath);
		Entity DeserializePrefab(SharedPtr<Prefab> prefab, bool preserveUUIDs = false);

		// In-memory entity capture, shared with the prefab path and used by the undo system.
		std::string SerializeEntitiesToString(const std::vector<Entity>& entities, const std::string& label = "Entities");

		// Recreates entities from such a document. With preserveUUIDs, an entity whose UUID already
		// exists is written into in place so its slot and inbound references survive.
		std::vector<Entity> DeserializeEntitiesFromString(const std::string& yaml, bool preserveUUIDs);

		// The root plus every descendant, in the breadth-first order the prefab format expects.
		std::vector<Entity> GatherSubtree(Entity root);

	private:
		void SerializeEntityNode(ryml::NodeRef& entityNode, Entity entity);
		void DeserializeEntityNode(ryml::NodeRef& entityNode, Entity deserializedEntity, const std::unordered_map<uint64_t, UUID>& uuidRemap);

	private:
		SharedPtr<Scene> m_Scene;
	};

}