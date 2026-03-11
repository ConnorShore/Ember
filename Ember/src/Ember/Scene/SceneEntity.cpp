#include "ebpch.h"
#include "SceneEntity.h"

namespace Ember {

	SceneEntity::SceneEntity(const std::string& tag, Scene* scene)
		: m_SceneHandle(scene)
	{
		m_EntityHandle = m_SceneHandle->GetRegistry().CreateEntity();

		TagComponent tagComponent(tag);
		TransformComponent transform({ 0.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f});

		m_SceneHandle->GetRegistry().AttachComponent(m_EntityHandle, tagComponent);
		m_SceneHandle->GetRegistry().AttachComponent(m_EntityHandle, transform);
	}


	SceneEntity::SceneEntity(EntityID entity, Scene* scene)
		: m_SceneHandle(scene), m_EntityHandle(entity)
	{
	}

	const std::string& SceneEntity::GetName() const
	{
		return m_SceneHandle->GetRegistry().GetComponent<TagComponent>(m_EntityHandle).Tag;
	}

}
