// This define is necessary to get the M_PI constant.
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"
#include "Mesh.hpp"

// ***** WEEK 5 LAB *****
// This week's lab builds on the work we did last week. We'll start from a slightly souped-up version
// of last week's code, and add the capability to compute *LIGHTING* for our scene.
// There are two main subtasks you need to complete here today.
// To find each one, you can use CTRL+SHIFT+F and search for the subtask name (e.g. "Subtask 1").
// Subtask 1: Finish the definition of the AmbientLight DirectionalLight classes in Light.hpp.
// Subtask 2: Finish the code here that works out the colour at this pixel.
// Subtask 3: Add some of these implemented lights to the scene, and see how they affect the image!
// Subtask 4: Go back and implement the PointLight class. Test this out (in the Subtask 3 section).
// Optional Extra Subtask 5: Implement the SpotLight class. Test this out (in the Subtask 3 section).

struct Triangle {
	std::array<Eigen::Vector2f, 3> screen; // Coordinates of the triangle in screen space.
	std::array<Eigen::Vector3f, 3> verts; // Vertices of the triangle in world space.
	std::array<Eigen::Vector3f, 3> norms; // Normals of the triangle corners in world space.
};


void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	const Eigen::Vector3f& albedo)
{
	// Find a bounding box around the triangle
	int minX, minY, maxX, maxY;
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	// Constrain it to lie within the image.
	minX = std::min(std::max(minX, 0), width);
	maxX = std::min(std::max(maxX, 0), width);
	minY = std::min(std::max(minY, 0), height);
	maxY = std::min(std::max(maxY, 0), height);

	Eigen::Vector2f edge1 = t.screen[2] - t.screen[0];
	Eigen::Vector2f edge2 = t.screen[1] - t.screen[0];
	float triangleArea = 0.5f * vec2Cross(edge2, edge1);
	if (triangleArea < 0) {
		// Triangle is backfacing
		// Exit and quit drawing!
		return;
	}

	for(int x = minX; x <= maxX; ++x) 
		for (int y = minY; y <= maxY; ++y) {
			Eigen::Vector2f p(x, y);

			// Find sub-triangle areas
			float a0 = 0.5f * fabsf(vec2Cross(t.screen[1] - t.screen[2], p - t.screen[2]));
			float a1 = 0.5f * fabsf(vec2Cross(t.screen[0] - t.screen[2], p - t.screen[2]));
			float a2 = 0.5f * fabsf(vec2Cross(t.screen[0] - t.screen[1], p - t.screen[1]));

			// find barycentrics
			float b0 = a0 / triangleArea;
			float b1 = a1 / triangleArea;
			float b2 = a2 / triangleArea;

			// If outside triangle, exit early
			float sum = b0 + b1 + b2;
			if (sum > 1.0001) {
				continue;
			}
			
			//========== Subtask 2 ==========

			// *** YOUR CODE HERE ***
			// Work out the world-space position and normal at this point on the triangle.
			// You can work this out using t.verts, t.norms and the barycentric coordinates.
			// HINT: Don't forget to re-normalise your norm afterwards!

			Eigen::Vector3f worldP = t.verts[0] * b0 + t.verts[1] * b1 + t.verts[2] * b2;
			Eigen::Vector3f normP = t.norms[0] * b0 + t.norms[1] * b1 + t.norms[2] * b2;
			normP.normalize();
			// *** END YOUR CODE ***

			// Work out colour at this position.
			Eigen::Vector3f color = Eigen::Vector3f::Zero(); 

			// Iterate over lights, and sum to find colour.
			for (auto& light : lights) {

				// *** YOUR CODE HERE ***
				// Work out the contribution from this light source, and add it to the color variable.
				// Comments and starter code are provided below to walk you through the steps involved.

				// Work out the intensity of this light source, at the point worldP.
				Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

				// We only need to do the following if the light isn't an ambient light.
				if (light->getType() != Light::Type::AMBIENT) {

					// Take the dot product of the normal with the light direction.
					// Be careful - the getDirection function returns the direction from
					// the light source to the surface.
					// You want the vector from the surface outward, so *negate* this vector
					// (i.e. use -direction, rather than direction).
					Eigen::Vector3f lightDir = -light->getDirection(worldP);
					
					float dotProd = lightDir.dot(normP);

					if (dotProd < 0) {
						dotProd = 0;
					}

					lightIntensity *= dotProd;
					// We don't want negative light - if your dot product was less than 0, set it to 0.

					// Multiply the light intensity by the dot product.
				}

				// Now add the intensity times the albedo.
				// You need to use a coefficient-wise multiply (not matrix multiply, dot product or cross product!)
				// There's a handy coeffWiseMultiply function I've written for you in LinAlg.hpp for this.
				color += coeffWiseMultiply(lightIntensity, albedo);
				// *** END YOUR CODE ***
			}

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(color.x(), 1/2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1/2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1/2.2f), 1.0f) * 255;

			c.a = 255;

			setPixel(image, x, y, width, height, c);
		}
}


