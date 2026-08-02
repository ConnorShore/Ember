// The animation state machine's data layer: blackboard, condition evaluation, states/transitions and
// controller layers. Sampling and skinning need real skeleton assets and are covered by the visual tests.

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

//////////////////////////////////////////////////////////////////////////
// Bone-driven entity transforms
//////////////////////////////////////////////////////////////////////////

namespace {

	// A two-bone rig laid out the way an imported model leaves one in the scene: an animator root, an
	// armature, a skinned mesh, and one entity per bone carrying only its bind-pose TRS. The bones
	// stack +Y so a pose change is unambiguous.
	struct RigFixture
	{
		SharedPtr<Skeleton> SkeletonAsset;
		Entity Root;
		Entity Armature;
		Entity Mesh;
		Entity RootBone;
		Entity ChildBone;

		// Bind pose: Root at origin, Child one unit above it.
		static constexpr float kChildBoneY = 1.0f;
	};

	RigFixture MakeRig(Scene& scene, const std::string& prefix)
	{
		RigFixture rig;

		std::vector<Bone> bones(2);
		bones[0].Name = prefix + "RootBone";
		bones[0].ParentID = static_cast<uint32_t>(-1);
		bones[0].LocalBindPoseTransform.Translation = Vector3f(0.0f);
		bones[0].LocalBindPoseTransform.Rotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
		bones[1].Name = prefix + "ChildBone";
		bones[1].ParentID = 0;
		bones[1].LocalBindPoseTransform.Translation = Vector3f(0.0f, RigFixture::kChildBoneY, 0.0f);
		bones[1].LocalBindPoseTransform.Rotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

		std::vector<Matrix4f> inverseBinds{ Matrix4f(1.0f), Math::Translate(Vector3f(0.0f, -RigFixture::kChildBoneY, 0.0f)) };
		rig.SkeletonAsset = Ember::Test::Assets().Create<Skeleton>(UUID(), prefix + "Skeleton", bones, inverseBinds);

		rig.Root = MakeEntityAt(scene, prefix + "Root", Vector3f(0.0f));
		auto& animator = rig.Root.AttachComponent<AnimatorComponent>();
		animator.SkeletonHandle = rig.SkeletonAsset->GetUUID();
		// Seed the bind pose the way AnimationSystem/InitializeAnimationPoseCaches would; the raw
		// component defaults to identity matrices, which is not any real skeleton's rest pose.
		animator.BonePoseMatrices[0] = Matrix4f(1.0f);
		animator.BonePoseMatrices[1] = Math::Translate(Vector3f(0.0f, RigFixture::kChildBoneY, 0.0f));

		rig.Armature = rig.Root.AddChild(prefix + "Armature");

		rig.Mesh = rig.Armature.AddChild(prefix + "Mesh");
		rig.Mesh.AttachComponent<SkinnedMeshComponent>(Constants::InvalidUUID, rig.Root.GetUUID());

		// Bone entities are siblings of the mesh, exactly as the glTF importer builds them.
		rig.RootBone = rig.Armature.AddChild(bones[0].Name);
		rig.ChildBone = rig.RootBone.AddChild(bones[1].Name);
		rig.ChildBone.GetComponent<TransformComponent>().Position = Vector3f(0.0f, RigFixture::kChildBoneY, 0.0f);

		return rig;
	}

	// One frame of the runtime order that matters here: transforms, then the bone pass.
	void TickBonePass(Scene& scene)
	{
		Ember::Test::Sys<TransformSystem>()->OnUpdate(Ember::Test::FixedStep(), &scene);
		Ember::Test::Sys<BoneSocketSystem>()->OnUpdate(Ember::Test::FixedStep(), &scene);
	}

} // namespace

EB_TEST_CASE(Animation, AnimatorWithoutAControllerIsSkipped, Integration)
{
	// Regression: AnimationSystem left `controller` null when ControllerHandle was unset and then
	// dereferenced it on the very next line, so a rig imported before its controller was authored
	// took the whole frame down. Nothing to assert but "we got here" - before the guard this test
	// crashed the process, and Logs/test-progress.log's last RUNNING line named it.
	SceneFixture scene("AnimatorNoControllerScene");
	RigFixture rig = MakeRig(*scene, "EmberTest_NoController_");

	auto& animator = rig.Root.GetComponent<AnimatorComponent>();
	EB_CHECK(animator.ControllerHandle == Constants::InvalidUUID);

	Ember::Test::Sys<AnimationSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	// The seeded bind pose must survive untouched - a skipped animator has no pose to write.
	EB_EXPECT_MAT4_NEAR(animator.BonePoseMatrices[1],
		Math::Translate(Vector3f(0.0f, RigFixture::kChildBoneY, 0.0f)), 1e-5f);

	Ember::Test::Assets().RemoveAsset(rig.SkeletonAsset->GetUUID());
}

