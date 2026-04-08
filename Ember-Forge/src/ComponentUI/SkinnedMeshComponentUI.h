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
			ImGui::Text("Skinned mesh component data here...");
		}

	};

}