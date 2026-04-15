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

		// Extracts translation, rotation (Euler), and scale from a transform matrix.
		// Uses GLM decompose which handles skew/perspective but we only use TRS.
		static inline bool DecomposeTransform(const Matrix4f& transform, Vector3f& outTranslation, Vector3f& outRotation, Vector3f& outScale)
		{
			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;

			// TODO: May want to optimize, glm::decompress does a lot of work that we don't need
			bool success = glm::decompose(transform, scale, rotation, translation, skew, perspective);
			if (success)
			{
				outTranslation = translation;
				outScale = scale;
				outRotation = glm::eulerAngles(rotation);
			}

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

		static inline float Lerp(float a, float b, float t)
		{
			return a + t * (b - a);
		}

		static inline Vector3f Lerp(const Vector3f& a, const Vector3f& b, float t)
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

	};

}