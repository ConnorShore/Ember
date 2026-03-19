#pragma once

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
		virtual void Draw(Entity entity) = 0;
		virtual const std::string& GetName() const = 0;
	};

	//////////////////////////////////////////////////////////////////////////
	// Component UI Template Class
	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	class ComponentUI : public ComponentUIBase
	{
	public:
		virtual ~ComponentUI() = default;

		virtual void Draw(Entity entity) override
		{
			DrawComponent(GetName(), entity, [this](T& component) { DrawComponentImpl(component); });
		}

		virtual const std::string& GetName() const = 0;

	protected:
		virtual void DrawComponentImpl(T& component) = 0;

    private:

		template<typename UIFunction>
		static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
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

			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, std::format("{} Component", name).c_str());

			ImGui::PopStyleVar();

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}
		}
	};

}