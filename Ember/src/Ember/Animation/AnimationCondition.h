#pragma once

#include <string>
#include <unordered_map>

namespace Ember {

	enum class AnimationParameterType
	{
		Float,
		Bool,
		Trigger // Optional: A bool that resets to false once consumed
	};

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

		// We store both, but only read the one that matches 'Type'.
		// (This is much easier to serialize to YAML than a std::variant)
		float FloatValue = 0.0f;
		bool BoolValue = false;
	};

	class AnimationConditionEvaluator
	{
	public:
		inline static bool Evaluate(const AnimationCondition& condition, const std::unordered_map<std::string, float>& floatParams, const std::unordered_map<std::string, bool>& boolParams)
		{
			switch (condition.Type)
			{
			case AnimationParameterType::Float:
				if (!floatParams.contains(condition.ParameterName))
					return false; // Parameter not found
				return EvaluateFloatCondition(floatParams.at(condition.ParameterName), condition.Operator, condition.FloatValue);
			case AnimationParameterType::Bool:
				if (!boolParams.contains(condition.ParameterName))
					return false; // Parameter not found
				return EvaluateBoolCondition(boolParams.at(condition.ParameterName), condition.Operator, condition.BoolValue);
			case AnimationParameterType::Trigger:
				// For triggers, we would need additional logic to reset the trigger after it's consumed.
				// This is a simplified example and may require a more complex implementation in a real system.
				if (!boolParams.contains(condition.ParameterName))
					return false; // Parameter not found
				return EvaluateBoolCondition(boolParams.at(condition.ParameterName), condition.Operator, true);
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
	};
}