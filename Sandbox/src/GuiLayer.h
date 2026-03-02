#pragma once

#include <Ember.h>>

class GuiLayer : public Ember::Layer {
public:

	GuiLayer();
	virtual ~GuiLayer();

	void OnAttach() override;
	void OnDetatch() override;
	void OnUpdate() override;

private:

};