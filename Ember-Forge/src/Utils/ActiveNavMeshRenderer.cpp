#include "efpch.h"
#include "ActiveNavMeshRenderer.h"

#include <Ember/AI/NavMeshDebugDraw.h>
#include <Ember/Asset/NavigationMeshData.h>
#include <Ember/Core/Application.h>
#include <Ember/Scene/Scene.h>

#include <DetourDebugDraw.h>

namespace Ember {

	namespace {
		bool s_Enabled = true;
	}

	void ActiveNavMeshRenderer::Draw(Scene* scene, EntityID selectedEntity)
	{
		if (!s_Enabled || !scene)
			return;

		if (selectedEntity == (EntityID)Constants::Entities::InvalidEntityID)
			return;

		auto& registry = scene->GetRegistry();
		if (!registry.ContainsComponent<NavigationMeshComponent>(selectedEntity))
			return;

		auto& navMeshComponent = registry.GetComponent<NavigationMeshComponent>(selectedEntity);
		if (navMeshComponent.NavMeshDataHandle == Constants::InvalidUUID)
			return;

		auto& assetManager = Application::Instance().GetAssetManager();
		auto navMeshAsset = assetManager.GetAsset<NavigationMeshData>(navMeshComponent.NavMeshDataHandle);
		if (!navMeshAsset)
			return;

		dtNavMesh* navMesh = navMeshAsset->GetNavMesh();
		if (!navMesh)
		{
			if (!navMeshAsset->InitializeFromRawData())
				return;

			navMesh = navMeshAsset->GetNavMesh();
			if (!navMesh)
				return;
		}

		NavMeshDebugDraw navDraw;
		duDebugDrawNavMesh(&navDraw, *navMesh, 0);
	}

	void ActiveNavMeshRenderer::SetEnabled(bool enabled)
	{
		s_Enabled = enabled;
	}

	bool ActiveNavMeshRenderer::GetEnabled()
	{
		return s_Enabled;
	}

}
