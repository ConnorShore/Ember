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
};