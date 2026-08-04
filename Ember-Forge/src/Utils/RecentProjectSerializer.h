#pragma once

#include "RecentProject.h"

#include <vector>

namespace Ember {

	class RecentProjectSerializer
	{
	public:
		RecentProjectSerializer(std::vector<RecentProject>& recentProjects);
		~RecentProjectSerializer() = default;

		bool Serialize(const std::filesystem::path filePath);
		bool Deserialize(const std::filesystem::path filePath);

	private:
		std::vector<RecentProject>& m_RecentProjects;
	};

}