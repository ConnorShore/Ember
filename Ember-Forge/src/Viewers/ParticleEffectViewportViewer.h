#pragma once

#include "EditorViewportViewer.h"

#include <Ember/Asset/ParticleEffect.h>
#include <Ember/Render/Framebuffer.h>
#include <Ember/Tools/EditorCamera.h>

namespace Ember {

	class ParticleEffectViewportViewer final : public EditorViewportViewer
	{
	public:
		ParticleEffectViewportViewer(SharedPtr<ParticleEffect> effect, const std::string& filePath, const std::string& title);
		virtual ~ParticleEffectViewportViewer();

		virtual void OnUpdate(TimeStep delta, EditorLayer* editor) override;
		virtual void OnImGuiRender(EditorLayer* editor) override;

		void SaveParticleEffect(EditorLayer* editor);
		void MarkDirty();

		SharedPtr<ParticleEffect> GetEffect() const { return m_Effect; }

	private:
		void DrawPreviewPanel();
		void DrawPropertiesPanel();
		void RestartPreview();

	private:
		static constexpr float AUTO_SAVE_DELAY = 2.0f;

		SharedPtr<ParticleEffect> m_Effect;

		SharedPtr<Scene> m_PreviewScene;
		SharedPtr<Framebuffer> m_PreviewFramebuffer;
		EditorCamera m_PreviewCamera;

		Vector2f m_PreviewSize = { 0.0f, 0.0f };
		bool m_PreviewHovered = false;
		bool m_IsPlaying = true;
		uint32_t m_ActiveParticleCount = 0;

		bool m_IsDirty = false;
		float m_SaveCooldown = 0.0f;
	};
}
