// ANIMATION TESTS
// ---------------
// Covers the animation state machine's DATA layer: the blackboard, condition evaluation, states and
// transitions, and the controller's layer stack. That is the part that decides *which* animation
// plays; the sampling/skinning half needs real skeleton + clip assets and is exercised by the visual
// tests instead.
//
// Condition evaluation is worth the attention: it is a small switch over parameter types where a
// wrong comparison operator or a missing-parameter case produces an animation that simply never
// transitions - no error, no crash, just a character stuck in idle.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <string>
#include <unordered_map>

using namespace Ember;
using Ember::Test::Type::Unit;
using Ember::Test::Type::Integration;
using Ember::Test::MakeEntityAt;
using Ember::Test::SceneFixture;

namespace {

	AnimationCondition MakeCondition(const std::string& name, AnimationParameterType type,
		AnimationConditionOperator op)
	{
		AnimationCondition condition;
		condition.ParameterName = name;
		condition.Type = type;
		condition.Operator = op;
		return condition;
	}

} // namespace

//////////////////////////////////////////////////////////////////////////
// Blackboard
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Animation, BlackboardStoresTypedParameters, Unit)
{
	AnimationBlackboard blackboard;
	blackboard.SetFloat("Speed", 3.5f);
	blackboard.SetBool("IsGrounded", true);
	blackboard.SetInt("WeaponIndex", 2);

	EB_CHECK_EQ(blackboard.Parameters.size(), (size_t)3);

	const AnimationParameter& speed = blackboard.Parameters.at("Speed");
	EB_EXPECT_MSG(speed.Type == AnimationParameterType::Float, "Speed was not stored as a Float parameter");
	EB_EXPECT_NEAR(speed.FloatValue, 3.5f, 1e-5);

	const AnimationParameter& grounded = blackboard.Parameters.at("IsGrounded");
	EB_EXPECT_MSG(grounded.Type == AnimationParameterType::Bool, "IsGrounded was not stored as a Bool parameter");
	EB_EXPECT(grounded.BoolValue);

	const AnimationParameter& weapon = blackboard.Parameters.at("WeaponIndex");
	EB_EXPECT_MSG(weapon.Type == AnimationParameterType::Int, "WeaponIndex was not stored as an Int parameter");
	EB_EXPECT_EQ(weapon.IntValue, 2);
}

EB_TEST_CASE(Animation, BlackboardOverwritesRatherThanDuplicating, Unit)
{
	AnimationBlackboard blackboard;
	blackboard.SetFloat("Speed", 1.0f);
	blackboard.SetFloat("Speed", 9.0f);

	EB_EXPECT_EQ(blackboard.Parameters.size(), (size_t)1);
	EB_EXPECT_NEAR(blackboard.Parameters.at("Speed").FloatValue, 9.0f, 1e-5);

	// Re-typing a name replaces the whole parameter, so the previous type's payload must not
	// linger and be read by a stale condition.
	blackboard.SetBool("Speed", true);
	EB_EXPECT_MSG(blackboard.Parameters.at("Speed").Type == AnimationParameterType::Bool,
		"re-assigning a parameter with a different type left the old type in place");
	EB_EXPECT_NEAR(blackboard.Parameters.at("Speed").FloatValue, 0.0f, 1e-5);
}

//////////////////////////////////////////////////////////////////////////
// Condition evaluation
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Animation, FloatConditionOperators, Unit)
{
	AnimationBlackboard blackboard;
	blackboard.SetFloat("Speed", 5.0f);
	const auto& params = blackboard.Parameters;

	AnimationCondition greater = MakeCondition("Speed", AnimationParameterType::Float, AnimationConditionOperator::GreaterThan);
	greater.FloatValue = 3.0f;
	EB_EXPECT(AnimationConditionEvaluator::Evaluate(greater, params));

	greater.FloatValue = 7.0f;
	EB_EXPECT_FALSE(AnimationConditionEvaluator::Evaluate(greater, params));

	AnimationCondition less = MakeCondition("Speed", AnimationParameterType::Float, AnimationConditionOperator::LessThan);
	less.FloatValue = 7.0f;
	EB_EXPECT(AnimationConditionEvaluator::Evaluate(less, params));

	AnimationCondition equal = MakeCondition("Speed", AnimationParameterType::Float, AnimationConditionOperator::Equal);
	equal.FloatValue = 5.0f;
	EB_EXPECT(AnimationConditionEvaluator::Evaluate(equal, params));

	AnimationCondition notEqual = MakeCondition("Speed", AnimationParameterType::Float, AnimationConditionOperator::NotEqual);
	notEqual.FloatValue = 5.0f;
	EB_EXPECT_FALSE(AnimationConditionEvaluator::Evaluate(notEqual, params));

	// Boundaries: GreaterThan is strict, so an exactly-equal value must NOT satisfy it.
	AnimationCondition boundary = MakeCondition("Speed", AnimationParameterType::Float, AnimationConditionOperator::GreaterThan);
	boundary.FloatValue = 5.0f;
	EB_EXPECT_MSG(!AnimationConditionEvaluator::Evaluate(boundary, params),
		"GreaterThan should be strict, but an equal value satisfied it");
}

