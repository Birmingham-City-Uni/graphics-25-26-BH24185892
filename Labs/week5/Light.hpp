#pragma once
#include <Eigen/Dense>
#include <vector>
#include <fstream>

/// <summary>
/// Abstract class representing a light source.
/// </summary>
class Light {
protected:
	Eigen::Vector3f _intensity;
public:
	Light(const Eigen::Vector3f& intensity)
		:_intensity(intensity)
	{};

	enum Type { POINT, SPOT, DIRECTIONAL, AMBIENT };

	/// <summary>
	/// Returns the intensity of this light source at a given location in world space.
	/// </summary>
	/// <param name="surfaceLocation"></param>
	/// <returns>RGB intensity at the given location.</returns>
	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) = 0;

	/// <summary>
	/// Check if the current light source is an ambient light.
	/// For ambient lights, getDirection should not be called (ambient lights have no
	/// world space location or direction).
	/// </summary>
	/// <returns>True if ambient, false otherwise</returns>
	virtual Type getType() = 0;

	/// <summary>
	/// Gets a unit direction vector representing the incoming direction of the light
	/// source at a given surface point.
	/// </summary>
	/// <param name="surfaceLocation"></param>
	/// <returns>Unit vector following direction of incoming light.</returns>
	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) = 0;

	virtual Eigen::Vector3f getLightLocation() = 0;

	Eigen::Vector3f getLightIntensity()
	{
		return _intensity;
	}

};

// =========== Subtask 1 ============
// Finish the definition of the AmbientLight class, and the DirectionalLight class
// below. 

/// <summary>
/// An ambient light is applied uniformly to all scene locations, having no direction
/// or location.
/// </summary>
class AmbientLight : public Light {
private:

public:
	AmbientLight(const Eigen::Vector3f& intensity)
		:Light(intensity)
	{};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		// This one should be quite easy - remember the intensity of an ambient
		// light is the same everywhere!
		// *** YOUR CODE HERE ***
		return _intensity;
		// *** END YOUR CODE ***
	}

	virtual Type getType() override
	{
		return Type::AMBIENT;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		// Ambient lights do not have a direction, so throw an error!
		// *** YOUR CODE HERE ***
		std::cout << "Ambient Lights dont have a direction " << std::endl;
		return Eigen::Vector3f::Zero();
		// *** END YOUR CODE ***
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		// Ambient lights do not have a location, so throw an error!
		// *** YOUR CODE HERE ***
		std::cout << "Ambient Lights dont have a location " << std::endl;
		return Eigen::Vector3f::Zero();
		// *** END YOUR CODE ***
	}
};

/// <summary>
/// Directional lights have a fixed direction and uniformly illuminate all world points
/// from that direction. They have no falloff (unlike point and spot lights).
/// </summary>
class DirectionalLight : public Light {
private:
	Eigen::Vector3f _direction;

public:
	DirectionalLight(const Eigen::Vector3f& intensity, const Eigen::Vector3f& direction)
		:Light(intensity), _direction(direction.normalized())
	{};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		// This one should also be quite easy - the intensity of an directional
		// light is also the same everywhere!
		// *** YOUR CODE HERE ***
		return _intensity;
		// *** END YOUR CODE ***
	}

	virtual Type getType() override
	{
		return Type::DIRECTIONAL;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		// Directional lights *do* have a direction, so return it!
		// *** YOUR CODE HERE ***
		return _direction;
		// *** END YOUR CODE ***
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		// Directional lights do not have a location, so throw an error!
		// *** YOUR CODE HERE ***
		std::cout << "Directional Lights dont have a location " << std::endl;
		return Eigen::Vector3f::Zero();
		// *** END YOUR CODE ***
	}
};	

// =========== Subtask 4 ============

/// <summary>
/// Point lights have a location in the world, and their intensity falls off with the 
/// inverse square law. Their direction is determined by the location of the surface
/// relative to the light.
/// </summary>
class PointLight : public Light {
private:
	Eigen::Vector3f _location;

public:
	PointLight(const Eigen::Vector3f& intensity, const Eigen::Vector3f& location)
		:Light(intensity), _location(location)
	{};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		// This is more complicated - here the location of the light and 
		// surface point do matter.
		// Work out how far away the light is from the surface point
		// Then multiply the intensity by 1/distance^2, following the inverse
		// square law.
		// *** YOUR CODE HERE ***
		float distance = (_location - surfaceLocation).norm();
		return _intensity / (distance * distance);
		// *** END YOUR CODE ***
	}

	virtual Type getType() override
	{
		return Type::POINT;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		// Again this is more complicated (but not too much)
		// Now we need to find a vector from the light location to
		// the surface location.
		// Don't forget to normalize it!
		// *** YOUR CODE HERE ***
		return (surfaceLocation - _location).normalized();
		// *** END YOUR CODE ***
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		// *** YOUR CODE HERE ***
		return _location;
		// *** END YOUR CODE ***
	}
};	

// =========== Optional Extra Subtask 5 ============

/// <summary>
/// Spot lights have a location in the world, and their intensity falls off with the 
/// inverse square law, just like point lights. However they also point in a specified
/// direction, and their intensity falls off as you move away from this direction.
/// This spot light implementation has a hard edge, and intensity drops to zero if the angle
/// between the surface direction and spot light direction is greater than _angle.
/// </summary>
class SpotLight : public Light {
private:
	Eigen::Vector3f _location;
	Eigen::Vector3f _direction;
	float _cosAngle;

public:
	SpotLight(const Eigen::Vector3f& intensity, const Eigen::Vector3f& location,
		const Eigen::Vector3f& direction, float angle)
		:Light(intensity), _location(location), _direction(direction), _cosAngle(cosf(angle))
	{};

	virtual Eigen::Vector3f getIntensityAt(const Eigen::Vector3f& surfaceLocation) override
	{
		// This is the trickiest one so far. Work out the direction from 
		// the light to the surface (normalize!)
		// Take the dot product with the light direction vector.
		// This tells you cos(theta) - if this is too low (less than cosAngle) 
		// you're outside the spotlight cone and can just return 0.
		// Otherwise, behave just like a point light!
		// *** YOUR CODE HERE ***
	
		Eigen::Vector3f direction = getDirection(surfaceLocation);
		float cosTheta = direction.dot(_direction);
		float distance = (_location - surfaceLocation).norm();
		
		if (cosTheta < _cosAngle)
		{
			return Eigen::Vector3f::Zero();
		}

		return _intensity / (distance * distance);
		// *** END YOUR CODE ***
	}

	virtual Type getType() override
	{
		return Type::SPOT;
	}

	virtual Eigen::Vector3f getDirection(const Eigen::Vector3f& surfaceLocation) override
	{
		// This will be just like your point light code.
		// *** YOUR CODE HERE ***
		return (surfaceLocation - _location).normalized();
		// *** END YOUR CODE ***
	}

	virtual Eigen::Vector3f getLightLocation() override
	{
		// *** YOUR CODE HERE ***
		return _location;
		// *** END YOUR CODE ***
	}
};	

