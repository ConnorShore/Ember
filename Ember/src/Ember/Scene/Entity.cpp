#include "ebpch.h"
#include "Entity.h"

namespace Ember {

	std::vector<Entity> Entity::GetAllChildren()
	{
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

	Entity Entity::GetParent()
	{
		auto& relationship = GetComponent<RelationshipComponent>();
		if (relationship.ParentHandle == Constants::InvalidUUID)
			return Entity(); // Return invalid entity if no parent

		return m_SceneHandle->GetEntity(relationship.ParentHandle);
	}

	Entity Entity::GetRootParent()
	{
		auto current = GetParent();
		while (current != Constants::Entities::InvalidEntityID)
		{
			if (current.IsRootParent())
				return current;

			current = current.GetParent();
		}

		return current;
	}

	bool Entity::IsRootParent()
	{
		return GetComponent<RelationshipComponent>().ParentHandle == Constants::InvalidUUID;
	}

	void Entity::RemoveFromParent()
	{
		auto& relationship = GetComponent<RelationshipComponent>();
		if (relationship.ParentHandle == Constants::InvalidUUID)
			return;

		Entity parent = m_SceneHandle->GetEntity(relationship.ParentHandle);
		if (parent != Constants::Entities::InvalidEntityID)
		{
			auto& parentRelationship = parent.GetComponent<RelationshipComponent>();
			UUID myUUID = GetUUID();
			parentRelationship.Children.erase(
				std::remove(parentRelationship.Children.begin(), parentRelationship.Children.end(), myUUID),
				parentRelationship.Children.end()
			);
		}

		relationship.ParentHandle = Constants::InvalidUUID;
		relationship.IsAttachment = false;

		// The local TRS is untouched but it now resolves against an identity parent, so the cached
		// world transform is stale. IsLocalDirty() cannot detect a parent change on its own.
		GetComponent<TransformComponent>().InvalidateWorld();
	}

	Entity Entity::AddChild(const std::string& name /*= ""*/)
	{
		Entity childEntity = m_SceneHandle->AddEntity(name);

		// Set parent child relationship
		auto& relationship = GetComponent<RelationshipComponent>();
		relationship.Children.push_back(childEntity.GetUUID());

		auto& childRelationship = childEntity.GetComponent<RelationshipComponent>();
		childRelationship.ParentHandle = GetUUID();

		return childEntity;
	}

	Entity Entity::AddChild(Entity entity)
	{
		auto& relationship = GetComponent<RelationshipComponent>();
		relationship.Children.push_back(entity.GetUUID());

		auto& childRelationship = entity.GetComponent<RelationshipComponent>();
		childRelationship.ParentHandle = GetUUID();

		// The child's local TRS is unchanged but now resolves against this parent instead of the
		// world, so its cached world transform must be rebuilt on the next pass.
		entity.GetComponent<TransformComponent>().InvalidateWorld();

		return entity;
	}

	Entity Entity::AddChild(Entity entity, bool isAttachment)
	{
		auto& relationship = GetComponent<RelationshipComponent>();
		relationship.Children.push_back(entity.GetUUID());

		auto& childRelationship = entity.GetComponent<RelationshipComponent>();
		childRelationship.ParentHandle = GetUUID();
		childRelationship.IsAttachment = isAttachment;

		// Convert child's world transform to local coordinates relative to parent
		auto& parentTransform = GetComponent<TransformComponent>();
		auto& childTransform = entity.GetComponent<TransformComponent>();

		// Decompose parent's world transform
		Vector3f parentTranslation, parentRotation, parentScale;
		Math::DecomposeTransform(parentTransform.WorldTransform, parentTranslation, parentRotation, parentScale);

		// Build parent transform matrix (excluding scale if attachment)
		Matrix4f parentMatrix;
		if (isAttachment)
		{
			// For attachments, use parent's translation and rotation only (no scale)
			parentMatrix = Math::Translate(parentTranslation) * Math::GetRotationMatrix(parentRotation);

			// Get child's current world transform (from Position, Rotation, Scale)
			Matrix4f childWorldMatrix = childTransform.GetLocalTransform();

			// Convert to local space: localTransform = inverse(parentMatrix) * childWorldMatrix
			Matrix4f localTransform = Math::Inverse(parentMatrix) * childWorldMatrix;

			// Decompose the local transform to get Position, Rotation, Scale
			Vector3f localTranslation, localRotation, localScale;
			Math::DecomposeTransform(localTransform, localTranslation, localRotation, localScale);

			// Update child's transform component with local coordinates
			childTransform.Position = localTranslation;
			childTransform.Rotation = localRotation;
			childTransform.Scale = localScale;
		}
		else
		{
			// For regular children, use full parent transform including scale
			parentMatrix = parentTransform.WorldTransform;
		}

		// Whether or not the local TRS was rewritten above, the child now resolves against a new
		// parent, so its cached world transform must be rebuilt on the next pass.
		childTransform.InvalidateWorld();

		return entity;
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

	void Entity::SetActive(bool active, bool recursive /*= true*/)
	{
		if (active)
		{
			if (ContainsComponent<DisabledComponent>())
				DetachComponent<DisabledComponent>();
		}
		else
		{
			if (!ContainsComponent<DisabledComponent>())
				AttachComponent<DisabledComponent>();
		}

		if (!recursive)
			return;

		for (auto& child : GetAllChildren())
		{
			if (child == Constants::Entities::InvalidEntityID)
				continue;

			if (active)
			{
				if (child.ContainsComponent<DisabledComponent>())
					child.DetachComponent<DisabledComponent>();
			}
			else
			{
				if (!child.ContainsComponent<DisabledComponent>())
					child.AttachComponent<DisabledComponent>();
			}
		}
	}

	bool Entity::IsActive() const
	{
		return !m_SceneHandle->GetRegistry().ContainsComponent<DisabledComponent>(m_EntityHandle);
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
