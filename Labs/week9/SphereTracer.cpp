// This define is necessary to get the M_PI constant.
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"

// =========== Week 9 Lab ==============
// Let's make our ultimate Sphere tracer!
//
// Task 1: Add Ray-Sphere intersection
// Task 2: Find the sphere normal
// Task 3: Try raytracing your scene - you should see your first raytraced spheres!
// Task 4: Add mirror reflection
// Task 5: Add a mirror reflective sphere to your scene, and raytrace again!
// Task 6: Add refraction
// Task 7: Add a refractive sphere to your scene, and raytrace again!
// Task 8: Look at the sphere intersection testing code here to prep for task 9.
// Task 9: Add shadow testing, trace and check it works!
//
// At the end, you should have a result similar to example.png (assuming you haven't changed
// the scene setup).
// 
// Bonus Task: Add (Blinn)-Phong shading as an extra material type!
//             (no code assistance for this one, but I recommend adding an extra Material type and implementing
//              in the traceRay function).

// For this week - just as a reminder if you prefer you can use the Eigen namespace.
// This saves some typing, but is generally a bit frowned on as it can cause name conflicts.
// The "using" keyword is less frowned upon in source (.cpp) files, but using it in .hpp files 
// is generally a really bad idea (it can lead to hard-to-find bugs very easily).
using namespace Eigen;

// ***** Important Constants *****

// This colour is the default one used if e.g. we don't hit anything,
// or we exceed the maximum bounce limit.
const Vector3f ambientColour(0.1f, 0.1f, 0.1f); 

// Maximum number of bounces (includes both reflection and refraction).
// This limit ensures we don't carry on bouncing forever!
const int maxBounces = 5;

struct Ray {
	Vector3f origin, direction;
};

enum Material {
	DIFFUSE, MIRROR, REFRACTIVE
};

struct Sphere {
	// Geometric properties
	Vector3f centre;
	float radius;

	// Material properties
	Material material;
	Vector3f colour;
	float ior; // Index of refraction (only used for refractive spheres).
};

struct Camera {
	Vector3f position, direction, up;
	float horzFov;
};


bool raySphereIntersection(const Ray& ray, const Sphere& sphere, Vector3f& intersection, float& t, float minT=0.001f)
{
	// Task 1: Add Ray-Sphere intersection
	// *** YOUR CODE HERE ***
	// Find the intersection point between the ray and the sphere. 
	// If an intersection exists, set the value of "intersection" and "t", and return true!
	// If no intersection is found, or the value of t is below minT, return false.

	// Steps:
	// 1. Find the value of A, B and C from the lecture slides.
	Vector3f oc = ray.origin - sphere.centre;
	float A = ray.direction.dot(ray.direction);
	float B = 2.0f * oc.dot(ray.direction);
	float C = oc.dot(oc) - sphere.radius * sphere.radius;
	
	// 2. Find the value of the discriminant B^2 - 4AC
	float discriminant = B * B - 4.0f * A * C;
	
	// 3. If the discriminant is less than 0, return false (no solutions).
	if (discriminant < 0.0f) {
		return false;
	}
	
	// 4. Otherwise, find the two solutions for t (t1 and t2, for example).
	float sqrtDiscriminant = sqrtf(discriminant);
	float t1 = (-B - sqrtDiscriminant) / (2.0f * A);
	float t2 = (-B + sqrtDiscriminant) / (2.0f * A);
	
	// 5. Find the smallest solution for t that's bigger than minT.
	//   a. If such a t exists, set the value of "intersection" and "t" and return true.
	//   b. If no such t exists, return false.
	float tResult;
	if (t1 > minT) {
		tResult = t1;
	} else if (t2 > minT) {
		tResult = t2;
	} else {
		return false;
	}
	
	t = tResult;
	intersection = ray.origin + ray.direction * t;
	return true;
	// *** END YOUR CODE ***
} 

Vector3f getSphereNormal(const Sphere& sphere, const Vector3f& location) {
	// Task 2: Find the sphere normal
	// *** YOUR CODE HERE ***
	// Find the value of the normal to the sphere at the given location.
	// This should only need one line of code!
	// See the slides for more detail.
	// 
	// Remove this existing code that just returns 0.
	return (location - sphere.centre).normalized();
	// *** END YOUR CODE ***
}

