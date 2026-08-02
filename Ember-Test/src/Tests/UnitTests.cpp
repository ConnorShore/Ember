// Pure logic - no scene, no systems, no GL. Fast, deterministic, and the layer everything else in
// the engine leans on.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Ember;
using Ember::Test::Type::Unit;

//////////////////////////////////////////////////////////////////////////
// Math - scalars
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Math, LerpEndpointsAndMidpoint, Unit)
{
	EB_EXPECT_NEAR(Math::Lerp(0.0f, 10.0f, 0.0f), 0.0f, 1e-5);
	EB_EXPECT_NEAR(Math::Lerp(0.0f, 10.0f, 0.5f), 5.0f, 1e-5);
	EB_EXPECT_NEAR(Math::Lerp(0.0f, 10.0f, 1.0f), 10.0f, 1e-5);
	EB_EXPECT_NEAR(Math::Lerp(-4.0f, 4.0f, 0.25f), -2.0f, 1e-5);

	// Lerp is unclamped by design (particle/animation code relies on overshoot for easing).
	EB_EXPECT_NEAR(Math::Lerp(0.0f, 10.0f, 2.0f), 20.0f, 1e-5);
	EB_EXPECT_NEAR(Math::Lerp(0.0f, 10.0f, -1.0f), -10.0f, 1e-5);
}

EB_TEST_CASE(Math, LerpOnVectors, Unit)
{
	const Vector3f from(0.0f, 0.0f, 0.0f);
	const Vector3f to(10.0f, -20.0f, 4.0f);
	EB_EXPECT_VEC3_NEAR(Math::Lerp(from, to, 0.5f), Vector3f(5.0f, -10.0f, 2.0f), 1e-5f);
	EB_EXPECT_VEC3_NEAR(Math::Mix(from, to, 0.25f), Vector3f(2.5f, -5.0f, 1.0f), 1e-5f);
}

EB_TEST_CASE(Math, ClampMinMax, Unit)
{
	EB_EXPECT_NEAR(Math::Clamp(5.0f, 0.0f, 1.0f), 1.0f, 1e-5);
	EB_EXPECT_NEAR(Math::Clamp(-5.0f, 0.0f, 1.0f), 0.0f, 1e-5);
	EB_EXPECT_NEAR(Math::Clamp(0.5f, 0.0f, 1.0f), 0.5f, 1e-5);

	EB_EXPECT_EQ(Math::Max(3, 7), 7);
	EB_EXPECT_EQ(Math::Min(3, 7), 3);
	EB_EXPECT_VEC3_NEAR(Math::Max(Vector3f(1.0f, 5.0f, -2.0f), Vector3f(3.0f, 2.0f, 0.0f)), Vector3f(3.0f, 5.0f, 0.0f), 1e-5f);
	EB_EXPECT_VEC3_NEAR(Math::Min(Vector3f(1.0f, 5.0f, -2.0f), Vector3f(3.0f, 2.0f, 0.0f)), Vector3f(1.0f, 2.0f, -2.0f), 1e-5f);
}

EB_TEST_CASE(Math, RadiansDegreesRoundTrip, Unit)
{
	EB_EXPECT_NEAR(Math::Degrees(Math::Radians(137.0f)), 137.0f, 1e-3);
	EB_EXPECT_NEAR(Math::Radians(180.0f), 3.14159265f, 1e-4);
	EB_EXPECT_NEAR(Math::Degrees(3.14159265f), 180.0f, 1e-3);
}

EB_TEST_CASE(Math, TrigIdentities, Unit)
{
	const float angle = Math::Radians(37.0f);
	// sin^2 + cos^2 == 1 for any angle; a cheap canary for a broken trig wrapper.
	EB_EXPECT_NEAR(Math::Sin(angle) * Math::Sin(angle) + Math::Cos(angle) * Math::Cos(angle), 1.0f, 1e-5);

	EB_EXPECT_NEAR(Math::Asin(Math::Sin(angle)), angle, 1e-4);
	EB_EXPECT_NEAR(Math::Acos(Math::Cos(angle)), angle, 1e-4);

	// Atan2 must be the two-argument form (quadrant aware), not atan(y/x).
	EB_EXPECT_NEAR(Math::Atan2(1.0f, 1.0f), Math::Radians(45.0f), 1e-4);
	EB_EXPECT_NEAR(Math::Atan2(1.0f, -1.0f), Math::Radians(135.0f), 1e-4);
	EB_EXPECT_NEAR(Math::Atan2(-1.0f, -1.0f), Math::Radians(-135.0f), 1e-4);
}

