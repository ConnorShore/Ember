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

		void SetPreviewEntity(EntityID entityID) { m_PreviewEntity = entityID; }
		void ClearPreviewEntity() { m_PreviewEntity = Constants::Entities::InvalidEntityID; }

		AIDebugRenderSettings& GetDebugRenderSettings() { return m_DebugRenderSettings; }

	private:

		AIDebugRenderSettings m_DebugRenderSettings;
		EntityID m_PreviewEntity = Constants::Entities::InvalidEntityID;
	};

}