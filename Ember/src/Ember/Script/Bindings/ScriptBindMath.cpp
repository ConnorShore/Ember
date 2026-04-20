#include "ebpch.h"
#include "ScriptBindMath.h"

#include "Ember/Math/Math.h"

namespace Ember {

	void BindMath(sol::state& state)
	{
		state.new_usertype<Vector2f>("Vector2f",
			sol::constructors<Vector2f(), Vector2f(float, float)>(),
			"x", &Vector2f::x,
			"y", &Vector2f::y,

			// Math operations
			sol::meta_function::addition, [](const Vector2f& a, const Vector2f& b) {
				return a + b;
			},

			sol::meta_function::subtraction, [](const Vector2f& a, const Vector2f& b) {
				return a - b;
			},

			sol::meta_function::multiplication, [](const Vector2f& a, float scalar) {
				return a * scalar;
			},

			// Division by Scalar (Vector / float)
			sol::meta_function::division, [](const Vector2f& a, float scalar) {
				return a / scalar;
			},

			// Unary Minus (-Vector)
			sol::meta_function::unary_minus, [](const Vector2f& a) {
				return -a;
			}
		);

		state.new_usertype<Vector3f>("Vector3f",
			sol::constructors<Vector3f(), Vector3f(float, float, float)>(),
			"x", &Vector3f::x,
			"y", &Vector3f::y,
			"z", &Vector3f::z,

			// Math operations
			sol::meta_function::addition, [](const Vector3f& a, const Vector3f& b) {
				return a + b;
			},

			sol::meta_function::subtraction, [](const Vector3f& a, const Vector3f& b) {
				return a - b;
			},

			sol::meta_function::multiplication, [](const Vector3f& a, float scalar) {
				return a * scalar;
			},

			// Division by Scalar (Vector / float)
			sol::meta_function::division, [](const Vector3f& a, float scalar) {
				return a / scalar;
			},

			// Unary Minus (-Vector)
			sol::meta_function::unary_minus, [](const Vector3f& a) {
				return -a;
			}
		);

		state.new_usertype<Vector4f>("Vector4f",
			sol::constructors<Vector4f(), Vector4f(float, float, float, float)>(),
			"x", &Vector4f::x,
			"y", &Vector4f::y,
			"z", &Vector4f::z,
			"w", &Vector4f::w,
			// Math operations
			sol::meta_function::addition, [](const Vector4f& a, const Vector4f& b) {
				return a + b;
			},
			sol::meta_function::subtraction, [](const Vector4f& a, const Vector4f& b) {
				return a - b;
			},
			sol::meta_function::multiplication, [](const Vector4f& a, float scalar) {
				return a * scalar;
			},
			// Division by Scalar (Vector / float)
			sol::meta_function::division, [](const Vector4f& a, float scalar) {
				return a / scalar;
			},
			// Unary Minus (-Vector)
			sol::meta_function::unary_minus, [](const Vector4f& a) {
				return -a;
			}
		);

		state.new_usertype<Matrix3f>("Matrix3f",
			sol::constructors<Matrix3f(), Matrix3f(float)>(), // Default, and Diagonal (e.g. Matrix3f(1.0) for identity)

			sol::meta_function::addition, [](const Matrix3f& a, const Matrix3f& b) { return a + b; },
			sol::meta_function::subtraction, [](const Matrix3f& a, const Matrix3f& b) { return a - b; },

			// Matrices can multiply against other matrices, vectors, or scalars!
			sol::meta_function::multiplication, sol::overload(
				[](const Matrix3f& a, const Matrix3f& b) { return a * b; },
				[](const Matrix3f& m, const Vector3f& v) { return m * v; },
				[](const Matrix3f& a, float scalar) { return a * scalar; }
			)
		);

		state.new_usertype<Matrix4f>("Matrix4f",
			sol::constructors<Matrix4f(), Matrix4f(float)>(),

			sol::meta_function::addition, [](const Matrix4f& a, const Matrix4f& b) { return a + b; },
			sol::meta_function::subtraction, [](const Matrix4f& a, const Matrix4f& b) { return a - b; },

			sol::meta_function::multiplication, sol::overload(
				[](const Matrix4f& a, const Matrix4f& b) { return a * b; },
				[](const Matrix4f& m, const Vector4f& v) { return m * v; },
				[](const Matrix4f& m, const Vector3f& v) { return m * v; }, // This maps to your custom Vector3f * Matrix4f operator!
				[](const Matrix4f& a, float scalar) { return a * scalar; }
			)
		);

		state.new_usertype<Quaternion>("Quaternion",
			sol::constructors<Quaternion(), Quaternion(float, float, float, float), Quaternion(const Vector3f&)>(),
			"x", &Quaternion::x,
			"y", &Quaternion::y,
			"z", &Quaternion::z,
			"w", &Quaternion::w,

			sol::meta_function::multiplication, sol::overload(
				[](const Quaternion& a, const Quaternion& b) { return a * b; },
				[](const Quaternion& q, const Vector3f& v) { return q * v; } // Rotating a vector by a quaternion
			),

			// Useful direct methods
			"Inverse", [](const Quaternion& q) { return Math::Inverse(glm::mat4_cast(q)); }, // GLM has inverse(quat) but we can route it safely
			"Normalize", [](const Quaternion& q) { return Math::Normalize(q); }
		);

		// --- Global Math Table ---
		auto math = state.create_table("Math");

		// Constants
		math["PI"] = glm::pi<float>();

		// Basic Utilities
		math.set_function("Max", sol::overload(
			[](float a, float b) { return Math::Max(a, b); },
			[](const Vector3f& a, const Vector3f& b) { return Math::Max(a, b); }
		));
		math.set_function("Min", sol::overload(
			[](float a, float b) { return Math::Min(a, b); },
			[](const Vector3f& a, const Vector3f& b) { return Math::Min(a, b); }
		));

		math.set_function("Radians", &Math::Radians);
		math.set_function("Degrees", &Math::Degrees);
		math.set_function("Length", &Math::Length);
		math.set_function("Cross", &Math::Cross);
		math.set_function("Dot", &Math::Dot);
		math.set_function("ProjectOnPlane", &Math::ProjectOnPlane);

		// Interpolation
		math.set_function("Lerp", sol::overload(
			[](float a, float b, float t) { return Math::Lerp(a, b, t); },
			[](const Vector3f& a, const Vector3f& b, float t) { return Math::Lerp(a, b, t); }
		));
		math.set_function("Slerp", &Math::Slerp);

		// Matrix / Transform Math
		math.set_function("Inverse", &Math::Inverse);
		math.set_function("LookAt", &Math::LookAt);
		math.set_function("GetRotationMatrix", &Math::GetRotationMatrix);

		math.set_function("Translate", sol::overload(
			[](const Vector3f& t) { return Math::Translate(t); },
			[](const Matrix4f& m, const Vector3f& t) { return Math::Translate(m, t); }
		));
		math.set_function("Rotate", sol::overload(
			[](float angle, const Vector3f& axis) { return Math::Rotate(angle, axis); },
			[](const Matrix4f& m, float angle, const Vector3f& axis) { return Math::Rotate(m, angle, axis); }
		));
		math.set_function("Scale", sol::overload(
			[](const Vector3f& s) { return Math::Scale(s); },
			[](const Matrix4f& m, const Vector3f& s) { return Math::Scale(m, s); }
		));

		// Conversions
		math.set_function("ToQuaternion", sol::overload(
			[](const Matrix4f& m) { return Math::ToQuaternion(m); },
			[](const Vector3f& e) { return Math::ToQuaternion(e); }
		));
		math.set_function("ToEulerAngles", &Math::ToEulerAngles);
		math.set_function("ToMatrix4f", &Math::ToMatrix4f);

		// Can return like:
		//	local success, translation, rotation, scale = Math.DecomposeTransform(transform)
		math.set_function("DecomposeTransform", [](const Matrix4f& transform) {
			Vector3f translation, rotation, scale;
			bool success = Math::DecomposeTransform(transform, translation, rotation, scale);
			return std::make_tuple(success, translation, rotation, scale);
			});
	}

}
