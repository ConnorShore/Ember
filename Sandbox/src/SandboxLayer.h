#pragma once

#include <Ember.h>

class SandboxLayer : public Ember::Layer {
public:

	SandboxLayer();
	virtual ~SandboxLayer();

	void OnAttach() override;
	void OnDetatch() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:

};