//////////////////////////////////////////////////////////////////////////
// Math - vectors
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Math, NormalizeUnitLength, Unit)
{
	const Vector3f n = Math::Normalize(Vector3f(3.0f, 0.0f, 4.0f));
	EB_EXPECT_NEAR(Math::Length(n), 1.0f, 1e-5);
	EB_EXPECT_NEAR(n.x, 0.6f, 1e-5);
	EB_EXPECT_NEAR(n.z, 0.8f, 1e-5);
}

EB_TEST_CASE(Math, DotAndCross, Unit)
{
	const Vector3f right(1.0f, 0.0f, 0.0f);
	const Vector3f up(0.0f, 1.0f, 0.0f);

	EB_EXPECT_NEAR(Math::Dot(right, up), 0.0f, 1e-6);
	EB_EXPECT_NEAR(Math::Dot(right, right), 1.0f, 1e-6);
	EB_EXPECT_NEAR(Math::Dot(right, -right), -1.0f, 1e-6);

	// Right-handed basis: X cross Y == Z.
	EB_EXPECT_VEC3_NEAR(Math::Cross(right, up), Vector3f(0.0f, 0.0f, 1.0f), 1e-6f);
	EB_EXPECT_VEC3_NEAR(Math::Cross(up, right), Vector3f(0.0f, 0.0f, -1.0f), 1e-6f);
}

EB_TEST_CASE(Math, DistanceAndMagnitude, Unit)
{
	const Vector3f a(1.0f, 2.0f, 3.0f);
	const Vector3f b(4.0f, 6.0f, 3.0f);

	EB_EXPECT_NEAR(Math::Distance(a, b), 5.0f, 1e-5);
	// Distance2 is the SQUARED distance - the whole point is skipping the sqrt in hot loops.
	EB_EXPECT_NEAR(Math::Distance2(a, b), 25.0f, 1e-4);
	EB_EXPECT_NEAR(Math::Magnitude(Vector3f(0.0f, 3.0f, 4.0f)), 5.0f, 1e-5);
	EB_EXPECT_NEAR(Math::Magnitude2(Vector3f(0.0f, 3.0f, 4.0f)), 25.0f, 1e-4);
}

EB_TEST_CASE(Math, ProjectOnPlane, Unit)
{
	const Vector3f planeNormal(0.0f, 1.0f, 0.0f);

	// Flattening a movement vector onto the ground plane - the character controller does this
	// every frame, so a sign error here turns into "player sinks through slopes".
	const Vector3f projected = Math::ProjectOnPlane(Vector3f(1.0f, 5.0f, 2.0f), planeNormal);
	EB_EXPECT_VEC3_NEAR(projected, Vector3f(1.0f, 0.0f, 2.0f), 1e-5f);

	// A vector already in the plane is unchanged; a vector along the normal collapses to zero.
	EB_EXPECT_VEC3_NEAR(Math::ProjectOnPlane(Vector3f(1.0f, 0.0f, 0.0f), planeNormal), Vector3f(1.0f, 0.0f, 0.0f), 1e-5f);
	EB_EXPECT_VEC3_NEAR(Math::ProjectOnPlane(planeNormal, planeNormal), Vector3f(0.0f), 1e-5f);
}

//////////////////////////////////////////////////////////////////////////
// Math - rotations
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Math, QuaternionEulerRoundTrip, Unit)
{
	const Vector3f euler(Math::Radians(30.0f), Math::Radians(45.0f), Math::Radians(-15.0f));
	const Quaternion quat = Math::ToQuaternion(euler);
	const Vector3f recovered = Math::ToEulerAngles(quat);

	// Compare as ORIENTATIONS - the same rotation has several valid Euler triples.
	EB_EXPECT_EULER_NEAR(recovered, euler, 1e-4f);

	// A rotation quaternion must be unit length, or every rotation built from it also scales.
	const float quatLength = std::sqrt(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w);
	EB_EXPECT_NEAR(quatLength, 1.0f, 1e-5);
}