bool refract(const Vector3f& incident, const Vector3f& norm, float eta, Vector3f& refracted)
{
	// *** YOUR CODE HERE ***
	// Find the refracted ray! Set the refracted variable to equal the refracted direction.
	// If a ray is refracted, return true. If Total Internal Reflection (TIR) occurs, return false.
	// If you return false you don't need to set the value of "refracted"!


	// Steps:
	// 1. Find the value of the "k" from the lecture slides.
	float cosI = -norm.dot(incident);
	float k = 1.0f - eta * eta * (1.0f - cosI * cosI);
	
	// 2. If k < 0, return false (TIR occurs).
	if (k < 0.0f) {
		return false;
	}
	
	// 3. Otherwise, find the refracted ray and return true.
	refracted = eta * incident + (eta * cosI - sqrtf(k)) * norm;
	return true;
	// *** END YOUR CODE
}

Vector3f traceRay(const Ray& ray, const std::vector<Sphere>& spheres, const std::vector<std::unique_ptr<Light>>& lights, int bounce=0)
{
	// First, if we get to too many bounces, we need to exit and do something reasonable 
	// I've chosen to return the ambient default colour.
	if (bounce > maxBounces) return ambientColour;

	// Task 8: Look at the sphere intersection testing code here to prep for task 9.
	// Keep track of the minimum t thus far,
	// where our closest hit location is,
	// and the closest sphere we've hit.
	float minHitT = FLT_MAX;
	Vector3f hitIntersection;
	const Sphere* hitSphere = nullptr;

	// Intersect each sphere, and if the hit is closer and valid,
	// update the closest hit info.
	for (const Sphere& sphere : spheres) {
		float t;
		Vector3f intersection;
		if (raySphereIntersection(ray, sphere, intersection, t)) {
			if (t < minHitT) {
				minHitT = t;
				hitSphere = &sphere;
				hitIntersection = intersection;
			}
		}
	}

	// If we didn't hit anything, exit early and return the default colour.
	if (!hitSphere) {
		return ambientColour;
	}

	// If we hit a diffuse material, do lighting calculations!
	// These are currently pretty much what we did for rasterisation before - do the dot product,
	// and a coefft-wise product with the albedo.
	else if (hitSphere->material == Material::DIFFUSE) {
		Vector3f color = Vector3f::Zero();

		for (const auto& light : lights) {
			if (light->getType() == Light::AMBIENT) {
				// Ambient lighting.
				// No need for a shadow test here!
				color += coeffWiseMultiply(hitSphere->colour, light->getLightIntensity());
			}
			else {
				Vector3f lightDir = light->getDirection(hitIntersection);
				// shadow test
				bool inShadow = false;

				// Task 9: Add shadow testing, trace and check it works!
				// *** YOUR CODE HERE ***
				// Add shadow testing to cast pixel-perfect shadows!
				// I've found the direction from the light to the surface, and 
				// set up an inShadow variable above. You should set this inShadow to true
				// if you find a sphere is blocking the light from the surface.

				// The overall code will be similar to the sphere intersection code above, with a few 
				// changes:
				// 1. You don't need to keep track of the closest hit this time - the minute you find
				//    a blocking object, exit the for loop early and set inShadow to true.
				// 2. If the light is DIRECTIONAL, distance doesn't matter. If it's a POINT or SPOT light, 
				//    things should only block the light source if the hit location is closer than the light.
				// 3. Optional: should REFRACTIVE spheres cast shadows? Maybe ignore hits from REFRACTIVE spheres.
				//              If you want to be really fancy, you could coefft-wise multiply by the colour of
				//              the refractive sphere.

				// Steps:
				// 1. Construct a shadow ray, from the hit location, pointing towards the light.
				Ray shadowRay;
				shadowRay.origin = hitIntersection;
				shadowRay.direction = -lightDir;

				// 2. For loop over all the spheres, testing for intersection.
				//		a. If there is a hit, check the sphere type isn't REFRACTIVE
				//		b. If the light is DIRECTIONAL, the point is definitely in shadow
				//      c. If it's not, compare the value of t to the distance from hitIntersection to the light
				//			the point is only in shadow if the value of t is less than this distance.
				for (const Sphere& sphere : spheres) {
					float t;
					Vector3f intersection;

					if (raySphereIntersection(shadowRay, sphere, intersection, t)) {
						if (sphere.material == Material::REFRACTIVE) {
							continue;
						}

						if (light->getType() == Light::DIRECTIONAL) {
							inShadow = true;
							break;
						}

						float lightDistance = (light->getLightLocation() - hitIntersection).norm();
						if (t < lightDistance) {
							inShadow = true;
							break;
						}
					}
				}
				// *** END YOUR CODE ***

				// If we're in shadow, this light source doesn't contribute to the colour so continue to the next.
				if (inShadow) continue;

				// Normal diffuse lighting calculations
				float dotProd = -lightDir.dot(getSphereNormal(*hitSphere, hitIntersection));
				dotProd = std::max(dotProd, 0.f);
				Vector3f reflectance = hitSphere->colour * dotProd;
				color += coeffWiseMultiply(reflectance, light->getIntensityAt(hitIntersection));
			}
		}
		return color;
	}
	else if (hitSphere->material == Material::MIRROR) {
		// Task 4: Add mirror reflection
		// *** YOUR CODE HERE ***
		// 1. Find the reflected ray, and call traceRay again recursively
		// 2. Return the resulting colour.
		// Optional extra task: coefft-wise multiply this returned colour with this 
		// sphere's own colour. This will allow you to simulate coloured mirrors.
		// REMINDER: don't forget to increase the value of bounce by 1 when you call traceRay
		// again recursively! This will make sure you don't exceed the maxBounces bounce count.

		// Calculate the surface normal at the hit point
		Vector3f normal = getSphereNormal(*hitSphere, hitIntersection);
		
		// Calculate the reflected direction using: R = I - 2(N·I)N
		Vector3f reflectedDir = ray.direction - 2.0f * ray.direction.dot(normal) * normal;
		reflectedDir.normalize();
		
		// Create the reflected ray
		Ray reflectedRay;
		reflectedRay.origin = hitIntersection;
		reflectedRay.direction = reflectedDir;
		
		// Trace the reflected ray recursively
		Vector3f reflectedColor = traceRay(reflectedRay, spheres, lights, bounce + 1);
		
		// Optional: multiply by sphere's color for colored mirrors
		return coeffWiseMultiply(reflectedColor, hitSphere->colour);
		//*** END YOUR CODE
	}
	else if (hitSphere->material == Material::REFRACTIVE) {
		// I've handled a few fiddly bits of the refraction code for you here:
		float eta; // This is n1/n2, the ratio of IORs.
		Vector3f normal = getSphereNormal(*hitSphere, hitIntersection);

		bool enteringSphere = ray.direction.dot(normal) < 0;

		if (enteringSphere) eta = 1.f / hitSphere->ior;

		else eta = hitSphere->ior;

		if (!enteringSphere) normal = -normal;

		// Task 6: Add refraction
		// *** YOUR CODE HERE ***

		// Remove this line when you've implemented refraction!

		// Handle refraction, and total internal reflection!
		Vector3f refractedDir;
		// Steps:
		// 1. Try to refract the incoming ray in the normal, using the value of eta calculated above.
		if (refract(ray.direction, normal, eta, refractedDir))
		{
			// 2. If refract returns true:
			//		a. Construct a refracted ray
			Ray refractedRay;
			refractedRay.origin = hitIntersection;
			refractedRay.direction = refractedDir;
			//      b. Call traceRay to find the colour. Don't forget to use bounce+1!
			Vector3f refractedColor = traceRay(refractedRay, spheres, lights, bounce + 1);
			//      c. Optional: Coefft-wise multiply the result with this hit sphere's colour (this will allow you to 
			//         make coloured glass).
			return coeffWiseMultiply(refractedColor, hitSphere->colour);
		}
		else {
			// 3. If refract returns false:
			//      a. Total Internal Reflection has occured!
			//      b. Find the reflected direction, and make a reflected ray.
			Vector3f reflectedDir = ray.direction - 2.0f * ray.direction.dot(normal) * normal;
			reflectedDir.normalize();

			Ray reflectedRay;
			reflectedRay.origin = hitIntersection;
			reflectedRay.direction = reflectedDir;
			//      c. Trace the reflected ray. Again, make sure to use bounce+1!
			return traceRay(reflectedRay, spheres, lights, bounce + 1);
		}
		// *** END YOUR CODE ***
	}
}

