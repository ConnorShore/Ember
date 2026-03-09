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
	Ember::SharedPtr<Ember::Scene> m_MainScene;

	Ember::SharedPtr<Ember::VertexArray> m_vao;
	Ember::SharedPtr<Ember::VertexBufferBase> m_vbo;
	Ember::SharedPtr<Ember::IndexBuffer> m_ibo;

	Ember::SharedPtr<Ember::SceneEntity> m_Entity;
	std::vector<Ember::SharedPtr<Ember::SceneEntity>> m_SpriteEntities;
};