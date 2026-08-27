#include "efpch.h"
#include "SceneInspectorPanel.h"

#include "Undo/ScopedEntityEdit.h"

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
#include "ComponentUI/UISelectableComponentUI.h"
#include "ComponentUI/UIButtonComponentUI.h"
#include "ComponentUI/UIToggleComponentUI.h"
#include "ComponentUI/RectTransformComponentUI.h"
#include "ComponentUI/NavigationMeshComponentUI.h"
#include "ComponentUI/NavigationMeshModifierComponentUI.h"

namespace Ember {

	static std::string GetCategoryName(SceneInspectorPanel::Category category)
	{
		switch (category)
		{
		case SceneInspectorPanel::Category::Transform: return "Transform";
		case SceneInspectorPanel::Category::Rendering: return "Rendering";
		case SceneInspectorPanel::Category::Lighting: return "Lighting";
		case SceneInspectorPanel::Category::Camera: return "Camera";
		case SceneInspectorPanel::Category::Physics: return "Physics";
		case SceneInspectorPanel::Category::Audio: return "Audio";
		case SceneInspectorPanel::Category::Animation: return "Animation";
		case SceneInspectorPanel::Category::Scripting: return "Scripting";
		case SceneInspectorPanel::Category::AI: return "AI & Navigation";
		case SceneInspectorPanel::Category::UI: return "UI";
		case SceneInspectorPanel::Category::Gameplay: return "Gameplay";
		case SceneInspectorPanel::Category::Effects: return "Effects";
		case SceneInspectorPanel::Category::Miscellaneous: return "Miscellaneous";
		default: return "Unknown";
		}
	}

	SceneInspectorPanel::SceneInspectorPanel(EditorContext* context)
		: InspectorPanelContent(context)
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
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<NavigationMeshComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<NavigationGridComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<NavigationMeshModifierComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::AI].emplace_back(ScopedPtr<WaypointComponentUI>::Create(m_Context));

		// --- UI ---
		m_ComponentUIs[Category::UI].emplace_back(ScopedPtr<CanvasComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::UI].emplace_back(ScopedPtr<UISelectableComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::UI].emplace_back(ScopedPtr<UIButtonComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::UI].emplace_back(ScopedPtr<UIToggleComponentUI>::Create(m_Context));
	}

	SceneInspectorPanel::~SceneInspectorPanel()
	{
	}

	void SceneInspectorPanel::OnImGuiRender()
	{
		auto activeScene = m_Context->ActiveScene();
		Entity entity = activeScene ? activeScene->GetEntityByHandle(m_Context->SelectedEntity.GetEntityHandle()) : Entity();
		if (entity == Constants::Entities::InvalidEntityID)
		{
			// Blank panel if no entity selected
			ImGui::Text("Select an Entity to inspect properties");
			return;
		}

		bool isActive = !entity.ContainsComponent<DisabledComponent>();
		if (ImGui::Checkbox("##Active", &isActive))
		{
			entity.SetActive(isActive, true);
		}

		ImGui::SameLine();

		// Entity Header
		DrawEntityHeader(entity);

		// Only the active entity is edited here, so say so rather than letting a multi-selection look
		// like the properties apply to all of it.
		if (m_Context->SelectionCount() > 1)
			ImGui::TextDisabled("%zu selected - editing %s", m_Context->SelectionCount(), entity.GetName().c_str());

		ImGui::Separator();

		// --- SCROLLABLE REGION ---

		// Create a child region that takes up the remaining width and height (ImVec2(0, 0))
		// The false parameter means it won't draw a border around the child region
		if (ImGui::BeginChild("ComponentRegion", ImVec2(0, 0), false, ImGuiWindowFlags_None))
			RenderEntityComponents(entity);
		ImGui::EndChild(); // End the scrollable region
	}

	void SceneInspectorPanel::DrawEntityHeader(Entity entity)
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
							{
								ScopedEntityEdit edit(*m_Context, "Add Component", { entity.GetUUID() }, false);
								comp->CreateComponentForEntity(entity);
							}
						}
						ImGui::EndMenu();
					}
				}

				ImGui::EndPopup();
			}

			UI::PropertyGrid::End();
		}
	}

	void SceneInspectorPanel::RenderEntityComponents(Entity entity)
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

	ComponentUIBase* SceneInspectorPanel::FindComponentUI(ComponentType componentType, Entity entity) const
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