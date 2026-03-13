#pragma once

#include <Ember.h>

class Test3DLayer : public Ember::Layer
{
public:
	Test3DLayer();
	virtual ~Test3DLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:
	Ember::SharedPtr<Ember::Scene> m_MainScene;
	Ember::Entity m_CameraEntity;

	// Interactive sphere controlled via ImGui
	Ember::Entity m_InteractiveSphere;
	Ember::SharedPtr<Ember::MaterialInstance> m_InteractiveInstance;
	float m_Albedo[3]  = { 1.0f, 0.2f, 0.2f };
	float m_Metallic   = 0.5f;
	float m_Roughness  = 0.5f;
	float m_AO         = 1.0f;
};