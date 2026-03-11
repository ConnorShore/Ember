#pragma once

#include <Ember.h>

class SceneTestLayer : public Ember::Layer {
public:

	SceneTestLayer();
	virtual ~SceneTestLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:
	Ember::SharedPtr<Ember::Scene> m_MainScene;
	Ember::SceneEntity m_Entity;
	Ember::SceneEntity m_CameraEntity;
	std::vector<Ember::SceneEntity> m_SpriteEntities;
};