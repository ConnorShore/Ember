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

#include <Ember.h>

#include <format>

namespace Ember {

	InspectorPanel::InspectorPanel()
		: Panel ("Inspector")
	{
		// Populate the list of Component UIs that this panel will draw for entities
		m_ComponentUIs.emplace_back(ScopedPtr<TransformComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<CameraComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<DirectionalLightComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<PointLightComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<SpotLightComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<MeshComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<MaterialComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<RigidBodyComponentUI>::Create());
		m_ComponentUIs.emplace_back(ScopedPtr<ScriptComponentUI>::Create());
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
		std::string entityName = entity.GetComponent<TagComponent>().Tag;

		ImGui::Begin(m_Title.c_str());

		// Entity Header
		if (ImGui::BeginTable("Name", 2, ImGuiTableFlags_SizingFixedSame))
		{
			ImGui::TableSetupColumn("EntityName", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch);

			// Entity name
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(entityName.c_str());

			// Actions
			ImGui::TableNextColumn();
			float buttonWidth = ImGui::CalcTextSize("Add Component").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float posX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth;
			ImGui::SetCursorPosX(posX);

			if (ImGui::Button("Add Component")) {
				// Handle click
			}

			ImGui::EndTable();
		}

		// Draw applicable component UIs
		for (auto& componentUI : m_ComponentUIs)
		{
			componentUI->Draw(entity);
		}

		ImGui::End();
	}

}