EB_TEST_CASE(Math, AngleAxisRotatesVector, Unit)
{
	// 90 degrees about +Y takes +X to -Z in a right-handed system.
	const Quaternion rotation = Math::AngleAxis(Math::Radians(90.0f), Vector3f(0.0f, 1.0f, 0.0f));
	const Vector3f rotated = Math::Rotate(rotation, Vector3f(1.0f, 0.0f, 0.0f));
	EB_EXPECT_VEC3_NEAR(rotated, Vector3f(0.0f, 0.0f, -1.0f), 1e-5f);
}

EB_TEST_CASE(Math, SlerpEndpointsAndMidpoint, Unit)
{
	const Quaternion from = Math::AngleAxis(0.0f, Vector3f(0.0f, 1.0f, 0.0f));
	const Quaternion to = Math::AngleAxis(Math::Radians(90.0f), Vector3f(0.0f, 1.0f, 0.0f));

	EB_EXPECT_ROTATION_NEAR(Math::Slerp(from, to, 0.0f), from, 1e-5f);
	EB_EXPECT_ROTATION_NEAR(Math::Slerp(from, to, 1.0f), to, 1e-5f);

	// Halfway between 0 and 90 degrees about the same axis is 45 degrees.
	const Quaternion expectedMid = Math::AngleAxis(Math::Radians(45.0f), Vector3f(0.0f, 1.0f, 0.0f));
	EB_EXPECT_ROTATION_NEAR(Math::Slerp(from, to, 0.5f), expectedMid, 1e-4f);
}

EB_TEST_CASE(Math, LookAtProducesForwardDirection, Unit)
{
	// The two-argument LookAt returns Euler angles that orient -Z toward the target.
	const Vector3f eulers = Math::LookAt(Vector3f(0.0f), Vector3f(0.0f, 0.0f, -10.0f));
	const Matrix4f rotation = Math::GetRotationMatrix(eulers);
	const Vector3f forward = Math::Normalize(Vector3f(-rotation[2][0], -rotation[2][1], -rotation[2][2]));

	EB_EXPECT_VEC3_NEAR(forward, Vector3f(0.0f, 0.0f, -1.0f), 1e-4f);
}

//////////////////////////////////////////////////////////////////////////
// Math - matrices
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Math, DecomposeTransformRoundTrip, Unit)
{
	// Build a TRS matrix, decompose it, and confirm we recover translation, rotation AND scale.
	// This is the exact path the editor gizmos, reparenting and transform serialization depend on.
	const Vector3f translation(2.0f, -3.0f, 5.0f);
	const Vector3f eulerRadians(Math::Radians(30.0f), Math::Radians(45.0f), 0.0f);
	const Vector3f scale(1.5f, 2.0f, 0.5f);

	const Matrix4f m = Math::Translate(translation) * Math::GetRotationMatrix(eulerRadians) * Math::Scale(scale);

	Vector3f outT, outR, outS;
	const bool ok = Math::DecomposeTransform(m, outT, outR, outS);
	EB_CHECK(ok);

	EB_EXPECT_VEC3_NEAR(outT, translation, 1e-4f);
	EB_EXPECT_VEC3_NEAR(outS, scale, 1e-4f);
	EB_EXPECT_EULER_NEAR(outR, eulerRadians, 1e-4f);

	// Recomposing must reproduce the original matrix.
	const Matrix4f recomposed = Math::Translate(outT) * Math::GetRotationMatrix(outR) * Math::Scale(outS);
	EB_EXPECT_MAT4_NEAR(recomposed, m, 1e-3f);
}

EB_TEST_CASE(Math, DecomposeTransformIdentity, Unit)
{
	Vector3f outT, outR, outS;
	EB_CHECK(Math::DecomposeTransform(Matrix4f(1.0f), outT, outR, outS));

	EB_EXPECT_VEC3_NEAR(outT, Vector3f(0.0f), 1e-6f);
	EB_EXPECT_VEC3_NEAR(outS, Vector3f(1.0f), 1e-6f);
	EB_EXPECT_EULER_NEAR(outR, Vector3f(0.0f), 1e-6f);
}

