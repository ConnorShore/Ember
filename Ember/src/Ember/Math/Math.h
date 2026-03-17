#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/quaternion.hpp>
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

	inline Vector3f operator*(const Matrix4f& matrix, const Vector3f& vector)
	{
		return Vector3f(matrix * Vector4f(vector, 1.0f));
	}

	class Math
	{
	public:

		static inline Matrix4f Translate(const Vector3f& translation) 
		{
			return glm::translate(Matrix4f(1.0f), translation);
		}
		static inline Matrix4f Translate(const Matrix4f& matrix, const Vector3f& translation) 
		{
			return glm::translate(matrix, translation);
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

		static inline Matrix4f GetRotationMatrix(const Vector3f eulerAngles)
		{
			return glm::toMat4(Quaternion(eulerAngles));
		}

		static inline Matrix4f LookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up)
		{
			return glm::lookAt(eye, center, up);
		}

		static inline bool DecomposeTransform(const Matrix4f& transform, Vector3f& outTranslation, Vector3f& outRotation, Vector3f& outScale)
		{
			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;

			bool success = glm::decompose(transform, scale, rotation, translation, skew, perspective);

			if (success)
			{
				outTranslation = translation;
				outScale = scale;
				outRotation = glm::eulerAngles(rotation);
			}

			return success;
		}

		static inline float Radians(float degrees)
		{
			return glm::radians(degrees);
		}

		static inline float Degrees(float radians)
		{
			return glm::degrees(radians);
		}
	};

}