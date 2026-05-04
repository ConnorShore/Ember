#pragma once

#include "System.h"
#include "Ember/Scene/Entity.h"

namespace Ember {

	struct AIDebugRenderSettings
	{
		bool Enabled = true;
	};

	class AISystem : public System
	{
	public:
		AISystem();
		virtual ~AISystem();

		void OnAttach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;
		void OnDetach() override;

		void OnEditorUpdate(TimeStep delta, Scene* scene);

		void SetPathPreviewEntity(EntityID entityID) { m_PreviewPathEntity = entityID; }
		void ClearPathPreviewEntity() { m_PreviewPathEntity = Constants::Entities::InvalidEntityID; }
	private:

		AIDebugRenderSettings m_DebugRenderSettings;
		EntityID m_PreviewPathEntity = Constants::Entities::InvalidEntityID;
	};

}