#include "efpch.h"
#include "InspectorPanel.h"

#include "ComponentUI/TransformComponentUI.h"
#include "ComponentUI/CameraComponentUI.h"
#include "ComponentUI/DirectionalLightComponentUI.h"
#include "ComponentUI/PointLightComponentUI.h"
#include "ComponentUI/SpotLightComponentUI.h"
#include "ComponentUI/RigidBodyComponentUI.h"
#include "ComponentUI/ScriptComponentUI.h"
#include "ComponentUI/StaticMeshComponentUI.h"
#include "ComponentUI/SkinnedMeshComponentUI.h"
#include "ComponentUI/MaterialComponentUI.h"
#include "ComponentUI/AnimatorComponentUI.h"
#include "ComponentUI/BoneSocketComponentUI.h"
#include "ComponentUI/Collision/BoxColliderComponentUI.h"
#include "ComponentUI/Collision/SphereColliderComponentUI.h"
#include "ComponentUI/Collision/CapsuleColliderComponent.h"
#include "ComponentUI/Collision/ConcaveMeshColliderComponentUI.h"
#include "ComponentUI/Collision/ConvexMeshColliderComponentUI.h"
#include "ComponentUI/CharacterControllerComponent.h"
#include "ComponentUI/LifetimeComponentUI.h"
#include "ComponentUI/TextComponentUI.h"
#include "ComponentUI/PoolConfigComponentUI.h"
#include "ComponentUI/ParticleEmitterComponent.h"
#include "ComponentUI/PostProcessVolumeComponentUI.h"
#include "ComponentUI/AudioSourceComponentUI.h"
#include "ComponentUI/AudioListenerComponentUI.h"
#include "ComponentUI/WaypointComponentUI.h"
#include "ComponentUI/AIPathComponentUI.h"
#include "ComponentUI/NavigationGridComponentUI.h"
#include "ComponentUI/AIAgentComponentUI.h"
#include "ComponentUI/LocalAvoidanceComponentUI.h"
#include "ComponentUI/SpriteComponentUI.h"
#include "ComponentUI/CanvasComponentUI.h"
#include "ComponentUI/RectTransformComponentUI.h"

#include <imgui/imgui.h>

#include <format>
#include <unordered_set>

namespace Ember {

	static std::string GetCategoryName(InspectorPanel::Category category)
	{
		switch (category)
		{
		case InspectorPanel::Category::Transform: return "Transform";
		case InspectorPanel::Category::Rendering: return "Rendering";
		case InspectorPanel::Category::Lighting: return "Lighting";
		case InspectorPanel::Category::Camera: return "Camera";
		case InspectorPanel::Category::Physics: return "Physics";
		case InspectorPanel::Category::Audio: return "Audio";
		case InspectorPanel::Category::Animation: return "Animation";
		case InspectorPanel::Category::Scripting: return "Scripting";
		case InspectorPanel::Category::AI: return "AI & Navigation";
		case InspectorPanel::Category::UI: return "UI";
		case InspectorPanel::Category::Gameplay: return "Gameplay";
		case InspectorPanel::Category::Effects: return "Effects";
		case InspectorPanel::Category::Miscellaneous: return "Miscellaneous";
		default: return "Unknown";
		}
	}

	InspectorPanel::InspectorPanel(EditorContext* context)
		: Panel("Inspector", context)
	{
		// --- TRANSFORM ---
		m_ComponentUIs[Category::Transform].emplace_back(ScopedPtr<TransformComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Transform].emplace_back(ScopedPtr<RectTransformComponentUI>::Create(m_Context));

		// --- CAMERA ---
		m_ComponentUIs[Category::Camera].emplace_back(ScopedPtr<CameraComponentUI>::Create(m_Context));

		// --- RENDERING ---
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<StaticMeshComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<SkinnedMeshComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<MaterialComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<SpriteComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<TextComponentUI>::Create(m_Context));

		// --- LIGHTING ---
		m_ComponentUIs[Category::Lighting].emplace_back(ScopedPtr<DirectionalLightComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Lighting].emplace_back(ScopedPtr<PointLightComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Lighting].emplace_back(ScopedPtr<SpotLightComponentUI>::Create(m_Context));

