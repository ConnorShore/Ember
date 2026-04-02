#pragma once
#include "ComponentUI.h"
#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember.h>
#include <filesystem>

namespace Ember {

	class MeshComponentUI : public ComponentUI<MeshComponent>
	{
	public:
		MeshComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Mesh Component"; }

	protected:
		inline void RenderComponentImpl(MeshComponent& component) override
		{
			auto& assetManager = Application::Instance().GetAssetManager();
			bool meshExists = component.MeshHandle != Constants::InvalidUUID;
			std::string meshName = "None (Mesh)";

			if (meshExists)
			{
				if (component.MeshHandle == Constants::Assets::CubeMeshUUID) 
					meshName = "Cube";
				else if (component.MeshHandle == Constants::Assets::QuadMeshUUID) 
					meshName = "Quad";
				else if (component.MeshHandle == Constants::Assets::SphereMeshUUID) 
					meshName = "Sphere";
				else
				{
					auto meshAsset = assetManager.GetAsset<Mesh>(component.MeshHandle);
					if (meshAsset)
						meshName = std::filesystem::path(meshAsset->GetFilePath()).filename().string();
				}
			}

			if (UI::PropertyGrid::Begin("MeshComponentProps"))
			{
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetMesh);
				std::string droppedPath;

				auto browseFunc = [&]() {
					ImGui::OpenPopup("ChooseMeshPopup");
					};

				auto clearFunc = meshExists ? UI::UICallbackFunc([&]() {
					component.MeshHandle = Constants::InvalidUUID;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("Mesh", meshName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto mesh = assetManager.Load<Mesh>(droppedPath);
					if (mesh)
						component.MeshHandle = mesh->GetUUID();
				}

				ImGui::PushID("Mesh");
				if (ImGui::BeginPopup("ChooseMeshPopup"))
				{
					if (ImGui::MenuItem("Cube")) 
						component.MeshHandle = Constants::Assets::CubeMeshUUID;
					if (ImGui::MenuItem("Quad")) 
						component.MeshHandle = Constants::Assets::QuadMeshUUID;
					if (ImGui::MenuItem("Sphere")) 
						component.MeshHandle = Constants::Assets::SphereMeshUUID;

					ImGui::Separator();

					if (ImGui::MenuItem("Load from file..."))
					{
						std::string meshFile = FileDialog::OpenFile("Ember-Forge/assets/models", "3D Model (*.obj;*.fbx)", "*.obj;*.fbx");
						if (!meshFile.empty())
						{
							auto meshAsset = assetManager.Load<Mesh>(meshFile);
							if (meshAsset)
								component.MeshHandle = meshAsset->GetUUID();
						}
					}

					ImGui::EndPopup();
				}
				ImGui::PopID();

				UI::PropertyGrid::End();
			}
		}

	};

}