EB_TEST_CASE(Math, DecomposeTransformRejectsNonAffine, Unit)
{
	// A projection matrix is not affine (the bottom row is not 0,0,0,1). Decompose must say so
	// rather than returning nonsense that silently teleports an entity.
	const Matrix4f projection = Math::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
	EB_EXPECT_FALSE(MathDetail::IsAffineTransform(projection));

	Vector3f outT, outR, outS;
	EB_EXPECT_FALSE(Math::DecomposeTransform(projection, outT, outR, outS));

	// ...while a plain TRS matrix must be accepted.
	const Matrix4f trs = Math::Translate(Vector3f(1.0f, 2.0f, 3.0f)) * Math::Scale(Vector3f(2.0f));
	EB_EXPECT(MathDetail::IsAffineTransform(trs));
}

EB_TEST_CASE(Math, DecomposeHandlesNegativeScale, Unit)
{
	// Mirrored transforms (negative scale on one axis) must still yield a PROPER rotation,
	// otherwise normals flip and the mesh renders inside-out.
	const Vector3f scale(-1.0f, 1.0f, 1.0f);
	const Matrix4f m = Math::Translate(Vector3f(1.0f, 0.0f, 0.0f)) * Math::Scale(scale);

	Vector3f outT, outR, outS;
	EB_CHECK(Math::DecomposeTransform(m, outT, outR, outS));

	// Exactly one axis is negated, so the product of the scale components stays negative.
	EB_EXPECT_LT(outS.x * outS.y * outS.z, 0.0f);
	EB_EXPECT_VEC3_NEAR(outT, Vector3f(1.0f, 0.0f, 0.0f), 1e-4f);
}

EB_TEST_CASE(Math, InverseUndoesTransform, Unit)
{
	const Matrix4f m = Math::Translate(Vector3f(3.0f, -1.0f, 2.0f))
		* Math::GetRotationMatrix(Vector3f(Math::Radians(20.0f), Math::Radians(70.0f), Math::Radians(10.0f)))
		* Math::Scale(Vector3f(2.0f, 2.0f, 2.0f));

	EB_EXPECT_MAT4_NEAR(m * Math::Inverse(m), Matrix4f(1.0f), 1e-4f);

	// Round-tripping a point through the matrix and its inverse must return the original point.
	const Vector3f point(1.0f, 2.0f, 3.0f);
	const Vector3f there = m * point;
	const Vector3f back = Math::Inverse(m) * there;
	EB_EXPECT_VEC3_NEAR(back, point, 1e-3f);
}

EB_TEST_CASE(Math, TranslateScaleCompose, Unit)
{
	// Scale-then-translate: the translation must NOT be scaled.
	const Matrix4f m = Math::Translate(Vector3f(10.0f, 0.0f, 0.0f)) * Math::Scale(Vector3f(2.0f));
	const Vector3f transformed = m * Vector3f(1.0f, 0.0f, 0.0f);
	EB_EXPECT_VEC3_NEAR(transformed, Vector3f(12.0f, 0.0f, 0.0f), 1e-5f);
}

EB_TEST_CASE(Math, PerspectiveAndOrthographicAreSane, Unit)
{
	const Matrix4f perspective = Math::Perspective(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
	// A perspective projection carries the perspective divide in [2][3]; an orthographic one does not.
	EB_EXPECT_NEAR(perspective[2][3], -1.0f, 1e-5);
	EB_EXPECT_NEAR(perspective[3][3], 0.0f, 1e-5);

	const Matrix4f ortho = Math::Orthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	EB_EXPECT_NEAR(ortho[3][3], 1.0f, 1e-5);
	EB_EXPECT_NEAR(ortho[2][3], 0.0f, 1e-5);
}

//////////////////////////////////////////////////////////////////////////
// Core - UUID
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Uuid, DistinctAndStable, Unit)
{
	const UUID a; // freshly generated
	const UUID b; // freshly generated
	EB_EXPECT_NE(a, b); // collision is astronomically unlikely

	const UUID fixed(42);
	EB_EXPECT_EQ(fixed, UUID(42)); // constructing from the same value is stable
	EB_EXPECT_EQ((uint64_t)fixed, 42ull);
	EB_EXPECT_NE(fixed, UUID(43));
}

