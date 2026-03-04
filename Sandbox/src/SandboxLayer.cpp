#include "SandboxLayer.h"

#include <imgui/imgui.h>

#include <Ember/Core/SharedPointer.h>

SandboxLayer::SandboxLayer()
	: Layer("Sandbox Layer")
{
}

SandboxLayer::~SandboxLayer()
{
}

void SandboxLayer::OnAttach()
{
	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		 0.5f,  0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
	};

	unsigned int indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	m_vbo = Ember::VertexBuffer<float>::Create(vertices);
	m_vbo->SetLayout({
		{ Ember::ShaderDataType::Float3, "v_Position", false }
		});

	m_ibo = Ember::IndexBuffer::Create(indices);
	m_vao = Ember::VertexArray::Create();

	m_vao->SetBuffer(m_vbo, m_ibo);

	RegisterShader("assets/shaders/Basic.glsl");
}

void SandboxLayer::OnDetatch()
{
	EB_INFO("Layer {} detatched!", GetName());
}

void SandboxLayer::OnUpdate(Ember::TimeStep delta)
{
	Ember::RenderAction::SetClearColor(Ember::Vector4f(0.0f, 0.0f, 0.0f, 1.0));
	Ember::RenderAction::UseDepthTest(false);
	Ember::RenderAction::UseFaceCulling(false);
	Ember::RenderAction::Clear();

	Ember::RenderAction::DrawInstanced(m_vao, GetShader("Basic"));
}

void SandboxLayer::OnImGuiRender(Ember::TimeStep delta)
{
	ImGui::ShowDemoWindow();
}
