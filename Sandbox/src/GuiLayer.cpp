#include "GuiLayer.h"

GuiLayer::GuiLayer()
	: Layer("GUI Layer")
{
}

GuiLayer::~GuiLayer()
{
}

void GuiLayer::OnAttach()
{
	EB_INFO("Layer {} attached!", GetName());
}

void GuiLayer::OnDetatch()
{
	EB_INFO("Layer {} detatched!", GetName());
}

void GuiLayer::OnUpdate()
{
}