EB_TEST_CASE(Uuid, NoCollisionsInBulk, Unit)
{
	// Generation is the identity backbone for every asset and entity - a duplicate silently
	// aliases two objects. 10k samples won't prove uniqueness, but it catches a broken generator
	// (constant seed, truncated range, non-reseeded thread-local) immediately.
	constexpr int kCount = 10000;
	std::unordered_set<uint64_t> seen;
	seen.reserve(kCount);

	for (int i = 0; i < kCount; ++i)
		seen.insert((uint64_t)UUID());

	EB_EXPECT_EQ(seen.size(), (size_t)kCount);
	// A generator that never returns 0 keeps InvalidUUID unambiguous.
	EB_EXPECT_EQ(seen.count(Constants::InvalidUUID), (size_t)0);
}

EB_TEST_CASE(Uuid, UsableAsMapKey, Unit)
{
	std::unordered_map<UUID, int> map;
	const UUID key(1234);
	map[key] = 7;

	EB_EXPECT_EQ(map.at(UUID(1234)), 7);
	EB_EXPECT_EQ(map.count(UUID(4321)), (size_t)0);
}

//////////////////////////////////////////////////////////////////////////
// Core - TimeStep
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Core, TimeStepConversions, Unit)
{
	const TimeStep step(0.016f);
	EB_EXPECT_NEAR(step.Seconds(), 0.016f, 1e-6);
	EB_EXPECT_NEAR(step.Milliseconds(), 16.0f, 1e-3);
	EB_EXPECT_NEAR((float)step, 0.016f, 1e-6);

	TimeStep accumulator(0.0f);
	accumulator += TimeStep(0.5f);
	accumulator += TimeStep(0.25f);
	EB_EXPECT_NEAR(accumulator.Seconds(), 0.75f, 1e-6);

	// Default construction must be zero, not garbage.
	EB_EXPECT_NEAR(TimeStep().Seconds(), 0.0f, 1e-9);
}

//////////////////////////////////////////////////////////////////////////
// Core - collision / render filters
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Core, FilterPresets, Unit)
{
	EB_EXPECT_EQ(FilterPreset::None, (Filter)0);
	EB_EXPECT_EQ(FilterPreset::Default, (Filter)1);
	// rp3d uses a 16-bit mask, so All must light every bit of that width.
	EB_EXPECT_EQ(FilterPreset::All, (Filter)0xFFFF);
	EB_EXPECT_EQ((Filter)(FilterPreset::Default & FilterPreset::All), FilterPreset::Default);
	EB_EXPECT_EQ((Filter)(FilterPreset::Default & FilterPreset::None), FilterPreset::None);
}

EB_TEST_CASE(Core, FilterManagerSlotsAndMasks, Unit)
{
	FilterManager filters;
	std::array<std::string, FilterManager::MaxSlots> slots{};
	slots[0] = "Default";
	slots[1] = "Player";
	slots[2] = "Enemy";
	filters.InitWithFilters(slots);

	// Slot i maps to bit (1 << i).
	EB_EXPECT_EQ(filters.GetFilter("Default"), (Filter)(1 << 0));
	EB_EXPECT_EQ(filters.GetFilter("Player"), (Filter)(1 << 1));
	EB_EXPECT_EQ(filters.GetFilter("Enemy"), (Filter)(1 << 2));

	// Unknown and empty names resolve to 0 rather than accidentally matching slot 0.
	EB_EXPECT_EQ(filters.GetFilter("DoesNotExist"), (Filter)0);
	EB_EXPECT_EQ(filters.GetFilter(""), (Filter)0);

	EB_EXPECT_EQ(filters.GetFilterName((Filter)(1 << 1)), std::string("Player"));

	// Only the named slots are reported, not all 16.
	EB_EXPECT_EQ(filters.GetFilters().size(), (size_t)3);

	// A mask with two bits set lists both names.
	const Filter combined = (Filter)((1 << 1) | (1 << 2));
	const std::vector<std::string> active = filters.GetActiveFilters(combined);
	EB_CHECK_EQ(active.size(), (size_t)2);
	EB_EXPECT_EQ(active[0], std::string("Player"));
	EB_EXPECT_EQ(active[1], std::string("Enemy"));

	// Out-of-range slot access must be bounds-checked, not UB.
	EB_EXPECT_EQ(filters.GetFilterNameBySlot(FilterManager::MaxSlots + 5), std::string(""));
}

