#pragma once
#include "ComponentUI.h"
#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Asset/AssetManager.h>
#include <Ember/Utils/PlatformUtil.h>

#include <imgui/imgui.h>

#include <filesystem>

namespace Ember {

	class SkinnedMeshComponentUI : public ComponentUI<SkinnedMeshComponent>
	{
	public:
		SkinnedMeshComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Skinned Mesh Component"; }

	protected:
		inline void RenderComponentImpl(SkinnedMeshComponent& component) override
		{
			auto& assetManager = Application::Instance().GetAssetManager();

			if (UI::PropertyGrid::Begin("AnimatorComponentProps"))
			{
				RenderMeshPicker(component, assetManager);
				RenderAnimatorEntitySelector(component, assetManager);

				UI::PropertyGrid::End();
			}

		}

		inline void RenderMeshPicker(SkinnedMeshComponent& component, AssetManager& assetManager)
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
				else
				{
					auto meshAsset = assetManager.GetAsset<Mesh>(component.MeshHandle);
					if (meshAsset)
						meshName = std::filesystem::path(meshAsset->GetFilePath()).filename().string();
				}
			}

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
		}

		
		void RenderAnimatorEntitySelector(SkinnedMeshComponent& component, AssetManager& assetManager)
		{
			// Add ability to pick animator entity
			auto animatorEntity = m_Context->ActiveScene->GetEntity(component.AnimatorEntityHandle);

			std::string animatorName = "None (Animator)";
			if (animatorEntity)
				animatorName = animatorEntity.GetName();

			if (UI::PropertyGrid::BeginComboBox("Animator Entity", animatorName.c_str()))
			{
				// Option for "None"
				if (UI::PropertyGrid::ComboBoxItem("None", component.AnimatorEntityHandle == Constants::InvalidUUID))
					component.AnimatorEntityHandle = Constants::InvalidUUID;

				ImGui::Separator();

				auto entities = m_Context->ActiveScene->GetAllEntitiesWithComponents<AnimatorComponent>();
				for (auto& entity : entities)
				{
					bool isSelected = component.AnimatorEntityHandle == entity.GetUUID();
					if (UI::PropertyGrid::ComboBoxItem(entity.GetName().c_str(), isSelected))
						component.AnimatorEntityHandle = entity.GetUUID();
					
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				UI::PropertyGrid::EndComboBox();
			}

			// Button to select animator entity
			ImGui::SameLine();
			if (ImGui::Button("->"))
			{
				if (animatorEntity)
					m_Context->SelectedEntity = animatorEntity;
			}
		}
	};

}