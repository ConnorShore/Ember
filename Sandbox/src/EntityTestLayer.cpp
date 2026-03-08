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

	// Attach test
	TransformComponent tComp1 = { 1,2,3 };
	TagComponent tag1 = { "Tag1" };
	m_Registry->AttachComponent<TransformComponent>(entities[3], tComp1);
	m_Registry->AttachComponent<TagComponent>(entities[3], tag1);

	TransformComponent tComp2 = { 4,5,6 };
	m_Registry->AttachComponent<TransformComponent>(entities[7], tComp2);

	TagComponent tag2 = { "Tag2" };
	m_Registry->AttachComponent<TagComponent>(entities[0], tag2);

	MeshComponent mComp = { 100 };
	m_Registry->AttachComponent<MeshComponent>(entities[0], mComp);
	m_Registry->AttachComponent<MeshComponent>(entities[2], mComp);

	// Get/contains test
	bool containsComp = m_Registry->ContainsComponent<TransformComponent>(entities[7]);
	TransformComponent ret = m_Registry->GetComponent<TransformComponent>(entities[7]);

	bool containsMesh = m_Registry->ContainsComponent<MeshComponent>(entities[7]);
	if (containsMesh)
		MeshComponent ret2 = m_Registry->GetComponent<MeshComponent>(entities[7]);

	bool containsComps = m_Registry->ContainsComponents<TagComponent, MeshComponent>(entities[0]);
	if (containsComps)
	{
		auto [tag, mesh] = m_Registry->GetComponents<TagComponent, MeshComponent>(entities[0]);
		tag.tag = "New Tag";
		mesh.meshID = 1000;
	}

	// Get all entities with mesh and transform components
	auto view = m_Registry->Query<MeshComponent, TagComponent>();
	for (auto e : view)
	{
		EB_TRACE("Entity {} has both Mesh and Tag component", e);
	}

	// Detach test
	m_Registry->DetachComponent<TransformComponent>(entities[3]);
	m_Registry->DetachComponent<MeshComponent>(entities[3]);	// Nothing should be found and no error either (maybe log)

	for (int i = 0; i < 10; i++) {
		m_Registry->DestroyEntity(entities[i]);
	}

	EB_TRACE("Successfully destroyed all entities!");
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