void drawMesh(std::vector<unsigned char>& image, const Mesh& mesh, 
	const Eigen::Vector3f& albedo, const Eigen::Matrix4f& transform, 
	const std::vector<std::unique_ptr<Light>>& lights,
	int width, int height)
{
	for (int i = 0; i < mesh.vFaces.size(); ++i) {
		Eigen::Vector3f
			v0 = mesh.verts[mesh.vFaces[i][0]],
			v1 = mesh.verts[mesh.vFaces[i][1]],
			v2 = mesh.verts[mesh.vFaces[i][2]];
		Eigen::Vector3f
			n0 = mesh.norms[mesh.nFaces[i][0]],
			n1 = mesh.norms[mesh.nFaces[i][1]],
			n2 = mesh.norms[mesh.nFaces[i][2]];

		Triangle t;
		t.verts[0] = (transform * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.verts[1] = (transform * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.verts[2] = (transform * vec3ToVec4(v2)).block<3, 1>(0, 0);

		t.screen[0] = Eigen::Vector2f(t.verts[0].x() * 250 + width / 2, -t.verts[0].y() * 250 + height / 2);
		t.screen[1] = Eigen::Vector2f(t.verts[1].x() * 250 + width / 2, -t.verts[1].y() * 250 + height / 2);
		t.screen[2] = Eigen::Vector2f(t.verts[2].x() * 250 + width / 2, -t.verts[2].y() * 250 + height / 2);

		t.norms[0] = (transform.block<3, 3>(0, 0).inverse().transpose() * n0).normalized();
		t.norms[1] = (transform.block<3, 3>(0, 0).inverse().transpose() * n1).normalized();
		t.norms[2] = (transform.block<3, 3>(0, 0).inverse().transpose() * n2).normalized();

		drawTriangle(image, width, height, t, lights, albedo);
	}
}


int main()
{
	std::string outputFilename = "output.png";

	const int width = 512, height = 512;
	const int nChannels = 4;

	// Setting up an image buffer
	// This std::vector has one 8-bit value for each pixel in each row and column of the image, and
	// for each of the 4 channels (red, green, blue and alpha).
	// Remember 8-bit unsigned values can range from 0 to 255.
	std::vector<uint8_t> imageBuffer(height*width*nChannels);

	// This line sets the image to black initially.
	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
		}
	}

	std::string bunnyFilename = "../models/stanford_bunny_simplified.obj";
	std::string dragonFilename = "../models/stanford_chinese_dragon_simplified.obj";

	std::vector<std::unique_ptr<Light>> lights;
	// I've already added an ambient light for you!
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.1f, 0.1f, 0.1f)));

	// ============ Subtask 3 ===========
	// Add some more lights to the scene! Use emplace_back as above.
	// Start with a DirectionalLight. For starters, try an intensity of 
	// (0.4, 0.4, 0.4) and direction of (1,0,0).
	// This will give you a similar result to example.png.
	// Then, play with other directions, and try adding colour to your light
	// by changing the intensity.
	// Come back here once you've implemented the point and spot lights and
	// add some of these too!

	// *** YOUR CODE HERE ***
	//lights.emplace_back(new PointLight(Eigen::Vector3f(1.1f, 1.1f, 1.1f), Eigen::Vector3f(0.f, 1.0f, 0.f)));
	//lights.emplace_back(new DirectionalLight(Eigen::Vector3f(0.4f, 0.4f, 0.4f), Eigen::Vector3f(1.f, 0.f, 0.0f)));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(0.0f, 5.0f, 10.0f), Eigen::Vector3f(-0.5f, 1.f, 0.0f), Eigen::Vector3f(0, -1, 0), M_PI/5));
	lights.emplace_back(new SpotLight(Eigen::Vector3f(10.0f, 5.0f, 0.0f), Eigen::Vector3f(1.f, 1.f, 0.0f), Eigen::Vector3f(-1, -1, 0), M_PI / 8));
	// *** END YOUR CODE ***



	Mesh bunnyMesh = loadMeshFile(bunnyFilename);
	Mesh dragonMesh = loadMeshFile(dragonFilename);


	Eigen::Matrix4f bunnyTransform = translationMatrix(Eigen::Vector3f(-0.5f, -0.5f, 0.f)) * rotateYMatrix(M_PI);

	Eigen::Matrix4f dragonTransform =
		translationMatrix(Eigen::Vector3f(0.3f, 0.1f, 0.0f))
		* scaleMatrix(1.2f) * rotateYMatrix(M_PI);

	drawMesh(imageBuffer, bunnyMesh, Eigen::Vector3f(0.3, 1, 0.3), bunnyTransform, lights, width, height);
	drawMesh(imageBuffer, dragonMesh, Eigen::Vector3f(0.3, 1, 1), dragonTransform, lights, width, height);

	// For debug - draw point lights as colored circles so we can see where they are
	for (auto& light : lights) {
		if (light->getType() == Light::Type::POINT || light->getType() == Light::Type::SPOT) {
			Eigen::Vector3f intensityNorm = light->getLightIntensity();
			intensityNorm /= std::max(intensityNorm.x(), std::max(intensityNorm.y(), intensityNorm.z()));
			Color lightColor{
				intensityNorm.x() * 255,
				intensityNorm.y() * 255,
				intensityNorm.z() * 255,
				255
			};
			Eigen::Vector3f lightPt = light->getLightLocation();
			Eigen::Vector2f screenPt(lightPt.x() * 250 + width / 2, -lightPt.y() * 250 + height / 2);
			drawCircle(imageBuffer, width, height, screenPt.y(), screenPt.x(), 10, lightColor);
		}
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
