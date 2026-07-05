#pragma once

namespace Ember {

	struct NavigationMeshBakeSettings
	{
		float CellSize = 0.3f;
		float CellHeight = 0.2f;
		float AgentHeight = 2.0f;
		float AgentRadius = 0.5f;
		float AgentMaxClimb = 0.5f;
		float AgentMaxSlope = 45.0f;

		bool operator==(const NavigationMeshBakeSettings& other) const
		{
			return CellSize == other.CellSize &&
				CellHeight == other.CellHeight &&
				AgentHeight == other.AgentHeight &&
				AgentRadius == other.AgentRadius &&
				AgentMaxClimb == other.AgentMaxClimb &&
				AgentMaxSlope == other.AgentMaxSlope;
		}

		bool operator!=(const NavigationMeshBakeSettings& other) const
		{
			return !(*this == other);
		}
	};

}