EB_TEST_CASE(Animation, BoneEntitiesFollowTheAnimatedPose, Integration)
{
	// Regression: an imported model's bone entities only ever carried their bind-pose TRS, so
	// TransformSystem parked them at the T-pose no matter what the animator evaluated. Anything
	// parented to a bone - a hitbox, a held weapon - stayed behind with them.
	SceneFixture scene("BoneDrivenScene");
	RigFixture rig = MakeRig(*scene, "EmberTest_Follow_");

	Entity hitbox = rig.ChildBone.AddChild("Hitbox");
	hitbox.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 0.0f, 0.5f);

	TickBonePass(*scene);

	// Bind pose first: the pose-driven result must agree with plain hierarchy composition, or every
	// rig would visibly snap the moment this pass took over.
	EB_EXPECT_VEC3_NEAR(rig.ChildBone.GetComponent<TransformComponent>().GetWorldPosition(),
		Vector3f(0.0f, RigFixture::kChildBoneY, 0.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(hitbox.GetComponent<TransformComponent>().GetWorldPosition(),
		Vector3f(0.0f, RigFixture::kChildBoneY, 0.5f), 1e-4f);

	// Now bend the rig: the child bone swings 2 units along +X, as an animation would move it.
	auto& animator = rig.Root.GetComponent<AnimatorComponent>();
	animator.BonePoseMatrices[1] = Math::Translate(Vector3f(2.0f, RigFixture::kChildBoneY, 0.0f));

	TickBonePass(*scene);

	EB_EXPECT_VEC3_NEAR(rig.ChildBone.GetComponent<TransformComponent>().GetWorldPosition(),
		Vector3f(2.0f, RigFixture::kChildBoneY, 0.0f), 1e-4f);
	EB_EXPECT_MSG(Ember::Test::NearlyEqual(hitbox.GetComponent<TransformComponent>().GetWorldPosition(),
		Vector3f(2.0f, RigFixture::kChildBoneY, 0.5f), 1e-4f),
		"a child of a posed bone did not follow it - collider debug draw would sit at the T-pose");

	Ember::Test::Assets().RemoveAsset(rig.SkeletonAsset->GetUUID());
}

EB_TEST_CASE(Animation, BoneEntityPoseIsRelativeToTheSkinnedMesh, Integration)
{
	// Bone poses live in skeleton space, which the renderer lifts into world space with the skinned
	// mesh entity's transform. Bone entities must use the same basis or hitboxes drift off the model
	// as soon as the character moves or is scaled.
	SceneFixture scene("BoneDrivenBasisScene");
	RigFixture rig = MakeRig(*scene, "EmberTest_Basis_");

	rig.Root.GetComponent<TransformComponent>().Position = Vector3f(5.0f, 0.0f, -3.0f);
	rig.Armature.GetComponent<TransformComponent>().Scale = Vector3f(2.0f);

	auto& animator = rig.Root.GetComponent<AnimatorComponent>();
	animator.BonePoseMatrices[1] = Math::Translate(Vector3f(0.0f, 3.0f, 0.0f));

	TickBonePass(*scene);

	// Skeleton-space (0, 3, 0), scaled by 2 and offset by the root's position.
	EB_EXPECT_VEC3_NEAR(rig.ChildBone.GetComponent<TransformComponent>().GetWorldPosition(),
		Vector3f(5.0f, 6.0f, -3.0f), 1e-4f);

	Ember::Test::Assets().RemoveAsset(rig.SkeletonAsset->GetUUID());
}

EB_TEST_CASE(Animation, NonBoneEntitiesKeepTheirHierarchyTransform, Integration)
{
	// Bones are matched to entities by name, so the pass must leave every other entity in the rig
	// alone - the animator root that anchors the character, the skinned mesh whose transform is the
	// skinning basis this pass reads, and props hung off the armature rather than off a bone.
	SceneFixture scene("BoneDrivenScopeScene");
	RigFixture rig = MakeRig(*scene, "EmberTest_Scope_");

	Entity prop = rig.Armature.AddChild("EmberTest_Scope_Prop");
	prop.GetComponent<TransformComponent>().Position = Vector3f(0.0f, 0.0f, 4.0f);

	auto& animator = rig.Root.GetComponent<AnimatorComponent>();
	animator.BonePoseMatrices[0] = Math::Translate(Vector3f(9.0f, 9.0f, 9.0f));
	animator.BonePoseMatrices[1] = Math::Translate(Vector3f(9.0f, 9.0f, 9.0f));

	TickBonePass(*scene);

	EB_EXPECT_VEC3_NEAR(prop.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(0.0f, 0.0f, 4.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(rig.Mesh.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(0.0f), 1e-4f);
	EB_EXPECT_VEC3_NEAR(rig.Root.GetComponent<TransformComponent>().GetWorldPosition(), Vector3f(0.0f), 1e-4f);

	Ember::Test::Assets().RemoveAsset(rig.SkeletonAsset->GetUUID());
}
