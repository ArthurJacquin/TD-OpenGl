#include "Vec3.h"

class Matrix4 
{
	float m_mat[16];

public:
	Matrix4();
	Matrix4(float m00, float m01, float m02, float m03,
			float m04, float m05, float m06, float m07,
			float m08, float m09, float m10, float m11,
			float m12, float m13, float m14, float m15);
	Matrix4(const Matrix4&);

	float* getMatrix();
	Matrix4 Matrix4::Translate(float x, float y, float z);
	Matrix4 Rotate(Vec3 angle);
	Matrix4 Scale(float scale);
	Matrix4 Ortho(float left, float right, float bottom, float top, float znear, float zfar);
	Matrix4 Perspective(float FoV, float aspectRatio, float n, float f);
	Matrix4 LookAt(Vec3 position, Vec3 target, Vec3 up);

	Matrix4& operator=(Matrix4);
	Matrix4 operator*(const Matrix4& m);
	Vec3 operator*(const Vec3& v);
	float& operator[](int id);

	void Debug();
};
