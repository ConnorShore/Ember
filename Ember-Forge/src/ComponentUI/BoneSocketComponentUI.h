#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"

#include <Ember/Asset/Skeleton.h>
#include <Ember/ECS/System/BoneSocketSystem.h>

namespace Ember {

	class BoneSocketComponentUI : public ComponentUI<BoneSocketComponent>
	{
	public:
		BoneSocketComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Bone Socket Component"; }

	protected:
		inline void RenderComponentImpl(BoneSocketComponent& component) override
		{
			if (UI::PropertyGrid::Begin("BoneSocketComponentProps"))
			{
				RenderTargetEntitySelector(component);
				RenderBoneSelector(component);

				UI::PropertyGrid::Float3("Position", component.Position);

				Vector3f rotationDegrees = Vector3f(
					Math::Degrees(component.Rotation.x),
					Math::Degrees(component.Rotation.y),
					Math::Degrees(component.Rotation.z)
				);

				if (UI::PropertyGrid::Float3("Rotation", rotationDegrees, 1.0f))
				{
					component.Rotation = Vector3f(
						Math::Radians(rotationDegrees.x),
						Math::Radians(rotationDegrees.y),
						Math::Radians(rotationDegrees.z)
					);
				}

				UI::PropertyGrid::Float3("Scale", component.Scale);

				UI::PropertyGrid::End();
			}
		}

	private:
		void RenderTargetEntitySelector(BoneSocketComponent& component)
		{
			SharedPtr<Scene> scene = m_Context->ActiveScene();
			std::string targetName = "None";

			Entity targetEntity = scene->GetEntity(component.TargetEntityHandle);
			if (targetEntity != Constants::Entities::InvalidEntityID)
				targetName = targetEntity.GetName();
			else if (component.TargetEntityHandle != Constants::InvalidUUID)
				targetName = "Invalid Entity";

			if (UI::PropertyGrid::BeginComboBox("Target Entity", targetName.c_str()))
			{
				if (UI::PropertyGrid::ComboBoxItem("None", component.TargetEntityHandle == Constants::InvalidUUID))
					SetTargetEntity(component, Constants::InvalidUUID);

				ImGui::Separator();

				std::unordered_set<UUID> addedEntities;
				for (Entity entity : scene->GetAllEntitiesWithComponents<AnimatorComponent>())
				{
					addedEntities.insert(entity.GetUUID());
					RenderTargetEntityOption(component, entity, " (Animator)");
				}

				for (Entity entity : scene->GetAllEntitiesWithComponents<SkinnedMeshComponent>())
				{
					if (addedEntities.find(entity.GetUUID()) != addedEntities.end())
						continue;

					RenderTargetEntityOption(component, entity, " (Skinned Mesh)");
				}

				UI::PropertyGrid::EndComboBox();
			}

			ImGui::SameLine();
			if (ImGui::Button("->"))
			{
				if (targetEntity != Constants::Entities::InvalidEntityID)
					m_Context->SelectedEntity = targetEntity;
			}
		}

		void RenderTargetEntityOption(BoneSocketComponent& component, Entity entity, const char* suffix)
		{
			std::string label = entity.GetName() + suffix;
			bool isSelected = component.TargetEntityHandle == entity.GetUUID();

			if (UI::PropertyGrid::ComboBoxItem(label.c_str(), isSelected))
				SetTargetEntity(component, entity.GetUUID());

			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}

		void RenderBoneSelector(BoneSocketComponent& component)
		{
			auto skeleton = GetTargetSkeleton(component);
			if (!skeleton)
			{
				if (UI::PropertyGrid::InputText("Bone Name", component.BoneName))
				{
					ResetBoneCache(component);
					PreserveCurrentWorldTransform(component);
				}
				return;
			}

			std::string boneName = component.BoneName.empty() ? "None" : component.BoneName;
			if (UI::PropertyGrid::BeginComboBox("Bone", boneName.c_str()))
			{
				if (UI::PropertyGrid::ComboBoxItem("None", component.BoneName.empty()))
				{
					component.BoneName.clear();
					ResetBoneCache(component);
					PreserveCurrentWorldTransform(component);
				}

				ImGui::Separator();

				const auto& bones = skeleton->GetBones();
				for (const Bone& bone : bones)
				{
					bool isSelected = component.BoneName == bone.Name;
					if (UI::PropertyGrid::ComboBoxItem(bone.Name.c_str(), isSelected))
					{
						component.BoneName = bone.Name;
						ResetBoneCache(component);
						PreserveCurrentWorldTransform(component);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				UI::PropertyGrid::EndComboBox();
			}
		}

		SharedPtr<Skeleton> GetTargetSkeleton(const BoneSocketComponent& component)
		{
			Entity targetEntity = m_Context->ActiveScene()->GetEntity(component.TargetEntityHandle);
			if (targetEntity == Constants::Entities::InvalidEntityID)
				return nullptr;

			Entity animatorEntity = targetEntity;
			if (targetEntity.ContainsComponent<SkinnedMeshComponent>())
			{
				auto& skinnedMesh = targetEntity.GetComponent<SkinnedMeshComponent>();
				animatorEntity = m_Context->ActiveScene()->GetEntity(skinnedMesh.AnimatorEntityHandle);
			}

			if (animatorEntity == Constants::Entities::InvalidEntityID || !animatorEntity.ContainsComponent<AnimatorComponent>())
				return nullptr;

			auto& animator = animatorEntity.GetComponent<AnimatorComponent>();
			if (animator.SkeletonHandle == Constants::InvalidUUID)
				return nullptr;

			return m_AssetManager.GetAsset<Skeleton>(animator.SkeletonHandle);
		}

		void SetTargetEntity(BoneSocketComponent& component, UUID entityHandle)
		{
			component.TargetEntityHandle = entityHandle;
			ResetBoneCache(component);
			PreserveCurrentWorldTransform(component);
		}

		void ResetBoneCache(BoneSocketComponent& component)
		{
			component.RuntimeBoneIndex = -1;
			component.RuntimeBoneName.clear();
		}

		void PreserveCurrentWorldTransform(BoneSocketComponent& component)
		{
			Entity entity = m_Context->SelectedEntity;
			if (entity == Constants::Entities::InvalidEntityID || !entity.ContainsComponent<TransformComponent>())
				return;

			BoneSocketSystem::SetOffsetFromWorldTransform(component, entity.GetComponent<TransformComponent>().WorldTransform, m_Context->ActiveScene().Ptr());
		}
	};

}