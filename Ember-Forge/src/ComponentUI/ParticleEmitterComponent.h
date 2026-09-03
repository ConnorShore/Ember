#pragma once
#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "Ui/ParticleEffectPropertiesUI.h"

#include <Ember/Asset/Serializers/ParticleEffectSerializer.h>
#include <Ember/Asset/Serializers/AssetRegistrySerializer.h>

#include <imgui/imgui.h>

namespace Ember {

	class ParticleEmitterComponentUI : public ComponentUI<ParticleEmitterComponent>
	{
	public:
		ParticleEmitterComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Particle Emitter Component"; }

	protected:
		inline void RenderComponentImpl(ParticleEmitterComponent& component) override
		{
			// Push the component's memory address as an ID so multiple emitters don't clash in ImGui
			ImGui::PushID(&component);

			auto effect = component.ResolveEffect();
			auto instance = DynamicPointerCast<ParticleEffectInstance>(effect);
			bool isInstanced = instance != nullptr;

			if (UI::PropertyGrid::Begin("ParticleEmitterProps"))
			{
				UI::PropertyGrid::Checkbox("Active", component.IsActive);

				// An instance has no file of its own, so show the asset it was cloned from
				auto displayedEffect = isInstanced ? instance->GetSourceEffect() : effect;
				std::string effectName = displayedEffect ? displayedEffect->GetName() : "None (Particle Effect)";
				if (isInstanced)
					effectName += "  (instance)";

				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetParticleEffect);
				std::string droppedPath;

				UI::UICallbackFunc browseFunc = [&]() {
					auto effectDir = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::ParticleEffect);
					std::string extensions = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetParticleEffect);
					std::string effectFile = FileDialog::OpenFile(effectDir.string().c_str(), std::format("Particle Effects ({})", extensions).c_str(), extensions.c_str());
					if (!effectFile.empty())
					{
						auto loaded = m_AssetManager.Load<ParticleEffect>(effectFile, false);
						if (loaded)
							component.EffectHandle = loaded->GetUUID();
					}
					};

				if (UI::PropertyGrid::AssetReference("Particle Effect", effectName, payloadType, droppedPath, browseFunc, nullptr))
				{
					auto loaded = m_AssetManager.Load<ParticleEffect>(droppedPath, false);
					if (loaded)
						component.EffectHandle = loaded->GetUUID();
				}

				UI::PropertyGrid::End();
			}

			bool openCreateModal = false;
			if (ImGui::Button("New Effect"))
				openCreateModal = true;

			auto sourceEffect = isInstanced ? instance->GetSourceEffect() : effect;
			if (sourceEffect && !sourceEffect->GetFilePath().empty())
			{
				ImGui::SameLine();
				if (ImGui::Button("Edit Effect"))
					m_Context->RequestParticleEffectOpenPath = sourceEffect->GetFilePath();
			}

			if (isInstanced)
			{
				ImGui::SameLine();
				if (ImGui::Button("Revert to Asset"))
				{
					component.RevertToSourceEffect();
					effect = component.ResolveEffect();
					instance = nullptr;
					isInstanced = false;
				}
			}

			if (isInstanced)
				ImGui::TextDisabled("Overridden on this entity only.");
			else if (effect)
				ImGui::TextDisabled("Shared asset - edits here create a per-entity instance.");

			ImGui::Separator();

			// Editing any property clones the shared asset first, so tweaking one emitter never
			// silently retunes every other user of the same effect.
			if (effect)
			{
				ParticleEffectProperties preview = effect->Properties;
				if (UI::DrawParticleEffectProperties(preview, m_AssetManager, "EmitterEffectProps"))
				{
					auto editable = component.GetInstanced(effect->GetName() + "_Instance");
					if (editable)
						editable->Properties = preview;
				}
			}

			if (openCreateModal)
				ImGui::OpenPopup("Create Particle Effect");

			RenderCreateEffectModal(component);

			ImGui::PopID();
		}

	private:
		inline void RenderCreateEffectModal(ParticleEmitterComponent& component)
		{
			if (!ImGui::BeginPopupModal("Create Particle Effect", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				return;

			static char effectNameBuffer[256] = "NewParticleEffect";
			ImGui::InputText("Name", effectNameBuffer, sizeof(effectNameBuffer));

			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				std::string newEffectName(effectNameBuffer);
				auto effectDir = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::ParticleEffect);
				std::filesystem::create_directories(effectDir);

				std::filesystem::path filePath = effectDir / (newEffectName + ".ebparticle");

				auto newEffect = m_AssetManager.Create<ParticleEffect>(UUID(), newEffectName);
				newEffect->SetFilePath(filePath.string());
				newEffect->SetIsEngineAsset(false);

				if (ParticleEffectSerializer::Serialize(filePath, newEffect))
				{
					m_AssetManager.Register<ParticleEffect>(newEffect);
					component.EffectHandle = newEffect->GetUUID();

					AssetRegistrySerializer registrySerializer(&m_AssetManager);
					registrySerializer.Serialize(ProjectManager::GetActive()->GetAssetsFilePath().string());

					m_Context->RequestParticleEffectOpenPath = filePath.string();
				}
				else
				{
					EB_CORE_ERROR("Failed to serialize new particle effect!");
				}

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	};

}
