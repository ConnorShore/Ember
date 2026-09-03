#include "efpch.h"
#include "ParticleEffectViewportViewer.h"
#include "EditorLayer.h"

#include "UI/ParticleEffectPropertiesUI.h"

#include <Ember/Asset/AssetManager.h>
#include <Ember/Asset/Serializers/ParticleEffectSerializer.h>
#include <Ember/ECS/System/ParticleSystem.h>
#include <Ember/Event/UIEvent.h>
#include <Ember/Render/RenderAction.h>
#include <Ember/Scene/Scene.h>

#include <imgui/imgui.h>

namespace Ember {

	ParticleEffectViewportViewer::ParticleEffectViewportViewer(SharedPtr<ParticleEffect> effect, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Particle, nullptr, filePath, title), m_Effect(effect)
	{
		FramebufferSpecification specs;
		specs.Width = 800;
		specs.Height = 600;
		specs.AttachmentSpecs = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::Depth24Stencil8 };
		m_PreviewFramebuffer = Framebuffer::Create(specs);

		m_PreviewCamera = EditorCamera(50.0f, (float)specs.Width / (float)specs.Height, 0.1f, 100.0f);
		m_PreviewCamera.SetFocalPoint(Vector3f(0.0f, 0.5f, 0.0f));
		m_PreviewCamera.SetPitch(Math::Radians(12.0f));
		m_PreviewCamera.SetYaw(Math::Radians(0.0f));
		m_PreviewCamera.SetDistance(4.0f);
		m_PreviewCamera.SetViewportSize(specs.Width, specs.Height);

		// A private scene so the preview's particles never mix with the scene being edited
		m_PreviewScene = SharedPtr<Scene>::Create("ParticleEffectPreview", "");
		m_PreviewScene->OnViewportResize(specs.Width, specs.Height);

		Entity emitter = m_PreviewScene->AddEntity("PreviewEmitter");
		UUID effectHandle = m_Effect ? m_Effect->GetUUID() : (UUID)Constants::InvalidUUID;
		emitter.AttachComponent<ParticleEmitterComponent>(ParticleEmitterComponent(effectHandle));

		m_PreviewSize = { (float)specs.Width, (float)specs.Height };
	}

	ParticleEffectViewportViewer::~ParticleEffectViewportViewer()
	{
	}

	void ParticleEffectViewportViewer::MarkDirty()
	{
		m_IsDirty = true;
		m_SaveCooldown = AUTO_SAVE_DELAY;
	}

	void ParticleEffectViewportViewer::RestartPreview()
	{
		if (m_PreviewScene)
			m_PreviewScene->GetParticleManager().Reset();
	}

	void ParticleEffectViewportViewer::OnUpdate(TimeStep delta, EditorLayer* editor)
	{
		if (!m_PreviewScene || !m_PreviewFramebuffer)
			return;

		if (m_PreviewHovered)
			m_PreviewCamera.OnUpdate(delta);

		// OnUpdateEdit deliberately doesn't run the particle system, so the preview drives it itself
		if (m_IsPlaying)
		{
			auto particleSystem = Application::Instance().GetSystem<ParticleSystem>();
			if (particleSystem)
				particleSystem->OnUpdate(delta, m_PreviewScene.Ptr());
		}

		m_ActiveParticleCount = m_PreviewScene->GetParticleManager().GetActiveParticleCount();

		m_PreviewFramebuffer->Bind();
		RenderAction::SetViewport(0, 0, m_PreviewFramebuffer->GetSpecification().Width, m_PreviewFramebuffer->GetSpecification().Height);

		RenderPassSettings settings;
		settings.ActiveCamera = &m_PreviewCamera;
		settings.CameraTransform = Math::Inverse(m_PreviewCamera.GetViewMatrix());
		settings.ScreenSpaceMode = ScreenSpaceRenderMode::None;

		m_PreviewScene->OnUpdateEdit(delta, settings);

		m_PreviewFramebuffer->Unbind();

		if (m_IsDirty)
		{
			m_SaveCooldown -= delta;
			if (m_SaveCooldown <= 0.0f)
				SaveParticleEffect(editor);
		}
	}

	void ParticleEffectViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		ImVec2 available = ImGui::GetContentRegionAvail();
		float propertiesWidth = 340.0f;
		float previewWidth = available.x - propertiesWidth;
		if (previewWidth < 200.0f)
			previewWidth = available.x * 0.5f;

		ImGui::BeginChild("ParticlePreviewPanel", ImVec2(previewWidth, 0), true);
		DrawPreviewPanel();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("ParticlePropertiesPanel", ImVec2(0, 0), true);
		DrawPropertiesPanel();
		ImGui::EndChild();
	}

	void ParticleEffectViewportViewer::DrawPreviewPanel()
	{
		if (ImGui::Button(m_IsPlaying ? "Pause" : "Play"))
			m_IsPlaying = !m_IsPlaying;

		ImGui::SameLine();
		if (ImGui::Button("Restart"))
			RestartPreview();

		ImGui::SameLine();
		ImGui::TextDisabled("Particles: %u", m_ActiveParticleCount);

		ImVec2 previewPanelSize = ImGui::GetContentRegionAvail();
		if (previewPanelSize.x > 0.0f && previewPanelSize.y > 0.0f &&
			(m_PreviewSize.x != previewPanelSize.x || m_PreviewSize.y != previewPanelSize.y))
		{
			m_PreviewSize = { previewPanelSize.x, previewPanelSize.y };
			m_PreviewFramebuffer->ViewportResize((uint32_t)previewPanelSize.x, (uint32_t)previewPanelSize.y);
			m_PreviewCamera.SetViewportSize((uint32_t)previewPanelSize.x, (uint32_t)previewPanelSize.y);

			if (m_PreviewScene)
				m_PreviewScene->OnViewportResize((uint32_t)previewPanelSize.x, (uint32_t)previewPanelSize.y);
		}

		uint32_t textureID = m_PreviewFramebuffer->GetColorAttachmentID(0);
		ImGui::Image(reinterpret_cast<void*>(static_cast<uintptr_t>(textureID)),
			ImVec2{ previewPanelSize.x, previewPanelSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		m_PreviewHovered = ImGui::IsItemHovered();
	}

	void ParticleEffectViewportViewer::DrawPropertiesPanel()
	{
		if (!m_Effect)
		{
			ImGui::TextDisabled("No particle effect loaded.");
			return;
		}

		ImGui::Text("%s", m_Effect->GetName().c_str());
		if (m_IsDirty)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(unsaved)");
		}

		ImGui::Separator();

		auto& assetManager = Application::Instance().GetAssetManager();
		if (UI::DrawParticleEffectProperties(m_Effect->Properties, assetManager, "ParticleEffectAsset"))
			MarkDirty();
	}

	void ParticleEffectViewportViewer::SaveParticleEffect(EditorLayer* editor)
	{
		if (!m_Effect)
			return;

		if (ParticleEffectSerializer::Serialize(GetFilePath(), m_Effect))
		{
			m_IsDirty = false;
			auto evt = UINotificationEvent("Particle Effect saved successfully", UINotificationEvent::Severity::Info);
			editor->GetContext().EventCallback(evt);
		}
		else
		{
			auto evt = UINotificationEvent("Failed to save Particle Effect", UINotificationEvent::Severity::Error);
			editor->GetContext().EventCallback(evt);
		}
	}

}
