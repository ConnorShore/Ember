#include "efpch.h"

#include "SceneViewportViewer.h"
#include "EditorLayer.h"
#include "UI/DragDropTypes.h"

#include <imgui/imgui.h>

namespace Ember {

	SceneViewportViewer::SceneViewportViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Scene, scene, filePath, title) {
	}

	void SceneViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
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
			editor->GetCameraPreviewFramebuffer()->ViewportResize(static_cast<uint32_t>(viewportPanelSize.x), static_cast<uint32_t>(viewportPanelSize.y));

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

			std::string payloadTypePrefab = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPrefab);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadTypePrefab.c_str()))
			{
				std::string filePath = std::string((char*)payload->Data, payload->DataSize > 0 ? payload->DataSize - 1 : 0);
				editor->CreateEntityFromPrefab(filePath);
			}

			std::string payloadTypeScene = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::Scene);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadTypeScene.c_str()))
			{
				std::string filePath = std::string((char*)payload->Data, payload->DataSize > 0 ? payload->DataSize - 1 : 0);
				editor->OpenScene(filePath);
			}
			ImGui::EndDragDropTarget();
		}

		auto& context = editor->GetContext();
		if (context.CurrentSceneState == SceneState::Edit
			&& context.SelectedEntity != Constants::Entities::InvalidEntityID
			&& context.SelectedEntity.ContainsComponent<CameraComponent>())
		{
			ImVec2 vMinRegion = ImGui::GetWindowContentRegionMin();
			ImVec2 vMaxRegion = ImGui::GetWindowContentRegionMax();
			ImVec2 vOffset = ImGui::GetWindowPos();

			float padding = 15.0f;
			float previewW = 320.0f;
			float previewH = 180.0f;
			ImVec2 previewPos = ImVec2(vOffset.x + vMinRegion.x + padding, vOffset.y + vMaxRegion.y - previewH - padding);
			ImVec2 previewMax = ImVec2(previewPos.x + previewW, previewPos.y + previewH);

			editor->SetCameraPreviewViewportSize(Vector2f(previewW, previewH));
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			drawList->AddRectFilled(
				ImVec2(previewPos.x - 5.0f, previewPos.y + 5.0f),
				ImVec2(previewMax.x - 5.0f, previewMax.y + 5.0f),
				IM_COL32(0, 0, 0, 85)
			);

			ImGui::SetCursorScreenPos(previewPos);
			uint32_t camTexID = editor->GetCameraPreviewFramebuffer()->GetColorAttachmentID(0);
			ImGui::Image((ImTextureID)(intptr_t)camTexID, ImVec2(previewW, previewH), ImVec2(0, 1), ImVec2(1, 0));
			drawList->AddRect(previewPos, previewMax, IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.0f);
		}

		editor->GetViewportGizmos().Render(&context, editor->GetCamera(), editor->GetViewportBounds(), editor->GetGizmoType());
	}
}