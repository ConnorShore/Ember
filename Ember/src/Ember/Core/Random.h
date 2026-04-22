#pragma once

#include <random>

namespace Ember {
	class Random
	{
	public:
		inline static void Init()
		{
			std::random_device rd;
			s_RandomEngine.seed(rd());
		}

		inline static float Float()
		{
			return (float)s_FloatDistribution(s_RandomEngine) / (float)std::numeric_limits<uint32_t>::max();
		}

	private:
		inline static std::mt19937 s_RandomEngine;
		inline static std::uniform_real_distribution<float> s_FloatDistribution;
	};
}