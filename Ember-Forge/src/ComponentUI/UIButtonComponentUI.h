#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <imgui/imgui.h>

namespace Ember {

	class UIButtonComponentUI : public ComponentUI<UIButtonComponent>
	{
	public:
		UIButtonComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "UI Button Component"; }

	protected:
		inline void RenderComponentImpl(UIButtonComponent& component) override
		{
			bool containsScript = m_Context->SelectedEntity.ContainsComponent<ScriptComponent>();
			if (!containsScript)
			{
				if (ImGui::Button("Generate Button Script"))
				{
					GenerateButtonScript(m_Context->SelectedEntity);
				}
			}
			else
			{
				ImGui::Text("Button script attached!");
			}
		}

	private:
		// Writes a starter script next to the project's other scripts, attaches it, and opens it.
		void GenerateButtonScript(Entity entity)
		{
			auto scriptDirectory = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Script);
			auto scriptPath = scriptDirectory / std::format("{}.lua", entity.GetName());

			auto scriptAsset = ScriptGenerator::GenerateButtonScriptTemplate(entity.GetName(), scriptPath.string());
			if (!scriptAsset)
			{
				EB_ERROR("Failed to generate a button script at '{}'", scriptPath.string());
				return;
			}

			auto& scriptComponent = entity.AttachComponent<ScriptComponent>();
			scriptComponent.ScriptHandle = scriptAsset->GetUUID();
			scriptComponent.Initialized = false;

			ScriptEditor::OpenScript(scriptPath.string());
		}
	};

}
