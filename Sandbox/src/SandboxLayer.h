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
	Ember::SharedPtr<Ember::Shader> m_Shader;

	unsigned int m_VAO, m_VBO, m_IBO;
};