EB_TEST_CASE(Animation, BoolAndIntConditionOperators, Unit)
{
	AnimationBlackboard blackboard;
	blackboard.SetBool("IsGrounded", true);
	blackboard.SetInt("Weapon", 3);
	const auto& params = blackboard.Parameters;

	AnimationCondition boolEqual = MakeCondition("IsGrounded", AnimationParameterType::Bool, AnimationConditionOperator::Equal);
	boolEqual.BoolValue = true;
	EB_EXPECT(AnimationConditionEvaluator::Evaluate(boolEqual, params));

	boolEqual.BoolValue = false;
	EB_EXPECT_FALSE(AnimationConditionEvaluator::Evaluate(boolEqual, params));

	AnimationCondition boolNotEqual = MakeCondition("IsGrounded", AnimationParameterType::Bool, AnimationConditionOperator::NotEqual);
	boolNotEqual.BoolValue = false;
	EB_EXPECT(AnimationConditionEvaluator::Evaluate(boolNotEqual, params));

	// GreaterThan is meaningless for a bool and must fall through to false, not to a garbage compare.
	AnimationCondition boolGreater = MakeCondition("IsGrounded", AnimationParameterType::Bool, AnimationConditionOperator::GreaterThan);
	boolGreater.BoolValue = false;
	EB_EXPECT_FALSE(AnimationConditionEvaluator::Evaluate(boolGreater, params));

	AnimationCondition intGreater = MakeCondition("Weapon", AnimationParameterType::Int, AnimationConditionOperator::GreaterThan);
	intGreater.IntValue = 1;
	EB_EXPECT(AnimationConditionEvaluator::Evaluate(intGreater, params));

	AnimationCondition intEqual = MakeCondition("Weapon", AnimationParameterType::Int, AnimationConditionOperator::Equal);
	intEqual.IntValue = 3;
	EB_EXPECT(AnimationConditionEvaluator::Evaluate(intEqual, params));

	intEqual.IntValue = 4;
	EB_EXPECT_FALSE(AnimationConditionEvaluator::Evaluate(intEqual, params));
}

EB_TEST_CASE(Animation, MissingParameterEvaluatesFalse, Unit)
{
	// A condition naming a parameter that was never set must be FALSE, not "true by default" -
	// otherwise a typo in a parameter name makes a transition fire unconditionally.
	AnimationBlackboard blackboard;
	blackboard.SetFloat("Speed", 5.0f);

	AnimationCondition typo = MakeCondition("Sped", AnimationParameterType::Float, AnimationConditionOperator::GreaterThan);
	typo.FloatValue = 0.0f;
	EB_EXPECT_FALSE(AnimationConditionEvaluator::Evaluate(typo, blackboard.Parameters));

	// Same for an entirely empty blackboard.
	const std::unordered_map<std::string, AnimationParameter> empty;
	EB_EXPECT_FALSE(AnimationConditionEvaluator::Evaluate(typo, empty));
}

//////////////////////////////////////////////////////////////////////////
// State machine
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Animation, FirstStateBecomesTheDefault, Unit)
{
	// The default state is what plays on spawn. If the first CreateState did not claim it, every
	// character would start with no animation bound at all.
	AnimationStateMachine machine;
	EB_EXPECT_EQ(machine.GetDefaultState(), UUID(Constants::InvalidUUID));

	AnimationState& idle = machine.CreateState("Idle");
	const UUID idleId = idle.Id;
	EB_EXPECT_EQ(machine.GetDefaultState(), idleId);

	// A second state must NOT steal the default.
	machine.CreateState("Run");
	EB_EXPECT_EQ(machine.GetDefaultState(), idleId);
	EB_EXPECT_EQ(machine.GetStates().size(), (size_t)2);

	EB_EXPECT(machine.ContainsState("Idle"));
	EB_EXPECT(machine.ContainsState("Run"));
	EB_EXPECT_FALSE(machine.ContainsState("Jump"));
}

