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
	Ember::SharedPtr<Ember::VertexArray> m_vao;
	Ember::SharedPtr<Ember::VertexBufferBase> m_vbo;
	Ember::SharedPtr<Ember::IndexBuffer> m_ibo;
};