#pragma once

#include <Ember.h>

class EntityTestLayer : public Ember::Layer
{
public:
	EntityTestLayer();
	virtual ~EntityTestLayer();

	void OnAttach() override;
	void OnDetatch() override;
	void OnUpdate(Ember::TimeStep delta) override;
	void OnImGuiRender(Ember::TimeStep delta) override;

private:
	Ember::ScopedPtr<Ember::Registry> m_Registry;
};

