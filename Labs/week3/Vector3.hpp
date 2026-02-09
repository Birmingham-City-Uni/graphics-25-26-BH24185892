#pragma once

#include <ostream>


// *** Your Code Here! ***
// * Task 1: Finish the implementation of the dot and cross product functions in Vector2.hpp and Vector3.hpp
class Vector3
{
public:
	// This is the default constructor.
	// Change this to set the x, y and z components of the vector to all be zero.
	Vector3()
		:x_(0), y_(0), z_(0)
	{
		float x_ = 0;
		float y_ = 0;
		float z_ = 0;
	}

	// This constructor can be used to set the x, y and z components of a vector when it
	// is created. Change this function to set the x_, y_ and z_ members to the values
	// provided.
	Vector3(float x, float y, float z)
		:x_(x), y_(y), z_(z)
	{
		x_ = x;
		y_ = y;
		z_ = z;
	}

	// Implement this method to add two vectors.
	Vector3 operator+(const Vector3& other) const
	{
		Vector3 output(x_ + other.x(), y_ + other.y(), z_ + other.z());
		return output;
	}

	Vector3 operator-(const Vector3& other) const
	{
		Vector3 output(x_ - other.x(), y_ - other.y(), z_ - other.z());
		return output;
	}

	// Multiply the vector by a scalar.
	Vector3 operator*(float scalar) const
	{
		Vector3 output(x_ * scalar, y_ * scalar, z_ * scalar);
		return output;
	}

	Vector3 operator/(float scalar) const
	{
		Vector3 output(x_ / scalar, y_ / scalar, z_ / scalar);
		return output;
	}

	Vector3 cross(const Vector3& other) const
	{
		// YOUR CODE HERE
		// Implement the cross product, following the formula from the slides.
		return Vector3(y_ * other.z_ - z_ * other.y_, x_ * other.z_ - z_ * other.x_, x_ * other.y_ - y_ * other.x_);
	}

	float dot(const Vector3& other) const
	{
		// YOUR CODE HERE
		// Implement the dot product, following the formula from the slides.
		return (x_ * other.x_) + (y_ * other.y_) + (z_ * other.z_);
	}

	float length() const
	{
		return sqrtf(x_ * x_ + y_ * y_ + z_ * z_);
	}

	Vector3 normalized() const
	{
		return *this / this->length();
	}

	// Get a component of the vector
	// This is the index operator - it's the one that is used when a user 
	// indexes part of a vector, for example:
	// myVec[0] = 1.0f; // Set the x component of this vector to 1.
	// Your code should return either x_, y_ or z_, depending on the value of i.
	// If the user tries to access an entry other than 0, 1 or 2, throw an
	// error.
	float& operator[](int i)
	{
		switch (i) {
		case 0:
			return x_;
		case 1:
			return y_;
		case 2:
			return z_;
		}
		throw std::runtime_error("Vector access exception!");
	}

	// This is the const version of the [] operator.
	// This version will be used when you have a const reference to a Vector3.
	// The code inside can be the same as for the non-const version above.
	float operator[](int i) const
	{
		switch (i) {
		case 0:
			return x_;
		case 1:
			return y_;
		case 2:
			return z_;
		}
		throw std::runtime_error("Vector access exception!");
	}

	// I have already written these getters for you.
	// Note that they return *references* to x_, y_ and z_
	// This means that they can be used to modify the values, for example:
	// Vector3 myVec;
	// myVec.x() = 10.0f;
	// The const versions return const references, which don't let you change
	// the value of x_, y_ or z_, so this would produce an error:
	// const Vector3& myVecReference;
	// myVecReference.x() = 10.0f;
	const float& x() const
	{
		return x_;
	}
	float& x()
	{
		return x_;
	}

	const float& y() const
	{
		return y_;
	}
	float& y()
	{
		return y_;
	}

	const float& z() const
	{
		return z_;
	}
	float& z()
	{
		return z_;
	}

private:
	// Add the data to store the x, y and z coordinates of the vector.
	float x_, y_, z_;
};

// Note: adding this version of operator* will let us do scalar * vector, as well as vector * scalar.
// see how it makes use of the operator* you define above.
Vector3 operator*(float scalar, const Vector3& vector)
{
	return vector * scalar;
}

// This tells us how to add the vector to a stream
// For example, how to print the vector with std::cout.
std::ostream& operator<<(std::ostream& stream, const Vector3& v)
{
	stream << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
	return stream;
}

