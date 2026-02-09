#pragma once

#include <ostream>


// *** Your Code Here! ***
// * Task 1: Finish the implementation of the dot and cross product functions in Vector2.hpp and Vector3.hpp

class Vector2
{
public:
	// This is the default constructor.
	// Change this to set the x, y and z components of the vector to all be zero.
	// You can use an initialiser list, or set them in the constructor function's body.
	Vector2()
		:x_(0), y_(0)
	{
		float x_ = 0;
		float y_ = 0;
	}

	// This constructor can be used to set the x, y and z components of a vector when it
	// is created. Change this function to set the x_, y_ and z_ members to the values
	// provided.
	Vector2(float x, float y)
		:x_(x), y_(y)
	{
		x_ = x;
		y_ = y; 
	}

	// Implement this method to add two vectors.
	Vector2 operator+(const Vector2& other) const
	{
		Vector2 output(x_ + other.x(), y_ + other.y());
		return output;
	}

	Vector2 operator-(const Vector2& other) const
	{
		Vector2 output(x_ - other.x(), y_ - other.y());
		return output;
	}

	// Multiply the vector by a scalar.
	Vector2 operator*(float scalar) const
	{
		Vector2 output(x_ * scalar, y_ * scalar);
		return output;
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
		}
		throw std::runtime_error("Vector access exception!");
	}

	// This is the const version of the [] operator.
	// This version will be used when you have a const reference to a Vector2.
	// The code inside can be the same as for the non-const version above.
	float operator[](int i) const
	{
		switch (i) {
		case 0:
			return x_;
		case 1:
			return y_;
		}
		throw std::runtime_error("Vector access exception!");
	}

	// I have already written these getters for you.
	// Note that they return *references* to x_, y_ and z_
	// This means that they can be used to modify the values, for example:
	// Vector2 myVec;
	// myVec.x() = 10.0f;
	// The const versions return const references, which don't let you change
	// the value of x_, y_ or z_, so this would produce an error:
	// const Vector2& myVecReference;
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

	const float cross(const Vector2& other)
	{
		// YOUR CODE HERE
		// Implement the cross product, following the formula from the slides.
		// Note of course the cross product isn't actually defined for 2D vectors!
		// To implement it here, we're interpreting these vectors as 3D vectors with a z-component of 0
		// We're only returning a float (which is the z-component of the resulting vector)
		// If you think about it, the x and y components of this output vector will always be 0.
		return x_ * other.y_ - y_ * other.x_;
	}

	const float dot(const Vector2& other)
	{
		// YOUR CODE HERE
		// Implement the dot product, following the formula from the slides.
		return (x_ * other.x_) + (y_ * other.y_);
	}

private:
	// Add the data to store the x, y and z coordinates of the vector.
	float x_, y_;
};

// Note: adding this version of operator* will let us do scalar * vector, as well as vector * scalar.
// see how it makes use of the operator* you define above.
Vector2 operator*(float scalar, const Vector2& vector)
{
	return vector * scalar;
}

// This tells us how to add the vector to a stream
// For example, how to print the vector with std::cout.
std::ostream& operator<<(std::ostream& stream, const Vector2& v)
{
	stream << "(" << v[0] << ", " << v[1] << ")";
	return stream;
}