//////////////////////////////////////////////////////////////////////////
// Core - reference-counted pointers
//////////////////////////////////////////////////////////////////////////

namespace {

	// Minimal SharedResource subclass that reports its own destruction, so the tests below can
	// assert on lifetime rather than just on ref counts.
	struct RefProbe : public SharedResource
	{
		explicit RefProbe(int* destructionCounter) : Counter(destructionCounter) {}
		~RefProbe() override { if (Counter) ++(*Counter); }
		int* Counter = nullptr;
		int Payload = 0;
	};

	struct DerivedProbe : public RefProbe
	{
		explicit DerivedProbe(int* destructionCounter) : RefProbe(destructionCounter) {}
		int Extra = 5;
	};

} // namespace

EB_TEST_CASE(Core, SharedPtrRefCounting, Unit)
{
	int destroyed = 0;
	{
		SharedPtr<RefProbe> first = SharedPtr<RefProbe>::Create(&destroyed);
		EB_EXPECT_EQ(first->GetRefCount(), (size_t)1);

		{
			SharedPtr<RefProbe> second = first;
			EB_EXPECT_EQ(first->GetRefCount(), (size_t)2);
			second->Payload = 99;
		}

		// Second went out of scope; the object must still be alive and mutated.
		EB_EXPECT_EQ(first->GetRefCount(), (size_t)1);
		EB_EXPECT_EQ(first->Payload, 99);
		EB_EXPECT_EQ(destroyed, 0);
	}
	// Last reference released - exactly one destruction, no double-free.
	EB_EXPECT_EQ(destroyed, 1);
}

EB_TEST_CASE(Core, SharedPtrMoveDoesNotBumpCount, Unit)
{
	int destroyed = 0;
	{
		SharedPtr<RefProbe> source = SharedPtr<RefProbe>::Create(&destroyed);
		SharedPtr<RefProbe> moved = std::move(source);

		EB_EXPECT_EQ(moved->GetRefCount(), (size_t)1);
		EB_EXPECT_EQ(source.Ptr(), (RefProbe*)nullptr);
		EB_EXPECT_EQ(destroyed, 0);
	}
	EB_EXPECT_EQ(destroyed, 1);
}

EB_TEST_CASE(Core, SharedPtrCasts, Unit)
{
	int destroyed = 0;
	{
		SharedPtr<DerivedProbe> derived = SharedPtr<DerivedProbe>::Create(&destroyed);
		SharedPtr<RefProbe> asBase = derived;

		EB_EXPECT_EQ(derived->GetRefCount(), (size_t)2);

		// Downcast back to the real type must succeed...
		SharedPtr<DerivedProbe> roundTripped = DynamicPointerCast<DerivedProbe>(asBase);
		EB_CHECK(roundTripped != nullptr);
		EB_EXPECT_EQ(roundTripped->Extra, 5);

		// ...and StaticPointerCast must land on the same object.
		EB_EXPECT_EQ(StaticPointerCast<RefProbe>(derived).Ptr(), asBase.Ptr());
	}
	EB_EXPECT_EQ(destroyed, 1);
}

EB_TEST_CASE(Core, ScopedPtrOwnership, Unit)
{
	int destroyed = 0;
	{
		ScopedPtr<RefProbe> owner = ScopedPtr<RefProbe>::Create(&destroyed);
		EB_CHECK(owner.Ptr() != nullptr);
		owner->Payload = 3;
		EB_EXPECT_EQ(owner->Payload, 3);
		EB_EXPECT_EQ(destroyed, 0);
	}
	EB_EXPECT_EQ(destroyed, 1);
}

//////////////////////////////////////////////////////////////////////////
// Core - engine-wide invariants
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Core, EntityCapacityInvariants, Unit)
{
	// EntityManager stores its component mask as std::bitset<MaxComponents> and its liveness as
	// std::bitset<MaxEntities>, indexed by raw ID. InvalidEntityID must therefore sit OUTSIDE the
	// valid range, or a sentinel value would index real storage.
	EB_EXPECT_GT(Constants::Entities::InvalidEntityID, Constants::Entities::MaxEntities - 1);
	EB_EXPECT_GT(Constants::Entities::MaxEntities, (uint64_t)0);
	EB_EXPECT_GT(Constants::Entities::MaxComponents, (uint64_t)0);
}

