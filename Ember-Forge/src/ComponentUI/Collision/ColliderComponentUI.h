#pragma once

#include "ComponentUI/ComponentUI.h"
#include "UI/FilterWidget.h"

#include <Ember/Core/ProjectManager.h>
#include <Ember/ECS/System/PhysicsSystem.h>

namespace Ember {

	template<typename T>
	class ColliderComponentUI : public ComponentUI<T>
	{
	public:
		ColliderComponentUI(EditorContext* context) : ComponentUI<T>(context) {}

		void CreateComponentForEntity(Entity entity) override
		{
			if (entity.ContainsComponent<T>())
			{
				EB_CORE_WARN("Entity already contains a component of type %s!", this->GetName());
				return;
			}

			this->m_Context->ActiveScene()->AttachComponent<T>(entity);
		}

	protected:
		virtual void RenderComponentProperties(T& component) = 0;

	protected:
		inline void RenderComponentImpl(T& component) override
		{
			// Preview checkbox — when enabled and the entity is selected, the collider is
			// drawn in the viewport without needing the global physics debug draw toggle.
			if (UI::PropertyGrid::Begin("ColliderPreviewProps"))
			{
				if (UI::PropertyGrid::Checkbox("Preview Collider", component.PreviewCollider))
				{
					auto physicsSystem = Application::Instance().GetSystemManager().GetSystem<PhysicsSystem>();
					if (physicsSystem)
					{
						Entity selected = this->m_Context->SelectedEntity;
						if (selected != Constants::Entities::InvalidEntityID && component.PreviewCollider)
							physicsSystem->SetColliderPreviewEntity(selected.GetEntityHandle());
						else
							physicsSystem->ClearColliderPreviewEntity();
					}
				}
				UI::PropertyGrid::End();
			}

			RenderComponentProperties(component);

			ImGui::PushID(&component);

			// Collision Offset Section
			if (ImGui::TreeNode("Collider Offset"))
			{
				if (UI::PropertyGrid::Begin("CollisionFilterProps"))
				{
					bool changed = false;
					changed |= UI::PropertyGrid::Float3("Position", component.Offset.Position);
					changed |= UI::PropertyGrid::Float3("Rotation", component.Offset.Rotation);
					if (changed)
						component.NeedsRebuild = true;

					UI::PropertyGrid::End();
				}

				ImGui::TreePop();
			}

			// Collision Filter Section
			if (ImGui::TreeNode("Collision Filters"))
			{
				if (UI::PropertyGrid::Begin("CollisionFilterProps"))
				{
					auto& filterManager = ProjectManager::GetActive()->GetCollisionFilterManager();
					if (UI::PropertyGrid::FilterGrid("Category", component.Category, filterManager))
						component.NeedsRebuild = true;
					if (UI::PropertyGrid::FilterGrid("Collision Mask", component.CollisionMask, filterManager))
						component.NeedsRebuild = true;

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			// Physics Material Section
			if (ImGui::TreeNode("Physics Material"))
			{
				RenderPhysicsMaterialSection(component);

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

	private:
		void RenderPhysicsMaterialSection(T& component)
		{
			bool matExists = component.PhysicsMaterialHandle != Constants::InvalidUUID;
			std::string matName = "None (Physics Material)";

			if (matExists)
			{
				auto matAsset = this->m_AssetManager.GetAsset<PhysicsMaterial>(component.PhysicsMaterialHandle);
				if (matAsset)
					matName = std::filesystem::path(matAsset->GetFilePath()).filename().string();
			}

			// 1. Flag to trigger the creation modal safely outside the menu
			bool openCreateModal = false;

			if (UI::PropertyGrid::Begin("PhysicsMaterialComponentProps"))
			{
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPhysicsMaterial);
				std::string droppedPath;

				auto browseFunc = [&]() {
					ImGui::OpenPopup("ChoosePhysicsMaterialPopup");
					};

				auto clearFunc = matExists ? UI::UICallbackFunc([&]() {
					component.PhysicsMaterialHandle = Constants::InvalidUUID;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("PhysicsMaterial", matName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto mat = this->m_AssetManager.Load<PhysicsMaterial>(droppedPath);
					if (mat)
						component.PhysicsMaterialHandle = mat->GetUUID(); // FIXED: Changed mesh->GetUUID() to mat->GetUUID()
				}

				ImGui::PushID("PhysicsMaterial");
				if (ImGui::BeginPopup("ChoosePhysicsMaterialPopup"))
				{
					if (ImGui::MenuItem("Create new Physics Material"))
					{
						// 2. Trigger the flag instead of opening the modal directly!
						openCreateModal = true;
					}

					if (ImGui::MenuItem("Load from file..."))
					{
						std::string defaultDir = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::PhysicsMaterial).string();
						std::string matFile = FileDialog::OpenFile(defaultDir.c_str(), "Physics Material (*.ebpmat)", "*.ebpmat;");
						if (!matFile.empty())
						{
							auto matAsset = this->m_AssetManager.Load<PhysicsMaterial>(matFile);
							if (matAsset)
								component.PhysicsMaterialHandle = matAsset->GetUUID();
						}
					}

					ImGui::EndPopup();
				}
				ImGui::PopID();

				UI::PropertyGrid::End();
			}

			// 3. Open the modal if the flag was set
			if (openCreateModal)
			{
				ImGui::OpenPopup("Create Physics Material");
			}

			// 4. Render the Creation Modal
			if (ImGui::BeginPopupModal("Create Physics Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				static char matNameBuffer[256] = "NewPhysicsMaterial";
				ImGui::InputText("Name", matNameBuffer, sizeof(matNameBuffer));

				ImGui::Spacing();

				if (ImGui::Button("Create", ImVec2(120, 0)))
				{
					// Use your AssetManager's variadic Create template to instantly build and register it!
					auto newMat = this->m_AssetManager.Create<PhysicsMaterial>(UUID(), std::string(matNameBuffer));
					std::string newMatName = newMat->GetName();

					// Assign the newly created UUID directly to the component
					component.PhysicsMaterialHandle = newMat->GetUUID();

					// Serialize the asset
					std::filesystem::path filePath = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::PhysicsMaterial) / (newMatName + ".ebpmat");
					newMat->SetFilePath(filePath.string());
					newMat->SetIsEngineAsset(false);

					if (!PhysicsMaterialSerializer::Serialize(filePath, newMat))
							EB_CORE_ERROR("Failed to serialize new physics material!");

					this->m_Context->ActiveScene()->RegisterAsset(newMat);
				
					// Reset the buffer for the next time the user creates a material
					strncpy_s(matNameBuffer, sizeof(matNameBuffer), "NewPhysicsMaterial", _TRUNCATE);
					ImGui::CloseCurrentPopup();

					// Notify the user of success
					auto evt = UINotificationEvent(std::format("New Physics Material {} Created!", newMatName));
					this->m_Context->EventCallback(evt);
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::Separator();

			// Render the physics material properties if one is assigned
			if (matExists)
			{
				auto matAsset = this->m_AssetManager.GetAsset<PhysicsMaterial>(component.PhysicsMaterialHandle);
				if (matAsset)
				{
					if (UI::PropertyGrid::Begin("PhysicsMaterialProps"))
					{
						if (UI::PropertyGrid::Float("Friction", matAsset->Friction, 0.01f, 0.0f, 1.0f))
							matAsset->Friction = std::clamp(matAsset->Friction, 0.0f, 1.0f);

						if (UI::PropertyGrid::Float("Bounciness", matAsset->Bounciness, 0.01f, 0.0f, 1.0f))
							matAsset->Bounciness = std::clamp(matAsset->Bounciness, 0.0f, 1.0f);

						UI::PropertyGrid::End();
					}
				}
			}
		}
	};

}