int main()
{
	std::string outputFilename = "output.png";

	const int width = 512, height = 512;
	const int nChannels = 4;

	std::vector<uint8_t> imageBuffer(height*width*nChannels);

	// This line sets the image to black initially.
	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
		}
	}

	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new AmbientLight(Vector3f(0.1f, 0.1f, 0.1f)));
	lights.emplace_back(new DirectionalLight(Vector3f(0.4f, 0.4f, 0.4f), Vector3f(1.f, -1.f, 0.0f)));

	// This code sets up a scene with a bunch of spheres.
	// I've initially set up the scene with diffuse spheres only. Once you've implemented 
	// MIRROR and REFRACTIVE spheres, you can uncomment these or add your own!
	std::vector<Sphere> spheres;
	spheres.push_back({ Vector3f(2.f, 0.f, 4.f), 1.f, Material::DIFFUSE, Vector3f(0.f, 0.8f, 0.8f) });
	spheres.push_back({ Vector3f(-2.f, 0.f, 4.f), 0.5f, Material::DIFFUSE, Vector3f(0.8f, 0.f, 0.8f) });
	spheres.push_back({ Vector3f(0.f, 2.f, 4.f), 0.5f, Material::DIFFUSE, Vector3f(0.8f, 0.8f, 0.f) });
	spheres.push_back({ Vector3f(0.f, -2.f, 4.f), 0.5f, Material::DIFFUSE, Vector3f(0.2f, 0.2f, 0.8f) });
	spheres.push_back({ Vector3f(0.f, 1.f, 6.f), 0.3f, Material::DIFFUSE, Vector3f(0.8f, 0.8f, 0.f) });
	// Task 5: Add a mirror reflective sphere to your scene, and raytrace again!
	spheres.push_back({ Vector3f(2.f, 2.f, 4.f), 0.5f, Material::MIRROR, Vector3f(0.9f, 0.9f, 0.9f) });
	
	// Task 7: Add a refractive sphere to your scene, and raytrace again!
	spheres.push_back({ Vector3f(0.f, 0.f, 3.f), 0.5f, Material::REFRACTIVE, Vector3f(0.9f, 0.8f, 0.8f), 1.4f });

	Camera camera{
		Vector3f(0.f, 0.f, 0.f), // position
		Vector3f(0.f, 0.f, 1.f), // direction
		Vector3f(0.f, 1.f, 0.f), // up
		M_PI_2 // horzFov
	};

	float vertFov = (camera.horzFov * height) / width;
	float minX = -atanf(camera.horzFov * 0.5f);
	float xStep = -2.f * minX / width;
	float minY = -atanf(vertFov * 0.5f);
	float yStep = -2.f * minY / width;

	Vector3f across = -camera.direction.cross(camera.up).normalized();

	for (int x = 0; x < width; ++x)
		for (int y = 0; y < height; ++y) {
			Ray ray;
			ray.origin = camera.position;
			ray.direction = camera.direction + minX * across + minY * camera.up;
			ray.direction += across * x * xStep;
			ray.direction += camera.up * y * yStep;
			ray.direction.normalize();

			//ray.origin = Vector3f::Zero();
			//ray.direction = Vector3f(0.f, 0.f, 1.f);

			Vector3f color = traceRay(ray, spheres, lights);

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(color.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1 / 2.2f), 1.0f) * 255;
			c.a = 255;
			setPixel(imageBuffer, x, height-y-1, width, height, c);
		}
	


	// Save the image to png.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
