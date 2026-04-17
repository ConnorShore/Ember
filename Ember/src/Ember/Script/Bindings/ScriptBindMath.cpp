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

	}

}
