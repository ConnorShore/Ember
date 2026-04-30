#pragma once

#include "Ember/Math/Math.h"

namespace Ember {

	struct AABB
	{
		Vector3f WorldMin;
		Vector3f WorldMax;
	};

	struct Plane {
		Vector3f Normal;
		float Distance;

		void Normalize() {
			float mag = std::sqrt(Normal.x * Normal.x + Normal.y * Normal.y + Normal.z * Normal.z);
			Normal.x /= mag; Normal.y /= mag; Normal.z /= mag;
			Distance /= mag;
		}
	};

	struct Frustum {
		Plane Planes[6];

		// Extracts the 6 planes directly from your ViewProjection Matrix!
		Frustum(const Matrix4f& vp) {
			// Left
			Planes[0].Normal.x = vp[0][3] + vp[0][0];
			Planes[0].Normal.y = vp[1][3] + vp[1][0];
			Planes[0].Normal.z = vp[2][3] + vp[2][0];
			Planes[0].Distance = vp[3][3] + vp[3][0];

			// Right
			Planes[1].Normal.x = vp[0][3] - vp[0][0];
			Planes[1].Normal.y = vp[1][3] - vp[1][0];
			Planes[1].Normal.z = vp[2][3] - vp[2][0];
			Planes[1].Distance = vp[3][3] - vp[3][0];

			// Bottom
			Planes[2].Normal.x = vp[0][3] + vp[0][1];
			Planes[2].Normal.y = vp[1][3] + vp[1][1];
			Planes[2].Normal.z = vp[2][3] + vp[2][1];
			Planes[2].Distance = vp[3][3] + vp[3][1];

			// Top
			Planes[3].Normal.x = vp[0][3] - vp[0][1];
			Planes[3].Normal.y = vp[1][3] - vp[1][1];
			Planes[3].Normal.z = vp[2][3] - vp[2][1];
			Planes[3].Distance = vp[3][3] - vp[3][1];

			// Near (OpenGL uses -1 to 1 depth, so we add the Z column)
			Planes[4].Normal.x = vp[0][3] + vp[0][2];
			Planes[4].Normal.y = vp[1][3] + vp[1][2];
			Planes[4].Normal.z = vp[2][3] + vp[2][2];
			Planes[4].Distance = vp[3][3] + vp[3][2];

			// Far
			Planes[5].Normal.x = vp[0][3] - vp[0][2];
			Planes[5].Normal.y = vp[1][3] - vp[1][2];
			Planes[5].Normal.z = vp[2][3] - vp[2][2];
			Planes[5].Distance = vp[3][3] - vp[3][2];

			for (int i = 0; i < 6; i++) {
				Planes[i].Normalize();
			}
		}

		bool IsBoxVisible(const Vector3f& minBounds, const Vector3f& maxBounds) const {
			for (int i = 0; i < 6; i++) {
				// Find the corner of the AABB that is furthest along the plane's normal direction
				Vector3f positiveVertex = minBounds;
				if (Planes[i].Normal.x >= 0) 
					positiveVertex.x = maxBounds.x;
				if (Planes[i].Normal.y >= 0)
					positiveVertex.y = maxBounds.y;
				if (Planes[i].Normal.z >= 0)
					positiveVertex.z = maxBounds.z;

				// If that furthest point is behind the plane, the ENTIRE box is behind the plane!
				if (Math::Dot(Planes[i].Normal, positiveVertex) + Planes[i].Distance < 0) {
					return false; // Culled!
				}
			}
			return true; // Visible!
		}
	};

	static void GetEntitiesInFrustum(const std::vector<std::pair<EntityID, AABB>>& entities, const Matrix4f& viewProjMatrix, std::vector<EntityID>& outEntities)
	{
		outEntities.reserve(entities.size());

		Frustum viewFrustum(viewProjMatrix);
		for (auto& [entityID, aabb] : entities)
		{
			if (viewFrustum.IsBoxVisible(aabb.WorldMin, aabb.WorldMax))
				outEntities.push_back(entityID);
		}
	}

}