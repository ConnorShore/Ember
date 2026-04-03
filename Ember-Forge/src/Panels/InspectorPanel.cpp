#include "efpch.h"
#include "InspectorPanel.h"

#include "ComponentUI/TransformComponentUI.h"
#include "ComponentUI/CameraComponentUI.h"
#include "ComponentUI/DirectionalLightComponentUI.h"
#include "ComponentUI/PointLightComponentUI.h"
#include "ComponentUI/SpotLightComponentUI.h"
#include "ComponentUI/RigidBodyComponentUI.h"
#include "ComponentUI/ScriptComponentUI.h"
#include "ComponentUI/MeshComponentUI.h"
#include "ComponentUI/MaterialComponentUI.h"

#include <imgui/imgui.h>

#include <format>

namespace Ember {

	InspectorPanel::InspectorPanel(EditorContext* context)
		: Panel("Inspector", context)
	{
		// Populate the list of Component UIs that this panel will draw for entities
		m_ComponentUIs.emplace_back(ScopedPtr<TransformComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<CameraComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<DirectionalLightComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<PointLightComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<SpotLightComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<MeshComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<MaterialComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<RigidBodyComponentUI>::Create(m_Context));
		m_ComponentUIs.emplace_back(ScopedPtr<ScriptComponentUI>::Create(m_Context));
	}

	InspectorPanel::~InspectorPanel()
	{
	}

	void InspectorPanel::OnEvent(Event& event)
	{
	}

	void InspectorPanel::OnImGuiRender()
	{
		if (m_Context->SelectedEntity == Constants::Entities::InvalidEntityID)
		{
			// Blank panel if no entity selected
			ImGui::Begin(m_Title.c_str());
			ImGui::Text("Select an Entity to inspect properties");
			ImGui::End();
			return;
		}

		Entity entity = m_Context->SelectedEntity;
		{
			ImGui::Begin(m_Title.c_str());

			// Entity Header
			DrawEntityHeader(entity);

			// Entity Components
			for (auto& componentUI : m_ComponentUIs)
				componentUI->Render(entity);

			ImGui::End();
		}
	}

	void InspectorPanel::DrawEntityHeader(Entity entity)
	{
		if (UI::PropertyGrid::Begin("EntityHeader"))
		{
			if (UI::PropertyGrid::HeaderWithActionButton(entity.GetName().c_str(), "Add Component"))
			{
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				for (auto& comps : m_ComponentUIs)
				{
					if (ImGui::MenuItem(comps->GetName()))
					{
						EB_CORE_TRACE("Adding component {}", comps->GetName());
						comps->CreateComponentForEntity(entity);
					}
				}
				ImGui::EndPopup();
			}

			UI::PropertyGrid::End();
		}
	}

}