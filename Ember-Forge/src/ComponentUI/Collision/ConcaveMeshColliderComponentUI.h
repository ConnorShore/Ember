#pragma once

#include "ColliderComponentUI.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <Ember/Utils/PlatformUtil.h>
#include <imgui/imgui.h>

namespace Ember {

	class ConcaveMeshColliderComponentUI : public ColliderComponentUI<ConcaveMeshColliderComponent>
	{
	public:
		ConcaveMeshColliderComponentUI(EditorContext* context) : ColliderComponentUI(context) {}
		inline const char* GetName() const override { return "Concave Mesh Collider"; }

	protected:
		inline void RenderComponentProperties(ConcaveMeshColliderComponent& component) override
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
				else if (component.MeshHandle == Constants::Assets::CapsuleMeshUUID)
					meshName = "Capsule";
				else
				{
					auto meshAsset = assetManager.GetAsset<Mesh>(component.MeshHandle);
					if (meshAsset)
						meshName = std::filesystem::path(meshAsset->GetFilePath()).filename().string();
				}
			}

			if (UI::PropertyGrid::Begin("ConcaveMeshComponentProps"))
			{
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetMesh);
				std::string droppedPath;

				auto browseFunc = [&]() {
					ImGui::OpenPopup("ChooseMeshPopup");
					};

				auto clearFunc = meshExists ? UI::UICallbackFunc([&]() {
					component.MeshHandle = Constants::InvalidUUID;
					component.NeedsRebuild = true;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("Mesh", meshName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto mesh = assetManager.Load<Mesh>(droppedPath);
					if (mesh)
					{
						component.MeshHandle = mesh->GetUUID();
						component.NeedsRebuild = true;
					}
				}

				ImGui::PushID("Mesh");
				if (ImGui::BeginPopup("ChooseMeshPopup"))
				{
					if (ImGui::MenuItem("Load from file..."))
					{
						std::string meshFile = FileDialog::OpenFile("Ember-Forge/assets/models", "Ember Mesh (*.ebmesh)", "*.ebmesh;");
						if (!meshFile.empty())
						{
							auto meshAsset = assetManager.Load<Mesh>(meshFile);
							if (meshAsset)
							{
								component.MeshHandle = meshAsset->GetUUID();
								component.NeedsRebuild = true;
							}
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