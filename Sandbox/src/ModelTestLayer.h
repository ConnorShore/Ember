#pragma once

#include <Ember.h>

class ModelTestLayer : public Ember::Layer
{
public:
	ModelTestLayer();
	virtual ~ModelTestLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:
	void SetupDirectionalLights();
	void SetupStandardLights();
	void SetupRandomLights();

private:
	Ember::SharedPtr<Ember::Scene> m_MainScene;
	Ember::SharedPtr<Ember::Framebuffer> m_Framebuffer;
	Ember::Entity m_CameraEntity;
	Ember::Vector2f m_ViewportSize;

	Ember::SharedPtr<Ember::Material> m_DefaultLightCubeMaterial;
	Ember::SharedPtr<Ember::Material> m_DefaultMaterial;

	Ember::Entity m_Satellite;
};