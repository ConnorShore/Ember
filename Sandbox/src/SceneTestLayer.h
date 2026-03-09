#pragma once

#include <Ember.h>

class SceneTestLayer : public Ember::Layer {
public:

	SceneTestLayer();
	virtual ~SceneTestLayer();

	void OnAttach() override;
	void OnDetatch() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:

};