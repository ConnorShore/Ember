#pragma once

#include <chrono>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Time Step
	//////////////////////////////////////////////////////////////////////////

	class TimeStep
	{
	public:
		TimeStep() : m_Time(0.0f) {}
		TimeStep(float time)
			: m_Time(time) {
		}

		float Seconds() const { return m_Time; }
		float Milliseconds() const { return m_Time * 1000.0f; }

		operator float() const { return m_Time; }
		float operator+=(const TimeStep& other) { m_Time += other.m_Time; return m_Time; }

	private:
		float m_Time;
	};

	//////////////////////////////////////////////////////////////////////////
	// Time Stamp
	//////////////////////////////////////////////////////////////////////////

	class TimeStamp
	{
	public:
		TimeStamp(const TimeStamp& other) = default;

		inline TimeStep operator-(const TimeStamp& other) const
		{
			std::chrono::duration<float> delta = m_TimeStamp - other.m_TimeStamp;
			return TimeStep(delta.count());
		}

	private:
		TimeStamp()
			:m_TimeStamp(std::chrono::steady_clock::now()) { }

	private:
		std::chrono::steady_clock::time_point m_TimeStamp;
		friend class Timer;
	};


	//////////////////////////////////////////////////////////////////////////
	// Timer
	//////////////////////////////////////////////////////////////////////////

	class Timer
	{
	public:
		Timer() = default;
		~Timer() = default;

		inline static TimeStamp Now()
		{
			return TimeStamp();
		}
	};
}