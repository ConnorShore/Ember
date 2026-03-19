#include "InspectorPanel.h"

#include <Ember.h>

#include <format>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Draw Component Helper Function
	//////////////////////////////////////////////////////////////////////////

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		// If the entity doesn't have this component, don't draw anything!
		if (!entity.ContainsComponent<T>())
			return;

		const ImGuiTreeNodeFlags treeNodeFlags = 
			ImGuiTreeNodeFlags_DefaultOpen		| 
			ImGuiTreeNodeFlags_Framed			| 
			ImGuiTreeNodeFlags_SpanAvailWidth	| 
			ImGuiTreeNodeFlags_AllowOverlap		| 
			ImGuiTreeNodeFlags_FramePadding;

		auto& component = entity.GetComponent<T>();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });

		bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, std::format("{} Component", name).c_str());

		ImGui::PopStyleVar();

		if (open)
		{
			uiFunction(component);
			ImGui::TreePop();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Inspector Panel Impl
	//////////////////////////////////////////////////////////////////////////

	InspectorPanel::InspectorPanel()
		: Panel ("Inspector")
	{
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

		// Go through all possible component types and see if it contains it, if so render the component with values
		DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component) {
			if (ImGui::BeginTable("TransformProps", 2, ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				// Position
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Position");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat3("##Position", &component.Position[0], 0.1f, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				// Rotation
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Rotation");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat3("##Rotation", &component.Rotation[0], 0.1f, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				// Scale
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Scale");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat3("##Scale", &component.Scale[0], 0.1f, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				ImGui::EndTable();
			}
		});

		DrawComponent<MaterialComponent>("Material", entity, [](MaterialComponent& component) {
			ImGui::Text("Material Component body");
			});

		ImGui::End();
	}

}