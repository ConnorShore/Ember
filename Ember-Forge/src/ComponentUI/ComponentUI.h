#pragma once

#include "EditorContext.h"

#include <Ember.h>
#include <string>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Component UI Base Class
	//////////////////////////////////////////////////////////////////////////

	class ComponentUIBase
	{
	public:
		virtual ~ComponentUIBase() = default;
		virtual void Render(Entity entity) = 0;
		virtual const char* GetName() const = 0;
		virtual void CreateComponentForEntity(Entity entity) = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Component UI Template Class
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	class ComponentUI : public ComponentUIBase
	{
	public:
		ComponentUI(EditorContext* context) : m_Context(context) {}
		virtual ~ComponentUI() = default;

		virtual void Render(Entity entity) override
		{
			RenderComponent(GetName(), entity, [this](T& component) { RenderComponentImpl(component); });
		}

		virtual const char* GetName() const = 0;
		virtual void CreateComponentForEntity(Entity entity) override
		{
			if (entity.ContainsComponent<T>())
			{
				EB_CORE_WARN("Entity already contains a component of type %s!", GetName());
				return;
			}

			T comp = {};
			m_Context->ActiveScene->AttachComponent(entity, comp);
		}

	protected:
		virtual void RenderComponentImpl(T& component) = 0;

	protected:
		EditorContext* m_Context;
		bool m_CanRemove = true;

    private:

		template<typename UIFunction>
		inline void RenderComponent(const char* name, Entity entity, UIFunction uiFunction)
		{
			// If the entity doesn't have this component, don't draw anything!
			if (!entity.ContainsComponent<T>())
				return;

			const ImGuiTreeNodeFlags treeNodeFlags =
				ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_AllowOverlap |
				ImGuiTreeNodeFlags_FramePadding;

			auto& component = entity.GetComponent<T>();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name);
			ImGui::PopStyleVar();

			if (m_CanRemove)
			{
				// Draw Remove Button
				float buttonWidth = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
				ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonWidth - 5.0f);

				// Center button vertically with the header text
				float currentCursorY = ImGui::GetCursorPosY();
				ImGui::SetCursorPosY(currentCursorY + 2.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 2 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				if (ImGui::Button("Remove"))
					entity.DetachComponent<T>();
				ImGui::PopStyleVar();
				ImGui::PopStyleVar();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}
		}
	};

}