EB_TEST_CASE(Core, SaveGameRoundTrip, Unit)
{
	// SaveGameManager writes into %LOCALAPPDATA%\<Project>\SavedGames. A silent regression here
	// loses player progress, which is exactly the class of bug nobody notices until shipping.
	const std::string fileName = "EmberTest_SaveRoundTrip";

	SaveGameManager saves;
	saves.DeleteFromDisk(fileName);

	SaveGameFile& file = saves.Open(fileName).Resolve();
	file.SetInt("Level", 7);
	file.SetFloat("Health", 42.5f);
	file.SetBool("Hardcore", true);
	file.SetString("PlayerName", "Ember");

	// Defaults are returned for keys that were never set.
	EB_EXPECT_EQ(file.GetInt("Missing", -1), -1);
	EB_EXPECT_NEAR(file.GetFloat("Missing", 1.5f), 1.5f, 1e-6);
	EB_EXPECT_FALSE(file.GetBool("Missing", false));
	EB_EXPECT_EQ(file.GetString("Missing", "fallback"), std::string("fallback"));

	if (!saves.Save(fileName))
		EB_SKIP("SaveGameManager could not write to the OS save directory");

	SaveGameManager loaded;
	SaveGameFile& reread = loaded.Open(fileName).Resolve();

	EB_EXPECT_EQ(reread.GetInt("Level", -1), 7);
	EB_EXPECT_NEAR(reread.GetFloat("Health", -1.0f), 42.5f, 1e-3);
	EB_EXPECT(reread.GetBool("Hardcore", false));
	EB_EXPECT_EQ(reread.GetString("PlayerName", ""), std::string("Ember"));
}

EB_TEST_CASE(Core, SaveGameMultipleFilesAreIndependent, Unit)
{
	// Before multi-file support every LoadFromFile cleared the one shared bag, so a game could not
	// keep a settings file and a high-score file alive at the same time.
	SaveGameManager saves;
	saves.DeleteFromDisk("EmberTest_Scores");
	saves.DeleteFromDisk("EmberTest_Settings");

	SaveFileHandle scores = saves.Open("EmberTest_Scores");
	SaveFileHandle settings = saves.Open("EmberTest_Settings");

	// Deliberately the same key in both files.
	scores.Resolve().SetInt("Value", 10);
	settings.Resolve().SetInt("Value", 20);
	settings.Resolve().SetBool("Muted", true);
	settings.Resolve().SetString("Language", "en");

	EB_EXPECT_EQ(scores.Resolve().GetInt("Value", -1), 10);
	EB_EXPECT_EQ(settings.Resolve().GetInt("Value", -1), 20);

	if (!saves.SaveAll())
		EB_SKIP("SaveGameManager could not write to the OS save directory");

	SaveGameManager reloaded;
	EB_EXPECT_EQ(reloaded.Open("EmberTest_Scores").Resolve().GetInt("Value", -1), 10);
	EB_EXPECT_EQ(reloaded.Open("EmberTest_Settings").Resolve().GetInt("Value", -1), 20);
	EB_EXPECT(reloaded.Open("EmberTest_Settings").Resolve().GetBool("Muted", false));
	EB_EXPECT_EQ(reloaded.Open("EmberTest_Settings").Resolve().GetString("Language", ""), std::string("en"));

	// The scores file must not have picked up anything from the settings file.
	EB_EXPECT_FALSE(reloaded.Open("EmberTest_Scores").Resolve().Has("Muted"));
}

