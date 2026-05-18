#pragma once

// Thin wrapper around GLM providing engine-standard type aliases and common math operations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Ember {

	template<typename T>
	using Vector2 = glm::vec<2, T>;

	template<typename T>
	using Vector3 = glm::vec<3, T>;

	template<typename T>
	using Vector4 = glm::vec<4, T>;

	template<typename T>
	using Matrix2 = glm::mat<2, 2, T>;

	template<typename T>
	using Matrix3 = glm::mat<3, 3, T>;

	template<typename T>
	using Matrix4 = glm::mat<4, 4, T>;

	using Quaternion = glm::quat;

	using Vector2f = Vector2<float>;
	using Vector3f = Vector3<float>;
	using Vector4f = Vector4<float>;

	using Matrix2f = Matrix2<float>;
	using Matrix3f = Matrix3<float>;
	using Matrix4f = Matrix4<float>;

	// Convenience: multiply a 4x4 matrix by a 3D vector (treats as w=1, returns xyz)
	inline Vector3f operator*(const Matrix4f& matrix, const Vector3f& vector)
	{
		return Vector3f(matrix * Vector4f(vector, 1.0f));
	}

	class Math
	{
	public:

		static inline float RandomFloat(float min, float max)
		{
			return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
		}

		static inline int RandomInt(int min, int max)
		{
			return min + rand() % (max - min + 1);
		}

		static inline float Random()
		{
			return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		}

		static inline int Max(int a, int b)
		{
			return (a > b) ? a : b;
		}

		static inline int Min(int a, int b)
		{
			return (a < b) ? a : b;
		}

		static inline float Max(float a, float b)
		{
			return (a > b) ? a : b;
		}

		static inline float Min(float a, float b)
		{
			return (a < b) ? a : b;
		}

		static inline Vector3f Min(const Vector3f& a, const Vector3f& b)
		{
			return Vector3f(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z));
		}

		static inline Vector3f Max(const Vector3f& a, const Vector3f& b)
		{
			return Vector3f(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z));
		}

		static inline Matrix4f Translate(const Vector3f& translation) 
		{
			return glm::translate(Matrix4f(1.0f), translation);
		}
		static inline Matrix4f Translate(const Matrix4f& matrix, const Vector3f& translation) 
		{
			return glm::translate(matrix, translation);
		}

		static inline Vector3f Rotate(const Quaternion& rotation, const Vector3f angle)
		{
			return glm::rotate(rotation, angle);
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

		static inline Matrix4f GetRotationMatrix(const Vector3f eulerAngles)
		{
			return glm::toMat4(Quaternion(eulerAngles));
		}

		static inline Matrix4f LookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up)
		{
			return glm::lookAt(eye, center, up);
		}

		static inline float Length(const Vector3f& vector)
		{
			return glm::length(vector);
		}

		static inline Matrix4f MakeMatrix4f(const float* data)
		{
			return glm::make_mat4(data);
		}

		// Extracts translation, rotation (Euler radians), and scale from a TRS matrix.
		// Handles negative scaling correctly: when the upper 3x3 has a negative
		// determinant (mirrored / negative scale), one axis is flipped so the
		// remaining rotation is a proper rotation (det == +1) and the quaternion
		// extraction does not produce NaNs.
		static inline bool DecomposeTransform(const Matrix4f& transform, Vector3f& outTranslation, Vector3f& outRotation, Vector3f& outScale)
		{
			// Reject matrices with a perspective row or zero w; we only support affine TRS.
			constexpr float kEpsilon = 1e-6f;
			if (std::abs(transform[0][3]) > kEpsilon ||
				std::abs(transform[1][3]) > kEpsilon ||
				std::abs(transform[2][3]) > kEpsilon ||
				std::abs(transform[3][3] - 1.0f) > kEpsilon)
			{
				return false;
			}

			// Translation lives in column 3.
			outTranslation = Vector3f(transform[3]);

			// Upper-left 3x3 contains rotation * scale.
			glm::vec3 col0(transform[0]);
			glm::vec3 col1(transform[1]);
			glm::vec3 col2(transform[2]);

			glm::vec3 scale(glm::length(col0), glm::length(col1), glm::length(col2));

			if (scale.x < kEpsilon || scale.y < kEpsilon || scale.z < kEpsilon)
			{
				// Degenerate basis — cannot recover a rotation reliably.
				outScale = scale;
				outRotation = Vector3f(0.0f);
				return false;
			}

			// If the basis is left-handed (mirrored), flip one axis so the
			// remaining 3x3 is a proper rotation matrix. We pick X by convention;
			// the resulting (rotation, scale) pair still reconstructs the original
			// matrix exactly.
			float det = glm::determinant(glm::mat3(col0, col1, col2));
			if (det < 0.0f)
			{
				scale.x = -scale.x;
				col0 = -col0;
			}

			glm::mat3 rotMtx(col0 / scale.x, col1 / scale.y, col2 / scale.z);
			glm::quat rotation = glm::quat_cast(rotMtx);

			outScale = scale;
			outRotation = glm::eulerAngles(rotation);
			return true;
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

		static inline float Lerp(float a, float b, float t)
		{
			return a + t * (b - a);
		}

		static inline Vector3f Lerp(const Vector3f& a, const Vector3f& b, float t)
		{
			return a + t * (b - a);
		}

		static inline Vector4f Lerp(const Vector4f& a, const Vector4f& b, float t)
		{
			return a + t * (b - a);
		}

		static inline Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
		{
			return glm::slerp(a, b, t);
		}

		static inline Vector3f Mix(const Vector3f& a, const Vector3f& b, float t)
		{
			return glm::mix(a, b, t);
		}

		static inline float Clamp(float value, float min, float max)
		{
			return glm::clamp(value, min, max);
		}

		static inline Vector3f Clamp(const Vector3f& value, const Vector3f& min, const Vector3f& max)
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
		
		static inline Quaternion AngleAxis(float radians, const Vector3f vec)
		{
			return glm::angleAxis(radians, vec);
		}

		static inline Vector3f ProjectOnPlane(const Vector3f& vector, const Vector3f& planeNormal)
		{
			return vector - Dot(vector, planeNormal) * planeNormal;
		}

		static inline float Distance(const Vector3f& a, const Vector3f& b)
		{
			return glm::distance(a, b);
		}

		static inline float Distance2(const Vector3f& a, const Vector3f& b)
		{
			return glm::distance2(a, b);
		}

	};

}