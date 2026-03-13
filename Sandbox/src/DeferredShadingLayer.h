#pragma once

#include <Ember.h>

class DeferredShadingLayer : public Ember::Layer
{
public:
	DeferredShadingLayer();
	virtual ~DeferredShadingLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:
	Ember::SharedPtr<Ember::Scene> m_MainScene;
	Ember::SharedPtr<Ember::Framebuffer> m_Framebuffer;
	Ember::Entity m_CameraEntity;
	Ember::Vector2f m_ViewportSize;

	// Interactive sphere controlled via ImGui
	Ember::Entity m_InteractiveSphere;
	Ember::SharedPtr<Ember::MaterialInstance> m_InteractiveInstance;
	float m_Albedo[3] = { 1.0f, 0.2f, 0.2f };
	float m_Metallic = 0.5f;
	float m_Roughness = 0.5f;
	float m_AO = 1.0f;
};