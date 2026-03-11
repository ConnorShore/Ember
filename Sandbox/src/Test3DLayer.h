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
	Ember::Entity m_Entity;
	Ember::Entity m_CameraEntity;
};