EB_TEST_CASE(Core, SaveGameHandleTracksSlotLifetime, Unit)
{
	// Handles store a slot index plus a generation rather than a pointer, so opening more files
	// cannot invalidate them but Close() must.
	SaveGameManager saves;
	saves.DeleteFromDisk("EmberTest_HandleLifetime");

	SaveFileHandle handle = saves.Open("EmberTest_HandleLifetime");
	handle.Resolve().SetInt("Round", 3);

	// Opening other files reallocates the slot vector; the handle has to survive that.
	saves.Open("EmberTest_HandleLifetimeOther1");
	saves.Open("EmberTest_HandleLifetimeOther2");
	EB_CHECK_MSG(handle.IsValid(), "handle went stale after other files were opened");
	EB_EXPECT_EQ(handle.Resolve().GetInt("Round", -1), 3);

	if (!saves.Save("EmberTest_HandleLifetime"))
		EB_SKIP("SaveGameManager could not write to the OS save directory");

	// Reload discards the in-memory edit but keeps the handle pointing at the same slot.
	handle.Resolve().SetInt("Round", 99);
	saves.Reload("EmberTest_HandleLifetime");
	EB_CHECK_MSG(handle.IsValid(), "handle went stale across Reload");
	EB_EXPECT_EQ(handle.Resolve().GetInt("Round", -1), 3);

	saves.Close("EmberTest_HandleLifetime");
	EB_EXPECT_FALSE(handle.IsValid());

	// Reopening the same name reuses the slot index, so the generation bump is the only thing
	// keeping the old handle from resolving to the new file.
	SaveFileHandle reopened = saves.Open("EmberTest_HandleLifetime");
	EB_EXPECT(reopened.IsValid());
	EB_EXPECT_FALSE(handle.IsValid());
}

EB_TEST_CASE(Core, SaveGameLoadsLegacyFormat, Unit)
{
	// Save files written before the versioned format used three per-type sequences under SaveData.
	// Players already have those on disk, so the reader has to keep understanding them.
	SaveGameManager saves;
	const std::filesystem::path saveDir = saves.GetOSSaveDirectory();
	if (saveDir.empty())
		EB_SKIP("SaveGameManager could not resolve the OS save directory");

	const std::filesystem::path filepath = saveDir / "EmberTest_LegacyFormat.sav";
	{
		std::ofstream fout(filepath);
		if (!fout.is_open())
			EB_SKIP("SaveGameManager could not write to the OS save directory");

		fout <<
			"SaveData:\n"
			"  IntData:\n"
			"    - Key: Level\n"
			"      Value: 7\n"
			"  FloatData:\n"
			"    - Key: Health\n"
			"      Value: 42.5\n"
			"  StringData:\n"
			"    - Key: PlayerName\n"
			"      Value: Ember\n";
	}

	SaveFileHandle handle = saves.Open("EmberTest_LegacyFormat");
	EB_EXPECT_EQ(handle.Resolve().GetInt("Level", -1), 7);
	EB_EXPECT_NEAR(handle.Resolve().GetFloat("Health", -1.0f), 42.5f, 1e-3);
	EB_EXPECT_EQ(handle.Resolve().GetString("PlayerName", ""), std::string("Ember"));

	// Saving migrates the file to the versioned format without losing anything.
	EB_CHECK_MSG(saves.Save("EmberTest_LegacyFormat"), "failed to rewrite the migrated save file");

	SaveGameManager reloaded;
	SaveFileHandle migrated = reloaded.Open("EmberTest_LegacyFormat");
	EB_EXPECT_EQ(migrated.Resolve().GetInt("Level", -1), 7);
	EB_EXPECT_EQ(migrated.Resolve().GetString("PlayerName", ""), std::string("Ember"));
}

EB_TEST_CASE(Core, SaveGameValuesCoerceBetweenNumericTypes, Unit)
{
	// YAML does not reliably preserve 1 vs 1.0, so a value written as one numeric type has to be
	// readable as the other. Silently returning the caller's default would look like lost progress.
	SaveGameFile file("EmberTest_Coercion");

	file.SetFloat("Ratio", 2.75f);
	EB_EXPECT_EQ(file.GetInt("Ratio", -1), 2);

	file.SetInt("Count", 5);
	EB_EXPECT_NEAR(file.GetFloat("Count", -1.0f), 5.0f, 1e-6);

	file.SetBool("Enabled", true);
	EB_EXPECT_EQ(file.GetInt("Enabled", -1), 1);
	EB_EXPECT(file.GetBool("Enabled", false));

	// Strings never coerce into numbers, and a missing key still yields the default.
	file.SetString("Name", "Ember");
	EB_EXPECT_EQ(file.GetInt("Name", -1), -1);
	EB_EXPECT_EQ(file.GetString("Ratio", "fallback"), std::string("fallback"));
	EB_EXPECT_EQ(file.GetInt("Missing", -7), -7);

	EB_EXPECT(file.Remove("Name"));
	EB_EXPECT_FALSE(file.Has("Name"));
	EB_EXPECT_FALSE(file.Remove("Name"));
}
