#include "ebpch.h"
#include "Entity.h"

namespace Ember {

	Entity::Entity(const std::string& tag, Scene* scene)
		: m_SceneHandle(scene)
	{
		m_EntityHandle = m_SceneHandle->GetRegistry().CreateEntity();

		TagComponent tagComponent(tag);
		TransformComponent transform;
		RelationshipComponent relationship;

		m_SceneHandle->GetRegistry().AttachComponent(m_EntityHandle, tagComponent);
		m_SceneHandle->GetRegistry().AttachComponent(m_EntityHandle, transform);
		m_SceneHandle->GetRegistry().AttachComponent(m_EntityHandle, relationship);
	}


	Entity::Entity(EntityID entity, Scene* scene)
		: m_SceneHandle(scene), m_EntityHandle(entity)
	{
	}

	std::vector<Entity> Entity::GetAllChildren()
	{
		std::vector<Entity> ret;
		auto& relationship = GetComponent<RelationshipComponent>();
		for (EntityID childID : relationship.Children)
		{
			Entity childEntity(childID, m_SceneHandle);
			ret.push_back(childEntity);
		}

		// Look at children's children
		for (EntityID childID : relationship.Children)
		{
			Entity childEntity(childID, m_SceneHandle);
			std::vector<Entity> childChildren = childEntity.GetAllChildren();
			ret.insert(ret.end(), childChildren.begin(), childChildren.end());
		}

		return ret;
	}

	unsigned int Entity::GetNumChildren()
	{
		return GetComponent<RelationshipComponent>().Children.size();
	}

	bool Entity::IsRootParent()
	{
		return GetComponent<RelationshipComponent>().ParentHandle == Constants::Entities::InvalidEntityID;
	}

	Entity Entity::GetChildByName(const std::string& name)
	{
		auto& relationship = GetComponent<RelationshipComponent>();

		for (EntityID childID : relationship.Children)
		{
			Entity childEntity(childID, m_SceneHandle);

			if (childEntity.GetName() == name)
			{
				return childEntity;
			}
		}

		return Entity();
	}

	Entity Entity::FindEntityInHierarchy(const std::string& name)
	{
		auto& relationship = GetComponent<RelationshipComponent>();
		for (EntityID childID : relationship.Children)
		{
			Entity childEntity(childID, m_SceneHandle);
			if (childEntity.GetName() == name)
				return childEntity;
		}

		// Look at children's children
		for (EntityID childID : relationship.Children)
		{
			Entity childEntity(childID, m_SceneHandle);
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

}