EB_TEST_CASE(Animation, TransitionsAreStoredPerSourceState, Unit)
{
	AnimationStateMachine machine;
	const UUID idleId = machine.CreateState("Idle").Id;
	const UUID runId = machine.CreateState("Run").Id;

	AnimationTransition& toRun = machine.CreateTransition(idleId, runId);
	toRun.BlendDuration = 0.4f;
	const UUID transitionId = toRun.Id;

	EB_CHECK_EQ(machine.GetTransitions().size(), (size_t)1);
	EB_CHECK_EQ(machine.GetTransitions().at(idleId).size(), (size_t)1);
	EB_EXPECT_EQ(machine.GetTransitions().at(idleId)[0].ToStateId, runId);
	EB_EXPECT_NEAR(machine.GetTransitions().at(idleId)[0].BlendDuration, 0.4f, 1e-5);

	AnimationTransition* found = machine.GetTransitionById(transitionId);
	EB_CHECK_MSG(found != nullptr, "GetTransitionById could not find a transition that was just created");
	EB_EXPECT_EQ(found->FromStateId, idleId);

	EB_EXPECT_MSG(machine.GetTransitionById(UUID(123456789)) == nullptr,
		"GetTransitionById returned something for an id that does not exist");
}

EB_TEST_CASE(Animation, RemovingAStateRemovesEveryTransitionTouchingIt, Unit)
{
	// A transition pointing at a deleted state is a dangling reference the runtime would follow
	// into a state that no longer exists. Both outgoing AND incoming edges must be cleaned up.
	AnimationStateMachine machine;
	const UUID idleId = machine.CreateState("Idle").Id;
	const UUID runId = machine.CreateState("Run").Id;
	const UUID jumpId = machine.CreateState("Jump").Id;

	machine.CreateTransition(idleId, runId);  // incoming to Run
	machine.CreateTransition(runId, jumpId);  // outgoing from Run
	machine.CreateTransition(idleId, jumpId); // unrelated to Run

	machine.RemoveState(runId);

	EB_EXPECT_EQ(machine.GetStates().size(), (size_t)2);
	EB_EXPECT_FALSE(machine.ContainsState("Run"));

	// Only Idle -> Jump should survive.
	int survivingTransitions = 0;
	for (const auto& [fromId, transitions] : machine.GetTransitions())
	{
		for (const AnimationTransition& transition : transitions)
		{
			++survivingTransitions;
			EB_EXPECT_MSG(transition.FromStateId != runId, "an outgoing transition from the removed state survived");
			EB_EXPECT_MSG(transition.ToStateId != runId, "an incoming transition to the removed state survived");
		}
	}
	EB_EXPECT_EQ(survivingTransitions, 1);
}

EB_TEST_CASE(Animation, RemoveTransitionByIdAndByEndpoints, Unit)
{
	AnimationStateMachine machine;
	const UUID idleId = machine.CreateState("Idle").Id;
	const UUID runId = machine.CreateState("Run").Id;
	const UUID jumpId = machine.CreateState("Jump").Id;

	const UUID toRunId = machine.CreateTransition(idleId, runId).Id;
	machine.CreateTransition(idleId, jumpId);

	machine.RemoveTransition(toRunId);
	EB_CHECK_EQ(machine.GetTransitions().at(idleId).size(), (size_t)1);
	EB_EXPECT_EQ(machine.GetTransitions().at(idleId)[0].ToStateId, jumpId);

	// Removing the last transition from a state must drop the (now empty) list entirely.
	machine.RemoveTransition(idleId, jumpId);
	EB_EXPECT_MSG(machine.GetTransitions().find(idleId) == machine.GetTransitions().end(),
		"an empty transition list was left behind after removing the last transition");
}

EB_TEST_CASE(Animation, StatesCarryTheirPlaybackSettings, Unit)
{
	AnimationStateMachine machine;
	AnimationState& run = machine.CreateState("Run");
	run.Looping = true;
	run.BasePlaybackSpeed = 1.5f;
	const UUID runId = run.Id;

	// CreateState returns a reference INTO the map, so edits through it must be visible on lookup.
	const AnimationState& stored = machine.GetStates().at(runId);
	EB_EXPECT(stored.Looping);
	EB_EXPECT_NEAR(stored.BasePlaybackSpeed, 1.5f, 1e-5);
	EB_EXPECT_EQ(stored.Name, std::string("Run"));
	EB_EXPECT_NE(stored.Id, UUID(Constants::InvalidUUID));
}

