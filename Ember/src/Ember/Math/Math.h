#pragma once

// Engine-facing math facade. GLM remains the backing implementation, while this
// header owns Ember's public math types and stable helper API.

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "Ember/Performance/Profiler.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Ember {

	template<glm::length_t Size, typename T = float, glm::qualifier Qualifier = glm::defaultp>
	using Vector = glm::vec<Size, T, Qualifier>;

	template<glm::length_t Columns, glm::length_t Rows, typename T = float, glm::qualifier Qualifier = glm::defaultp>
	using Matrix = glm::mat<Columns, Rows, T, Qualifier>;

	template<typename T, glm::qualifier Qualifier = glm::defaultp>
	using Vector2 = Vector<2, T, Qualifier>;

	template<typename T, glm::qualifier Qualifier = glm::defaultp>
	using Vector3 = Vector<3, T, Qualifier>;

	template<typename T, glm::qualifier Qualifier = glm::defaultp>
	using Vector4 = Vector<4, T, Qualifier>;

	template<typename T, glm::qualifier Qualifier = glm::defaultp>
	using Matrix2 = Matrix<2, 2, T, Qualifier>;

	template<typename T, glm::qualifier Qualifier = glm::defaultp>
	using Matrix3 = Matrix<3, 3, T, Qualifier>;

	template<typename T, glm::qualifier Qualifier = glm::defaultp>
	using Matrix4 = Matrix<4, 4, T, Qualifier>;

	template<typename T, glm::qualifier Qualifier = glm::defaultp>
	using QuaternionT = glm::qua<T, Qualifier>;

	using Quaternion = QuaternionT<float>;

	using Vector2f = Vector2<float>;
	using Vector3f = Vector3<float>;
	using Vector4f = Vector4<float>;

	using Vector2i = Vector2<int>;
	using Vector3i = Vector3<int>;
	using Vector4i = Vector4<int>;

	using Matrix2f = Matrix2<float>;
	using Matrix3f = Matrix3<float>;
	using Matrix4f = Matrix4<float>;

	struct TransformComponents
	{
		Vector3f Translation = Vector3f(0.0f);
		Vector3f Rotation = Vector3f(0.0f);
		Vector3f Scale = Vector3f(1.0f);
	};

	namespace MathConstants {
		inline constexpr float Epsilon = 1.0e-6f;
		inline constexpr float ParallelDotThreshold = 0.99f;
	}

	namespace MathDetail {

		template<typename T>
		concept Arithmetic = std::is_arithmetic_v<T>;

		template<typename T>
		[[nodiscard]] constexpr T Min(T a, T b)
		{
			return std::min(a, b);
		}

		template<typename T>
		[[nodiscard]] constexpr T Max(T a, T b)
		{
			return std::max(a, b);
		}

		template<typename T>
		[[nodiscard]] constexpr T Lerp(const T& a, const T& b, float t)
		{
			return a + t * (b - a);
		}

		[[nodiscard]] inline bool IsAffineTransform(const Matrix4f& transform, float epsilon = MathConstants::Epsilon)
		{
			return std::abs(transform[0][3]) <= epsilon
				&& std::abs(transform[1][3]) <= epsilon
				&& std::abs(transform[2][3]) <= epsilon
				&& std::abs(transform[3][3] - 1.0f) <= epsilon;
		}

		[[nodiscard]] inline Vector3f SelectLookAtUp(const Vector3f& direction)
		{
			const Vector3f worldUp(0.0f, 1.0f, 0.0f);
			const float alignment = std::abs(glm::dot(direction, worldUp));
			return alignment > MathConstants::ParallelDotThreshold
				? Vector3f(0.0f, 0.0f, 1.0f)
				: worldUp;
		}

		[[nodiscard]] inline bool DecomposeTransform(const Matrix4f& transform, TransformComponents& outComponents)
		{
			if (!IsAffineTransform(transform))
				return false;

			outComponents.Translation = Vector3f(transform[3]);

			Vector3f basisX(transform[0]);
			Vector3f basisY(transform[1]);
			Vector3f basisZ(transform[2]);

			Vector3f scale(glm::length(basisX), glm::length(basisY), glm::length(basisZ));
			if (scale.x < MathConstants::Epsilon || scale.y < MathConstants::Epsilon || scale.z < MathConstants::Epsilon)
			{
				outComponents.Rotation = Vector3f(0.0f);
				outComponents.Scale = scale;
				return false;
			}

			const float determinant = glm::determinant(Matrix3f(basisX, basisY, basisZ));
			if (determinant < 0.0f)
			{
				scale.x = -scale.x;
				basisX = -basisX;
			}

			const Matrix3f rotationMatrix(basisX / scale.x, basisY / scale.y, basisZ / scale.z);
			const Quaternion rotation = glm::quat_cast(rotationMatrix);

			outComponents.Rotation = glm::eulerAngles(rotation);
			outComponents.Scale = scale;
			return true;
		}

	}

	// Convenience: multiply a 4x4 matrix by a 3D vector (treats as w=1, returns xyz)
	[[nodiscard]] inline Vector3f operator*(const Matrix4f& matrix, const Vector3f& vector)
	{
		return Vector3f(matrix * Vector4f(vector, 1.0f));
	}

	class Math
	{
	public:
		static inline constexpr float Epsilon = MathConstants::Epsilon;
		static inline constexpr float ParallelDotThreshold = MathConstants::ParallelDotThreshold;

		static inline float RandomFloat(float min, float max)
		{
			return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (max - min)));
		}

		static inline int RandomInt(int min, int max)
		{
			return min + std::rand() % (max - min + 1);
		}

		static inline float Random()
		{
			return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
		}

		template<MathDetail::Arithmetic T>
		static inline constexpr T Max(T a, T b)
		{
			return MathDetail::Max(a, b);
		}

		template<MathDetail::Arithmetic T>
		static inline constexpr T Min(T a, T b)
		{
			return MathDetail::Min(a, b);
		}

		static inline Vector3f Min(const Vector3f& a, const Vector3f& b)
		{
			return glm::min(a, b);
		}

		static inline Vector3f Max(const Vector3f& a, const Vector3f& b)
		{
			return glm::max(a, b);
		}

		static inline Matrix4f Translate(const Vector3f& translation) 
		{
			return glm::translate(Matrix4f(1.0f), translation);
		}
		static inline Matrix4f Translate(const Matrix4f& matrix, const Vector3f& translation) 
		{
			return glm::translate(matrix, translation);
		}

		static inline Vector3f Rotate(const Quaternion& rotation, const Vector3f& vector)
		{
			return glm::rotate(rotation, vector);
		}

		static inline Matrix4f Rotate(float angle, const Vector3f& axis) 
		{
			return glm::rotate(Matrix4f(1.0f), angle, axis);
		}
		static inline Matrix4f Rotate(const Matrix4f& matrix, float angle, const Vector3f& axis) 
		{
			return glm::rotate(matrix, angle, axis);
		}

		static inline Matrix4f Scale(const Vector3f& scale) 
		{
			return glm::scale(Matrix4f(1.0f), scale);
		}
		static inline Matrix4f Scale(const Matrix4f& matrix, const Vector3f& scale) 
		{
			return glm::scale(matrix, scale);
		}

		static inline Matrix4f Orthographic(float left, float right, float bottom, float top, float zNear = -1.0f, float zFar = 1.0f) 
		{
			return glm::ortho(left, right, bottom, top, zNear, zFar);
		}

		static inline Matrix4f Perspective(float fovDegrees, float aspectRatio, float zNear, float zFar)
		{
			return glm::perspective(glm::radians(fovDegrees), aspectRatio, zNear, zFar);
		}

		static inline Matrix4f Inverse(const Matrix4f& matrix)
		{
			return glm::inverse(matrix);
		}

		static inline Vector3f Normalize(const Vector3f& vector)
		{
			return glm::normalize(vector);
		}

		static inline Quaternion Normalize(const Quaternion& quat)
		{
			return glm::normalize(quat);
		}

		static inline Quaternion ToQuaternion(const Matrix4f& matrix)
		{
			return glm::quat(matrix);
		}

		static inline Quaternion ToQuaternion(const Vector3f& eulerAngles)
		{
			return glm::quat(eulerAngles);
		}

		static inline Vector3f ToEulerAngles(const Quaternion& quat)
		{
			return glm::eulerAngles(quat);
		}

		static inline Matrix4f GetRotationMatrix(const Vector3f& eulerAngles)
		{
			return glm::toMat4(Quaternion(eulerAngles));
		}

		static inline Matrix4f LookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up)
		{
			return glm::lookAt(eye, center, up);
		}

		static inline Vector3f LookAt(const Vector3f& start, const Vector3f& end)
		{
			const Vector3f direction = Normalize(end - start);
			const Quaternion quat = glm::quatLookAt(direction, MathDetail::SelectLookAtUp(direction));
			return ToEulerAngles(quat);
		}

		static inline float Length(const Vector3f& vector)
		{
			return glm::length(vector);
		}

		static inline float Length(const Vector2f& vector)
		{
			return glm::length(vector);
		}

		static inline Matrix4f MakeMatrix4f(const float* data)
		{
			return glm::make_mat4(data);
		}

		static inline bool DecomposeTransform(const Matrix4f& transform, TransformComponents& outComponents)
		{
			return MathDetail::DecomposeTransform(transform, outComponents);
		}

		// Extracts translation, rotation (Euler radians), and scale from a TRS matrix.
		// Handles negative scaling by flipping one basis axis when needed so the
		// extracted rotation remains a proper rotation.
		static inline bool DecomposeTransform(const Matrix4f& transform, Vector3f& outTranslation, Vector3f& outRotation, Vector3f& outScale)
		{
			TransformComponents components;
			const bool success = DecomposeTransform(transform, components);

			outTranslation = components.Translation;
			outRotation = components.Rotation;
			outScale = components.Scale;
			return success;
		}

		static inline Matrix4f ToMatrix4f(const Quaternion& quat)
		{
			return glm::toMat4(quat);
		}

		static inline float Radians(float degrees)
		{
			return glm::radians(degrees);
		}

		static inline float Degrees(float radians)
		{
			return glm::degrees(radians);
		}

		template<typename T>
		static inline T Normalize(const T& value, float min, float max)
		{
			return (value - min) / (max - min);
		}

		template<typename T>
		static inline T Lerp(const T& a, const T& b, float t)
		{
			return MathDetail::Lerp(a, b, t);
		}

		static inline Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
		{
			return glm::slerp(a, b, t);
		}

		static inline Vector3f Mix(const Vector3f& a, const Vector3f& b, float t)
		{
			return glm::mix(a, b, t);
		}

		template<typename T>
		static inline T Clamp(const T& value, const T& min, const T& max)
		{
			return glm::clamp(value, min, max);
		}

		static inline Vector3f Cross(const Vector3f& a, const Vector3f& b)
		{
			return glm::cross(a, b);
		}

		static inline float Dot(const Vector3f& a, const Vector3f& b)
		{
			return glm::dot(a, b);
		}

		static inline float Dot(const Vector2f& a, const Vector2f& b)
		{
			return glm::dot(a, b);
		}
		
		static inline Quaternion AngleAxis(float radians, const Vector3f& axis)
		{
			return glm::angleAxis(radians, axis);
		}

		static inline Vector3f ProjectOnPlane(const Vector3f& vector, const Vector3f& planeNormal)
		{
			return vector - Dot(vector, planeNormal) * planeNormal;
		}

		static inline float Magnitude(const Vector3f& vector)
		{
			return glm::length(vector);
		}

		static inline float Magnitude2(const Vector3f& vector)
		{
			return glm::length2(vector);
		}

		static inline float Distance(const Vector3f& a, const Vector3f& b)
		{
			return glm::distance(a, b);
		}

		static inline float Distance2(const Vector3f& a, const Vector3f& b)
		{
			return glm::distance2(a, b);
		}

		static inline float Sin(float radians)
		{
			return glm::sin(radians);
		}

		static inline float Cos(float radians)
		{
			return glm::cos(radians);
		}

		static inline float Tan(float radians)
		{
			return glm::tan(radians);
		}

		static inline float Atan(float x)
		{
			return glm::atan(x);
		}

		static inline float Atan2(float y, float x)
		{
			return glm::atan(y, x); // GLM's atan(y, x) is actually atan2
		}

		static inline float Acos(float x)
		{
			return glm::acos(x);
		}

		static inline float Asin(float x)
		{
			return glm::asin(x);
		}

		static inline float Abs(float x)
		{
			return glm::abs(x);
		}

		static inline float Round(float x)
		{
			return glm::round(x);
		}

		static inline float Floor(float x)
		{
			return glm::floor(x);
		}

		static inline float Ceil(float x)
		{
			return glm::ceil(x);
		}

		// Rounds to the nearest multiple of `increment`; a non-positive increment means no snapping.
		static inline float Snap(float value, float increment)
		{
			return increment > 0.0f ? glm::round(value / increment) * increment : value;
		}

		static inline Vector3f Snap(const Vector3f& value, float increment)
		{
			return Vector3f(Snap(value.x, increment), Snap(value.y, increment), Snap(value.z, increment));
		}

		static inline bool IsNaN(float x)
		{
			return std::isnan(x);
		}

		// False for NaN and for both infinities, so it doubles as the guard on values read from disk.
		static inline bool IsFinite(float x)
		{
			return std::isfinite(x);
		}

	};

}