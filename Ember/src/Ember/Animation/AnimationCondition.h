#pragma once

#include "AnimationParameter.h"

#include <string>
#include <unordered_map>

namespace Ember {

	enum class AnimationConditionOperator
	{
		GreaterThan,
		LessThan,
		Equal,
		NotEqual
	};

	struct AnimationCondition
	{
		std::string ParameterName;
		AnimationParameterType Type;
		AnimationConditionOperator Operator;

		// Threshold values — only the one matching 'Type' is used.
		// (Kept as flat fields for simple binary serialization)
		float FloatValue = 0.0f;
		bool BoolValue = false;
		int32_t IntValue = 0;
	};

	class AnimationConditionEvaluator
	{
	public:
		inline static bool Evaluate(const AnimationCondition& condition, const std::unordered_map<std::string, AnimationParameter>& params)
		{
			auto it = params.find(condition.ParameterName);

			switch (condition.Type)
			{
			case AnimationParameterType::Float:
				if (it == params.end())
					return false;
				return EvaluateFloatCondition(it->second.FloatValue, condition.Operator, condition.FloatValue);
			case AnimationParameterType::Bool:
				if (it == params.end())
					return false;
				return EvaluateBoolCondition(it->second.BoolValue, condition.Operator, condition.BoolValue);
			case AnimationParameterType::Int:
				if (it == params.end())
					return false;
				return EvaluateIntCondition(it->second.IntValue, condition.Operator, condition.IntValue);
			case AnimationParameterType::Trigger:
				// For triggers, we would need additional logic to reset the trigger after it's consumed.
				if (it == params.end())
					return false;
				return EvaluateBoolCondition(it->second.BoolValue, condition.Operator, true);
			default:
				return false;
			}
		}

	private:
		inline static bool EvaluateFloatCondition(float paramValue, AnimationConditionOperator op, float conditionValue)
		{
			switch (op)
			{
			case AnimationConditionOperator::GreaterThan:
				return paramValue > conditionValue;
			case AnimationConditionOperator::LessThan:
				return paramValue < conditionValue;
			case AnimationConditionOperator::Equal:
				return paramValue == conditionValue;
			case AnimationConditionOperator::NotEqual:
				return paramValue != conditionValue;
			default:
				return false;
			}
		}

		inline static bool EvaluateBoolCondition(bool paramValue, AnimationConditionOperator op, bool conditionValue)
		{
			switch (op)
			{
			case AnimationConditionOperator::Equal:
				return paramValue == conditionValue;
			case AnimationConditionOperator::NotEqual:
				return paramValue != conditionValue;
			default:
				return false;
			}
		}

		inline static bool EvaluateIntCondition(int32_t paramValue, AnimationConditionOperator op, int32_t conditionValue)
		{
			switch (op)
			{
			case AnimationConditionOperator::GreaterThan:
				return paramValue > conditionValue;
			case AnimationConditionOperator::LessThan:
				return paramValue < conditionValue;
			case AnimationConditionOperator::Equal:
				return paramValue == conditionValue;
			case AnimationConditionOperator::NotEqual:
				return paramValue != conditionValue;
			default:
				return false;
			}
		}
	};
}