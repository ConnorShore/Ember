#include "efpch.h"

#include "PrefabViewportViewer.h"
#include "EditorLayer.h"
#include "UI/DragDropTypes.h"

#include <imgui/imgui.h>

namespace Ember {
	PrefabViewportViewer::PrefabViewportViewer(SharedPtr<Scene> scene, SharedPtr<Prefab> prefab, Entity rootEntity, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Prefab, scene, filePath, title), PrefabAsset(prefab), RootEntity(rootEntity) {
	}

	void PrefabViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		// Note: The implementation is currently identical to SceneViewportViewer
		// but correctly decoupled so you can easily diverge the Prefab UI logic later.

		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();

		editor->SetViewportBounds(
			{ viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y },
			{ viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y }
		);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		Vector2f currentSize = editor->GetViewportSize();

		if (viewportPanelSize.x > 0.0f && viewportPanelSize.y > 0.0f && (currentSize.x != viewportPanelSize.x || currentSize.y != viewportPanelSize.y))
		{
			editor->SetViewportSize({ viewportPanelSize.x, viewportPanelSize.y });
			editor->GetOutputFramebuffer()->ViewportResize(static_cast<uint32_t>(viewportPanelSize.x), static_cast<uint32_t>(viewportPanelSize.y));

			if (auto activeScene = editor->GetContext().ActiveScene())
				activeScene->OnViewportResize(static_cast<uint32_t>(viewportPanelSize.x), static_cast<uint32_t>(viewportPanelSize.y));

			editor->GetCamera().SetViewportSize(static_cast<uint32_t>(viewportPanelSize.x), static_cast<uint32_t>(viewportPanelSize.y));
		}

		uint32_t textureID = editor->GetOutputFramebuffer()->GetColorAttachmentID(0);
		ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(textureID)), ImVec2{ viewportPanelSize.x, viewportPanelSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		if (ImGui::BeginDragDropTarget())
		{
			std::string payloadTypeModel = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetModel);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadTypeModel.c_str()))
			{
				std::string filePath = std::string((char*)payload->Data, payload->DataSize > 0 ? payload->DataSize - 1 : 0);
				editor->CreateEntityFromModel(filePath);
			}
			ImGui::EndDragDropTarget();
		}

		editor->GetViewportGizmos().Render(&editor->GetContext(), editor->GetCamera(), editor->GetViewportBounds(), editor->GetGizmoType());
	}
}