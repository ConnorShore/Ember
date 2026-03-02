#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

	using Vector2f = Vector2<float>;
	using Vector3f = Vector3<float>;
	using Vector4f = Vector4<float>;

	using Matrix2f = Matrix2<float>;
	using Matrix3f = Matrix3<float>;
	using Matrix4f = Matrix4<float>;

	class Math
	{
	public:
		static inline Matrix4f Translate(const Matrix4f& matrix, const Vector3f& translation) {
			return glm::translate(matrix, translation);
		}

		static inline Matrix4f Rotate(const Matrix4f& matrix, float angle, const Vector3f& axis) {
			return glm::rotate(matrix, angle, axis);
		}

		static inline Matrix4f Scale(const Matrix4f& matrix, const Vector3f& scale) {
			return glm::scale(matrix, scale);
		}

		static inline Matrix4f Orthographic(float left, float right, float bottom, float top, float zNear, float zFar) {
			return glm::ortho(left, right, bottom, top, zNear, zFar);
		}
	};

}