#include "ebpch.h"
#include "Entity.h"

namespace Ember {

	std::vector<Entity> Entity::GetAllChildren()
	{
		// TODO: FIX THIS, may need to get actual entities from the scene registry
		// instead of creating new ones with the same ID and scene handle

		// Collect direct children first, then recursively gather all descendants
		std::vector<Entity> ret;
		auto& relationship = GetComponent<RelationshipComponent>();
		for (UUID childID : relationship.Children)
		{
			Entity childEntity = m_SceneHandle->GetEntity(childID);
			ret.push_back(childEntity);
		}

		// Look at children's children
		for (UUID childID : relationship.Children)
		{
			Entity childEntity = m_SceneHandle->GetEntity(childID);
			std::vector<Entity> childChildren = childEntity.GetAllChildren();
			ret.insert(ret.end(), childChildren.begin(), childChildren.end());
		}

		return ret;
	}

	uint32_t Entity::GetNumChildren()
	{
		return static_cast<uint32_t>(GetComponent<RelationshipComponent>().Children.size());
	}

	bool Entity::IsRootParent()
	{
		return GetComponent<RelationshipComponent>().ParentHandle == Constants::InvalidUUID;
	}

	Entity Entity::GetChildByName(const std::string& name)
	{
		auto& relationship = GetComponent<RelationshipComponent>();

		for (UUID childID : relationship.Children)
		{
			Entity childEntity = m_SceneHandle->GetEntity(childID);
			if (childEntity.GetName() == name)
			{
				return childEntity;
			}
		}

		return Entity();
	}

	// Depth-first search through the entity hierarchy by name
	Entity Entity::FindEntityInHierarchy(const std::string& name)
	{
		auto& relationship = GetComponent<RelationshipComponent>();
		for (UUID childID : relationship.Children)
		{
			Entity childEntity = m_SceneHandle->GetEntity(childID);
			if (childEntity.GetName() == name)
				return childEntity;
		}

		// Look at children's children
		for (UUID childID : relationship.Children)
		{
			Entity childEntity = m_SceneHandle->GetEntity(childID);
			Entity found = childEntity.FindEntityInHierarchy(name);

			if (found.GetEntityHandle() != Constants::Entities::InvalidEntityID)
				return found;
		}

		return Entity();
	}

	const std::string& Entity::GetName() const
	{
		return m_SceneHandle->GetRegistry().GetComponent<TagComponent>(m_EntityHandle).Tag;
	}

	UUID Entity::GetUUID() const 
	{ 
		return m_SceneHandle->GetRegistry().GetComponent<IDComponent>(m_EntityHandle).ID; 
	}

}
