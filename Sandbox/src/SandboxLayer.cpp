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
	EB_INFO("Layer {} attached!", GetName());
}

void SandboxLayer::OnDetatch()
{
	EB_INFO("Layer {} detatched!", GetName());
}

void SandboxLayer::OnUpdate(Ember::TimeStep delta)
{
	Ember::RenderAction::SetClearColor(Ember::Vector4f(0.0f, 1.0f, 0.0f, 1.0));
	Ember::RenderAction::Clear();

	EB_TRACE("Layer {} updated! Delta time: {} milliseconds", GetName(), delta.Milliseconds());
}

void SandboxLayer::OnImGuiRender(Ember::TimeStep delta)
{
	ImGui::ShowDemoWindow();
}
