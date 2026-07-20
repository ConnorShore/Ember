// UNIT TESTS
// ----------
// Pure logic with no engine subsystems, no GL context, no scene. These are the tests you want the most
// of: fast, deterministic, and runnable anywhere. Great for math, containers, IDs, parsing, etc.

#include <Ember.h>
#include "TestFramework.h"

using namespace Ember;
using Ember::Test::Type::Unit;

EB_TEST_CASE(Math, LerpMidpoint, Unit)
{
	EB_CHECK_NEAR(Math::Lerp(0.0f, 10.0f, 0.5f), 5.0f, 1e-5);
	EB_CHECK_NEAR(Math::Lerp(-4.0f, 4.0f, 0.25f), -2.0f, 1e-5);
}

EB_TEST_CASE(Math, NormalizeUnitLength, Unit)
{
	Vector3f n = Math::Normalize(Vector3f(3.0f, 0.0f, 4.0f));
	EB_CHECK_NEAR(Math::Length(n), 1.0f, 1e-5);
	EB_CHECK_NEAR(n.x, 0.6f, 1e-5);
	EB_CHECK_NEAR(n.z, 0.8f, 1e-5);
}

EB_TEST_CASE(Math, RadiansDegreesRoundTrip, Unit)
{
	EB_CHECK_NEAR(Math::Degrees(Math::Radians(137.0f)), 137.0f, 1e-3);
}

EB_TEST_CASE(Math, DecomposeTransformRoundTrip, Unit)
{
	// Build a TRS matrix, decompose it, and confirm we recover translation & scale. This is the exact
	// path the editor gizmos and transform serialization depend on, so it earns its keep as a unit test.
	const Vector3f translation(2.0f, -3.0f, 5.0f);
	const Vector3f eulerRadians(Math::Radians(30.0f), Math::Radians(45.0f), 0.0f);
	const Vector3f scale(1.5f, 2.0f, 0.5f);

	const Matrix4f m = Math::Translate(translation) * Math::GetRotationMatrix(eulerRadians) * Math::Scale(scale);

	Vector3f outT, outR, outS;
	const bool ok = Math::DecomposeTransform(m, outT, outR, outS);
	EB_CHECK(ok);

	EB_CHECK_NEAR(outT.x, translation.x, 1e-4);
	EB_CHECK_NEAR(outT.y, translation.y, 1e-4);
	EB_CHECK_NEAR(outT.z, translation.z, 1e-4);

	EB_CHECK_NEAR(outS.x, scale.x, 1e-4);
	EB_CHECK_NEAR(outS.y, scale.y, 1e-4);
	EB_CHECK_NEAR(outS.z, scale.z, 1e-4);
	// (Euler comparison is intentionally omitted — equal rotations have multiple valid Euler triples.)
}

EB_TEST_CASE(Uuid, DistinctAndStable, Unit)
{
	const UUID a; // freshly generated
	const UUID b; // freshly generated
	EB_CHECK(a != b); // collision is astronomically unlikely

	const UUID fixed(42);
	EB_CHECK(fixed == UUID(42)); // constructing from the same value is stable
}
