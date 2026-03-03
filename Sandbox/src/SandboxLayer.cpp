#include "SandboxLayer.h"

#include <imgui/imgui.h>

SandboxLayer::SandboxLayer()
	: Layer("Sandbox Layer")
{
}

SandboxLayer::~SandboxLayer()
{
}

void SandboxLayer::OnAttach()
{
	RegisterShader("assets/shaders/Basic.glsl");

	//float vertices[] = {
	//	-0.5f, -0.5f, 0.0f,
	//	 0.5f, -0.5f, 0.0f,
	//	 0.5f,  0.5f, 0.0f,
	//	-0.5f,  0.5f, 0.0f,
	//};

	//unsigned int[] indices = {
	//	0, 1, 2,
	//	2, 3, 0
	//};

	// 2 ways, 1 with vbo and ibo, 1 with combined buffer
	// 
	// separate buffers
	//m_VBO = VertexBuffer::Create(vertices, sizeof(vertices));
	//m_IBO = IndexBuffer::Create(indices, sizeof(indices));
	//m_VAO = VertexArray::Create();
	//m_VAO->AddVertexBuffer(m_VBO);
	//m_VAO->AddIndexBuffer(m_IBO);
	//m_VAO->SetLayout({
	//	{ Ember::Float3, "a_Position" },
	//	{ Ember::Float4, "a_Color" }
	//	});

	//// combined buffer
	//m_Buffer = CombinedBuffer::Create(vertices, sizeof(vertices), indices, sizeof(indices));
	//mVAO = VertexArray::Create();
	//mVAO->AddCombinedBuffer(m_Buffer);
}

void SandboxLayer::OnDetatch()
{
	EB_INFO("Layer {} detatched!", GetName());
}

void SandboxLayer::OnUpdate(Ember::TimeStep delta)
{
	Ember::RenderAction::SetClearColor(Ember::Vector4f(0.0f, 1.0f, 0.0f, 1.0));
	Ember::RenderAction::UseDepthTest(false);
	Ember::RenderAction::UseFaceCulling(false);
	Ember::RenderAction::Clear();

	GetShader("Basic")->Bind();
}

void SandboxLayer::OnImGuiRender(Ember::TimeStep delta)
{
	ImGui::ShowDemoWindow();
}
