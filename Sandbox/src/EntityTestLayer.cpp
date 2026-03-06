#include "EntityTestLayer.h"

#include <array>
#include <string>

struct TransformComponent
{
	float x, y, z;
};

struct MeshComponent
{
	unsigned int meshID;
};

struct TagComponent
{
	std::string tag;
};

EntityTestLayer::EntityTestLayer()
	: Layer("ECS Test Layer"), m_Registry(Ember::ScopedPtr<Ember::Registry>::Create())
{
}

EntityTestLayer::~EntityTestLayer()
{

}

void EntityTestLayer::OnAttach()
{
	std::array<Ember::Entity, 10> entities;
	for (int i = 0; i < 10; i++) {
		entities[i] = m_Registry->CreateEntity();
	}

	TransformComponent tComp1 = { 1,2,3 };
	TagComponent tag1 = { "Tag1" };
	m_Registry->AttachComponent<TransformComponent>(entities[3], tComp1);
	m_Registry->AttachComponent<TagComponent>(entities[3], tag1);

	TransformComponent tComp2 = { 4,5,6 };
	m_Registry->AttachComponent<TransformComponent>(entities[7], tComp1);

	TagComponent tag2 = { "Tag2" };
	m_Registry->AttachComponent<TagComponent>(entities[0], tag2);

	MeshComponent mesh = { 100 };
	m_Registry->AttachComponent<MeshComponent>(entities[0], mesh);
	m_Registry->AttachComponent<MeshComponent>(entities[2], mesh);
}

void EntityTestLayer::OnDetatch()
{

}

void EntityTestLayer::OnUpdate(Ember::TimeStep delta)
{

}

void EntityTestLayer::OnImGuiRender(Ember::TimeStep delta)
{

}
