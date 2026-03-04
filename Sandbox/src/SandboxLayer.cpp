#include "SandboxLayer.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <Ember/Core/SharedPointer.h>

SandboxLayer::SandboxLayer()
	: Layer("Sandbox Layer"), u_Color(0.2f, 0.3f, 0.8f, 1.0f)
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

	m_vbo = Ember::VertexBuffer<float>::Create(vertices, {
		{ Ember::ShaderDataType::Float3, "v_Position" }
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
	Ember::RenderAction::Clear();

	auto shader = GetShader("Basic");
	shader->Bind();
	shader->SetFloat4("u_Color", u_Color);

	Ember::RenderAction::DrawInstanced(m_vao, GetShader("Basic"));
}

void SandboxLayer::OnImGuiRender(Ember::TimeStep delta)
{
	ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

	static bool initializeDockspace = true;
	if (initializeDockspace)
	{
		initializeDockspace = false;

		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

		ImGuiID dockLeft, dockRight;
		ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Right, 0.2f, &dockRight, &dockLeft);

		ImGui::DockBuilderDockWindow("Settings", dockRight);
		ImGui::DockBuilderFinish(dockspaceId);
	}

	ImGui::Begin("Settings");
	ImGui::ColorPicker4("Color", &u_Color[0]);
	ImGui::End();
}
