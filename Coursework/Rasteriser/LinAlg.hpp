# pragma once
# include <Eigen/Dense>

// This convenience function converts a 3D vector to a 4D homogeneous vector,
// setting the w component to 1.
Eigen::Vector4f vec3ToVec4(const Eigen::Vector3f& v)
{
	Eigen::Vector4f output;
	output << v.x(), v.y(), v.z(), 1.0f;
	return output;
}

// This is the 2D "cross product" function you implemented last week.
float vec2Cross(const Eigen::Vector2f& v0, const Eigen::Vector2f& v1)
{
	return v0.x() * v1.y() - v0.y() * v1.x();
}

float vec2CrossXY(const Eigen::Vector3f& v0, const Eigen::Vector3f& v1)
{
	return v0.x() * v1.y() - v0.y() * v1.x();
}

Eigen::Vector2f v2(const Eigen::Vector3f& v)
{
	return v.block<2, 1>(0, 0);
}

// ============ TASK 2 =================
Eigen::Matrix4f translationMatrix(const Eigen::Vector3f& t)
{
	// *** Your code here ***
	Eigen::Matrix4f output = Eigen::Matrix4f::Identity();
	output.block<3, 1>(0, 3) = t;
	return output;
}

// Implement this function that makes a uniform scaling matrix
Eigen::Matrix4f scaleMatrix(float s)
{
	// *** Your code here ***
	Eigen::Matrix4f output = Eigen::Matrix4f::Identity();
	output.block<3, 3>(0, 0) *= s;
	return output;
}

Eigen::Matrix4f rotateXMatrix(float theta)
{
	// *** Your code here ***
	Eigen::Matrix4f output;
	output <<
		1.f, 0.f, 0.f, 0.f,
		0.f, cosf(theta), -sinf(theta), 0.f,
		0.f, sinf(theta), cosf(theta), 0.f,
		0.f, 0.f, 0.f, 1.f;
	return output;
}
Eigen::Matrix4f rotateYMatrix(float theta)
{
	// *** Your code here ***
	Eigen::Matrix4f output;
	output <<
		cosf(theta), 0, sinf(theta), 0.f,
		0.f, 1.f, 0.f, 0.f,
		-sinf(theta), 0.f, cosf(theta), 0.f,
		0.f, 0.f, 0.f, 1.f;
	return output;
}

template<typename T> T coeffWiseMultiply(const T& l, const T& r)
{
	return (l.array() * r.array()).matrix();
}

bool outsideClipBox(const Eigen::Vector4f& v)
{
	return
		v.x() < -1.f ||
		v.y() < -1.f ||
		v.z() < -1.f ||
		v.x() > 1.f ||
		v.y() > 1.f ||
		v.z() > 1.f;
}

