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
#include "ComponentUI/Collision/BoxColliderComponentUI.h"
#include "ComponentUI/Collision/SphereColliderComponentUI.h"
#include "ComponentUI/Collision/CapsuleColliderComponent.h"
#include "ComponentUI/Collision/ConcaveMeshColliderComponentUI.h"
#include "ComponentUI/Collision/ConvexMeshColliderComponentUI.h"
#include "ComponentUI/CharacterControllerComponent.h"

#include <imgui/imgui.h>

#include <format>

namespace Ember {

	static std::string GetCategoryName(InspectorPanel::Category category)
	{
		switch (category)
		{
		case InspectorPanel::Category::Core: return "Core";
		case InspectorPanel::Category::Rendering: return "Rendering";
		case InspectorPanel::Category::Lighting: return "Lighting";
		case InspectorPanel::Category::Physics: return "Physics";
		case InspectorPanel::Category::Animation: return "Animation";
		case InspectorPanel::Category::Scripting: return "Scripts";
		default: return "Unknown";
		}
	}

	InspectorPanel::InspectorPanel(EditorContext* context)
		: Panel("Inspector", context)
	{
		// --- CORE ---
		m_ComponentUIs[Category::Core].emplace_back(ScopedPtr<TransformComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Core].emplace_back(ScopedPtr<CharacterControllerComponentUI>::Create(m_Context));

		// --- RENDERING ---
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<CameraComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<StaticMeshComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<SkinnedMeshComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Rendering].emplace_back(ScopedPtr<MaterialComponentUI>::Create(m_Context));

		// --- LIGHTING ---
		m_ComponentUIs[Category::Lighting].emplace_back(ScopedPtr<DirectionalLightComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Lighting].emplace_back(ScopedPtr<PointLightComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Lighting].emplace_back(ScopedPtr<SpotLightComponentUI>::Create(m_Context));

		// --- PHYSICS ---
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<RigidBodyComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<BoxColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<SphereColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<CapsuleColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<ConvexMeshColliderComponentUI>::Create(m_Context));
		m_ComponentUIs[Category::Physics].emplace_back(ScopedPtr<ConcaveMeshColliderComponentUI>::Create(m_Context));

		// --- ANIMATION ---
		m_ComponentUIs[Category::Animation].emplace_back(ScopedPtr<AnimatorComponentUI>::Create(m_Context));

		// --- SCRIPTING ---
		m_ComponentUIs[Category::Scripting].emplace_back(ScopedPtr<ScriptComponentUI>::Create(m_Context));
	}

	InspectorPanel::~InspectorPanel()
	{
	}

	void InspectorPanel::OnEvent(Event& event)
	{
	}

	void InspectorPanel::OnImGuiRender()
	{
		if (m_Context->SelectedEntity == Constants::Entities::InvalidEntityID)
		{
			// Blank panel if no entity selected
			ImGui::Begin(m_Title.c_str());
			ImGui::Text("Select an Entity to inspect properties");
			ImGui::End();
			return;
		}

		Entity entity = m_Context->SelectedEntity;
		{
			ImGui::Begin(m_Title.c_str());

			// Entity Header
			DrawEntityHeader(entity);

			// Entity Components
			for (auto& [Category, components] : m_ComponentUIs)
			{
				for (auto& componentUI : components)
				{
					componentUI->Render(entity);
				}
			}

			ImGui::End();
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
							{
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

}