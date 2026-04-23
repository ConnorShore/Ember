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
			return s_FloatDistribution(s_RandomEngine);
		}

	private:
		inline static std::mt19937 s_RandomEngine;
		inline static std::uniform_real_distribution<float> s_FloatDistribution;
	};
}