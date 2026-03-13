#pragma once

#include <Ember.h>

class SpriteTestLayer : public Ember::Layer {
public:

	SpriteTestLayer();
	virtual ~SpriteTestLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:
	Ember::SharedPtr<Ember::Scene> m_MainScene;
	Ember::Entity m_Entity;
	Ember::Entity m_CameraEntity;
	std::vector<Ember::Entity> m_SpriteEntities;
};