//////////////////////////////////////////////////////////////////////////
// Controller
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Animation, ControllerLayersAndParameters, Unit)
{
	AnimationController controller(UUID(), "EmberTest_Controller", "");

	// InitializeDefaultController must create exactly one base layer, and must be idempotent -
	// it runs on every load, and a second layer would double-evaluate every pose.
	controller.InitializeDefaultController();
	EB_CHECK_EQ(controller.GetLayers().size(), (size_t)1);
	EB_EXPECT_EQ(controller.GetLayer(0).Name, std::string("Base Layer"));

	controller.InitializeDefaultController();
	EB_EXPECT_MSG(controller.GetLayers().size() == 1, "InitializeDefaultController added a duplicate base layer");

	AnimationLayer& upperBody = controller.CreateLayer("UpperBody");
	upperBody.Weight = 0.5f;
	upperBody.Mode = AnimationLayerMode::Additive;
	upperBody.StateMachine.CreateState("Aim");

	EB_CHECK_EQ(controller.GetLayers().size(), (size_t)2);
	EB_EXPECT_NEAR(controller.GetLayer(1).Weight, 0.5f, 1e-5);
	EB_EXPECT_MSG(controller.GetLayer(1).Mode == AnimationLayerMode::Additive, "layer mode did not persist");
	EB_EXPECT(controller.GetLayer(1).StateMachine.ContainsState("Aim"));

	std::unordered_map<std::string, AnimationParameter> parameters;
	AnimationParameter speed;
	speed.Type = AnimationParameterType::Float;
	speed.FloatValue = 4.0f;
	parameters["Speed"] = speed;
	controller.SetParameters(parameters);

	EB_CHECK_EQ(controller.GetParameters().size(), (size_t)1);
	EB_EXPECT_NEAR(controller.GetParameters().at("Speed").FloatValue, 4.0f, 1e-5);
	EB_EXPECT_EQ(controller.GetType(), AssetType::AnimationController);
}

EB_TEST_CASE(Animation, AnimatorSeedsItsBlackboardFromTheController, Integration)
{
	// AnimatorComponent copies the controller's parameter DEFINITIONS into its per-instance
	// blackboard on runtime start. Without that, scripts calling SetFloat("Speed", ...) would be
	// writing parameters no transition is looking at.
	auto controller = Ember::Test::Assets().Create<AnimationController>(UUID(), "EmberTest_AnimatorController", "");
	EB_CHECK(controller != nullptr);

	std::unordered_map<std::string, AnimationParameter> parameters;
	AnimationParameter speed;
	speed.Type = AnimationParameterType::Float;
	speed.FloatValue = 2.0f;
	parameters["Speed"] = speed;

	AnimationParameter grounded;
	grounded.Type = AnimationParameterType::Bool;
	grounded.BoolValue = true;
	parameters["IsGrounded"] = grounded;

	controller->SetParameters(parameters);

	SceneFixture scene("AnimatorScene");
	Entity entity = MakeEntityAt(*scene, "Animated", Vector3f(0.0f));
	auto& animator = entity.AttachComponent<AnimatorComponent>();
	animator.ControllerHandle = controller->GetUUID();

	EB_CHECK_EQ(animator.Blackboard.Parameters.size(), (size_t)0);
	animator.InitializeBlackboardFromController();

	EB_CHECK_EQ(animator.Blackboard.Parameters.size(), (size_t)2);
	EB_EXPECT_NEAR(animator.Blackboard.Parameters.at("Speed").FloatValue, 2.0f, 1e-5);
	EB_EXPECT(animator.Blackboard.Parameters.at("IsGrounded").BoolValue);

	// Script-facing setters write into the same blackboard the transitions read.
	animator.SetFloat("Speed", 8.0f);
	EB_EXPECT_NEAR(animator.Blackboard.Parameters.at("Speed").FloatValue, 8.0f, 1e-5);

	// An animator with no controller must clear its blackboard rather than keep stale parameters.
	animator.ControllerHandle = Constants::InvalidUUID;
	animator.InitializeBlackboardFromController();
	EB_EXPECT_EQ(animator.Blackboard.Parameters.size(), (size_t)0);

	Ember::Test::Assets().RemoveAsset(controller->GetUUID());
}

EB_TEST_CASE(Animation, AnimatorBoneCachesAreSizedAndInitialised, Integration)
{
	// The bone matrix arrays are uploaded straight to the skinning shader. They must be
	// MaxBones long and identity-initialised, or an un-posed character renders as an exploded
	// mess of garbage transforms on its first frame.
	SceneFixture scene("AnimatorCacheScene");
	Entity entity = MakeEntityAt(*scene, "Animated", Vector3f(0.0f));
	auto& animator = entity.AttachComponent<AnimatorComponent>();

	EB_CHECK_EQ(animator.BoneMatrices.size(), (size_t)Constants::Renderer::MaxBones);
	EB_CHECK_EQ(animator.BonePoseMatrices.size(), (size_t)Constants::Renderer::MaxBones);

	EB_EXPECT_MAT4_NEAR(animator.BoneMatrices[0], Matrix4f(1.0f), 1e-6f);
	EB_EXPECT_MAT4_NEAR(animator.BoneMatrices[Constants::Renderer::MaxBones - 1], Matrix4f(1.0f), 1e-6f);
	EB_EXPECT_NEAR(animator.PlaybackSpeed, 1.0f, 1e-6);
}
