#pragma once

#include "ColliderComponentUI.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <Ember/Utils/PlatformUtil.h>
#include <imgui/imgui.h>

namespace Ember {

	class ConvexMeshColliderComponentUI : public ColliderComponentUI<ConvexMeshColliderComponent>
	{
	public:
		ConvexMeshColliderComponentUI(EditorContext* context) : ColliderComponentUI(context) {}
		inline const char* GetName() const override { return "Convex Mesh Collider"; }

	protected:
		inline void RenderComponentProperties(ConvexMeshColliderComponent& component) override
		{
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
					auto meshAsset = m_AssetManager.GetAsset<Mesh>(component.MeshHandle);
					if (meshAsset)
						meshName = std::filesystem::path(meshAsset->GetFilePath()).filename().string();
				}
			}

			if (UI::PropertyGrid::Begin("ConvexMeshColliderComponent"))
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
					auto mesh = m_AssetManager.Load<Mesh>(droppedPath);
					if (mesh)
					{
						component.MeshHandle = mesh->GetUUID();
						component.NeedsRebuild = true;
					}
				}

				bool changed = false;
				changed |= UI::PropertyGrid::Checkbox("Is Trigger", component.IsTrigger);
				if (changed)
					component.NeedsRebuild = true;

				ImGui::PushID("Mesh");
				if (ImGui::BeginPopup("ChooseMeshPopup"))
				{
					if (ImGui::MenuItem("Load from file..."))
					{
						std::string defaultDir = (ProjectManager::GetActive()->GetAssetDirectory() / "Models").string();
						std::string meshFile = FileDialog::OpenFile(defaultDir.c_str(), "Ember Mesh (*.ebmesh)", "*.ebmesh;");
						if (!meshFile.empty())
						{
							auto meshAsset = m_AssetManager.Load<Mesh>(meshFile);
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