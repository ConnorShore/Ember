#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <imgui/imgui.h>

namespace Ember {

	class PoolConfigComponentUI : public ComponentUI<PoolConfigComponent>
	{
	public:
		PoolConfigComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Pool Configuration Component"; }

	protected:
		inline void RenderComponentImpl(PoolConfigComponent& component) override
		{
			if (UI::PropertyGrid::Begin("PoolConfigProps"))
			{
				UI::PropertyGrid::InputText("Pool ID", component.PoolID);
				UI::PropertyGrid::UInt("Capacity", component.Capacity, 1, 0, 50000);
				
				// Drag drop and picker for prefab
				auto& assetManager = Application::Instance().GetAssetManager();
				bool prefabExists = component.PrefabHandle != Constants::InvalidUUID;

				std::string prefabName = "None";
				if (prefabExists)
				{
					auto prefabAsset = assetManager.GetAsset<Prefab>(component.PrefabHandle);
					prefabName = std::filesystem::path(prefabAsset->GetFilePath()).filename().string();
				}

				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPrefab);
				std::string droppedPath;

				auto browseFunc = [&]() {
					ImGui::OpenPopup("ChooseMeshPopup");
					};

				auto clearFunc = prefabExists ? UI::UICallbackFunc([&]() {
					component.PrefabHandle = Constants::InvalidUUID;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("Prefab", prefabName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto prefab = assetManager.Load<Prefab>(droppedPath);
					if (prefab)
						component.PrefabHandle = prefab->GetUUID();
					else
						EB_CORE_ERROR("Prefab {} doesn't exist!", droppedPath);
				}

				UI::PropertyGrid::End();
			}
		}
	};

}