		// --- PHYSICS ---
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<RigidBodyComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<CharacterControllerComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<BoxColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<SphereColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<CapsuleColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<ConvexMeshColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<ConcaveMeshColliderComponentUI>::Create(m_Context));

		// --- AUDIO ---
		m_ComponentUIs[Category::Audio].emplace_back(ScopedPtr<AudioSourceComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Audio].emplace_back(ScopedPtr<AudioListenerComponentUI>::Create(m_Context));

		// --- ANIMATION ---
		m_ComponentUIs[Category::Animation].emplace_back(ScopedPtr<AnimatorComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Animation].emplace_back(ScopedPtr<BoneSocketComponentUI>::Create(m_Context));

		// --- SCRIPTING ---
		m_ComponentUIs[Category::Scripting].emplace_back(ScopedPtr<ScriptComponentUI>::Create(m_Context));

		// --- GAMEPLAY ---
		m_ComponentUIs[Category::Gameplay].emplace_back(ScopedPtr<LifetimeComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Gameplay].emplace_back(ScopedPtr<PoolConfigComponentUI>::Create(m_Context));

		// --- EFFECTS ---
		m_ComponentUIs[Category::Effects].emplace_back(ScopedPtr<ParticleEmitterComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Effects].emplace_back(ScopedPtr<PostProcessVolumeComponentUI>::Create(m_Context));

		// --- AI ---
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<AIAgentComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<AIPathComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<LocalAvoidanceComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<NavigationGridComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<WaypointComponentUI>::Create(m_Context));

		// --- UI ---
		m_ComponentUIs[Category::UI].emplace_back(ScopedPtr<CanvasComponentUI>::Create(m_Context));
	}

	InspectorPanel::~InspectorPanel()
	{
	}

	void InspectorPanel::OnEvent(Event& event)
	{
	}

	void InspectorPanel::OnImGuiRender()
	{
		if (m_Context->SelectedEntity == Constants::Entities::InvalidEntityID
			|| !m_Context->SelectedEntity.ContainsComponent<IDComponent>()
			|| !m_Context->SelectedEntity.ContainsComponent<TagComponent>())
		{
			m_Context->SelectedEntity = Entity();

			// Blank panel if no entity selected
			ImGui::Begin(m_Title.c_str());
			ImGui::Text("Select an Entity to inspect properties");
			ImGui::End();
			return;
		}

		Entity entity = m_Context->SelectedEntity;
		{
			ImGui::Begin(m_Title.c_str());

			bool isActive = !entity.ContainsComponent<DisabledComponent>();
			if (ImGui::Checkbox("##Active", &isActive))
			{
				if (isActive)
				{
					if (entity.ContainsComponent<DisabledComponent>())
						entity.DetachComponent<DisabledComponent>();

					for (auto& child : entity.GetAllChildren())
					{
						if (child && child.ContainsComponent<DisabledComponent>())
							child.DetachComponent<DisabledComponent>();
					}
				}
				else
				{
					if (!entity.ContainsComponent<DisabledComponent>())
						entity.AttachComponent<DisabledComponent>();

					for (auto& child : entity.GetAllChildren())
					{
						if (child && !child.ContainsComponent<DisabledComponent>())
							child.AttachComponent<DisabledComponent>();
					}
				}
			}

			ImGui::SameLine();

			// Entity Header
			DrawEntityHeader(entity);

			ImGui::Separator();

			// --- SCROLLABLE REGION ---

			// Create a child region that takes up the remaining width and height (ImVec2(0, 0))
			// The false parameter means it won't draw a border around the child region
			if (ImGui::BeginChild("ComponentRegion", ImVec2(0, 0), false, ImGuiWindowFlags_None))
			{
				RenderEntityComponents(entity);
			}
			ImGui::EndChild(); // End the scrollable region

			ImGui::End(); // End the main Inspector window
		}
	}

	void InspectorPanel::DrawEntityHeader(Entity entity)
	{
		if (UI::PropertyGrid::Begin("EntityHeader"))
		{
			if (UI::PropertyGrid::HeaderWithActionButton(entity.GetName().c_str(), "Add Component"))
			{
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup"))
			{
				for (auto& [category, comps] : m_ComponentUIs)
				{
					if (ImGui::BeginMenu(GetCategoryName(category).c_str()))
					{
						for (auto& comp : comps)
						{
							if (ImGui::MenuItem(comp->GetName()))
								comp->CreateComponentForEntity(entity);
						}
						ImGui::EndMenu();
					}
				}

				ImGui::EndPopup();
			}

			UI::PropertyGrid::End();
		}
	}

	void InspectorPanel::RenderEntityComponents(Entity entity)
	{
		ComponentType transformComponentType = entity.GetComponentType<TransformComponent>();
		bool hasRectTransform = entity.ContainsComponent<RectTransformComponent>();
		std::unordered_set<ComponentType> renderedComponentTypes;

		for (ComponentType componentType : entity.GetComponentOrder())
		{
			ComponentUIBase* componentUI = FindComponentUI(componentType, entity);
			if (!componentUI)
				continue;

			if (hasRectTransform && componentType == transformComponentType)
				continue;

			componentUI->Render(entity);
			renderedComponentTypes.insert(componentType);
		}

		for (auto& [category, components] : m_ComponentUIs)
		{
			for (auto& componentUI : components)
			{
				ComponentType componentType = componentUI->GetComponentType(entity);
				if (renderedComponentTypes.contains(componentType))
					continue;

				if (hasRectTransform && componentType == transformComponentType)
					continue;

				if (!entity.ContainsComponent(componentType))
					continue;

				componentUI->Render(entity);
				renderedComponentTypes.insert(componentType);
			}
		}
	}

	ComponentUIBase* InspectorPanel::FindComponentUI(ComponentType componentType, Entity entity) const
	{
		for (auto& [category, components] : m_ComponentUIs)
		{
			for (auto& componentUI : components)
			{
				if (componentUI->GetComponentType(entity) == componentType)
					return componentUI.Ptr();
			}
		}

		return nullptr;
	}

}