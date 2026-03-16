// This define is necessary to get the M_PI constant.
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <lodepng.h>
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"
#include "Mesh.hpp"

// ***** WEEK 6 LAB *****
// Subtask 1: Implement the projectionMatrix function, to make a projection matrix to view your scene!
// Subtask 2: Complete the transformation chain, moving vertices from model space all the way to screen space.
// Subtask 3: Set up the camera and projection matrices for the transformation chain
// Subtask 4: Implement Z buffering.
// Subtask 5: Implement texture mapping.
// If you finish early - note that we now have all the tools to properly set up your own scene!
// This is a great time to start on your own code in the coursework/rasteriser folder, using this as a base if
// you wish. We will in future labs work on more advanced shading, but you can port this feature over later.


struct Triangle {
	std::array<Eigen::Vector3f, 3> screen; // Coordinates of the triangle in screen space.
	std::array<Eigen::Vector3f, 3> verts; // Vertices of the triangle in world space.
	std::array<Eigen::Vector3f, 3> norms; // Normals of the triangle corners in world space.
	std::array<Eigen::Vector2f, 3> texs; // Texture coordinates of the triangle corners.
};


Eigen::Matrix4f projectionMatrix(int height, int width, float horzFov = 70.f*M_PI/180.f, float zFar = 10.f, float zNear = 0.1f)
{
	// ========= Subtask 1: Make a Projection Matrix ========
	// *** YOUR CODE HERE ***

	// Make a projection matrix following the formulation in the lecture slides, and using the provided parameters.
	// First, work out vertical FoV based on the horizontal FoV:
	float vertFov = horzFov * float(height) / width;;
	// Now construct the matrix.
	Eigen::Matrix4f projection;
	projection <<
		1.0f / tanf(0.5f * horzFov), 0, 0, 0,
		0, 1.0f / tanf(0.5f * vertFov), 0, 0,
		0, 0, zFar / (zFar - zNear), -zFar * zNear / (zFar - zNear),
		0, 0, 1, 0;
	return projection;
	// *** END YOUR CODE ***
}

void findScreenBoundingBox(const Triangle& t, int width, int height, int& minX, int& minY, int& maxX, int& maxY)
{
	// Find a bounding box around the triangle
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	// Constrain it to lie within the image.
	minX = std::min(std::max(minX, 0), width-1);
	maxX = std::min(std::max(maxX, 0), width-1);
	minY = std::min(std::max(minY, 0), height-1);
	maxY = std::min(std::max(maxY, 0), height-1);
}


void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	std::vector<float>& zBuffer,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight)
{
	int minX, minY, maxX, maxY;
	findScreenBoundingBox(t, width, height, minX, minY, maxX, maxY);

	Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
	Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
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
			float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

			// find barycentrics
			float b0 = a0 / triangleArea;
			float b1 = a1 / triangleArea;
			float b2 = a2 / triangleArea;

			// If outside triangle, exit early
			float sum = b0 + b1 + b2;
			if (sum > 1.0001) {
				continue;
			}
			
			Eigen::Vector3f worldP = t.verts[0] * b0 + t.verts[1] * b1 + t.verts[2] * b2;

			// ========== Subtask 4: Z Buffering ==========
			// Here we'll implement Z-buffering, using the zBuffer image and working out the 
			// depth of this pixel in screen space.
			// HINT: If you have trouble with this task, note that I've added code to save the z buffer to
			// zBuffer.png. This is encoded so further away objects are lighter in color. It should match
			// the example_zBuffer.png image if your code is working!
			// *** YOUR CODE HERE ***

			// First, work out the depth of this location in screen space. 
			// We saved the clip space z values in t.screen[0].z(), t.screen[1].z() and t.screen[2].z.
			// Use barycentric interpolation on these to work out the depth of this pixel.
			float depth = t.screen[0].z() * b0 + t.screen[1].z() * b1 + t.screen[2].z() * b2;

			// Work out where to sample in the zBuffer. Remember the zBuffer has only one channel,
			// so your index should be based on the pixel's x and y locations, and the width of the 
			// z buffer only.
			int depthIdx = static_cast<int>(p.x()) + static_cast<int>(p.y()) * width;

			// If your depth is bigger than the current depth, skip drawing this pixel.
			// Otherwise, replace the zBuffer value at depthIdx with this depth.
			// ADD YOUR OWN CODE TO DO THIS HERE
			if (depth > zBuffer[depthIdx]) continue;
			zBuffer[depthIdx] = depth;

			// *** END YOUR CODE ***

			Eigen::Vector3f normP = t.norms[0] * b0 + t.norms[1] * b1 + t.norms[2] * b2;
			normP.normalize();



			// ========== Subtask 5: Texture Mapping ===========
			// Here we'll actually implement the texture mapping! Follow the steps below, implementing each
			// stage in turn.
			// *** YOUR CODE HERE ***
			// Add code to calculate the texture coordinates corresponding to P, texP.
			// Use barycentric interpolation!
			Eigen::Vector2f texP = t.texs[0] * b0 + t.texs[1] * b1 + t.texs[2] * b2;

			// Convert this coordinate to a point in texture space
			// To do so, multiply by the texWidth and texHeight to get to the correct range.
			// Don't forget to flip the y coordinates! 
			int texR = (texP.x() * texWidth);;
			int texC = ((1.0f - texP.y()) * texHeight);
			// Handle the case where texR or texC end up outside the image!
			// There are different ways you could do this - for example using 
			// the modulo (%) operator to wrap around, or clamping to the edges.
			// Write your own code below to do this - once you're done you should be sure 
			// that 0 <= texC < texWidth and 0 <= texR < texHeight.
			texC = std::max(0, std::min(texC, texWidth - 1));
			texR = std::max(0, std::min(texR, texHeight - 1));

			// Get the value from the texture (hint: use the getPixel function on the albedoTexture).
			Color texColor = getPixel(albedoTexture, texC, texR, texWidth, texHeight);;

			// Convert it into an Eigen::Vector3f as an albedo
			// (Optional bonus task, if you checked out the slides on gamma correction:
			// gamma correct this colour, so the texture doesn't appear overly bright.
			// should you raise to the power 1/2.2, or 2.2?)
			Eigen::Vector3f albedo(powf(texColor.r / 255.0f, 2.2f), powf(texColor.g / 255.0f, 2.2f), powf(texColor.b / 255.0f, 2.2f));
			// *** END YOUR CODE ***


			// ----- Lighting code ------
			// Work out colour at this position.
			Eigen::Vector3f color = Eigen::Vector3f::Zero();

			// Iterate over lights, and sum to find colour.
			for (auto& light : lights) {

				// Work out the contribution from this light source, and add it to the color variable.

				// Work out the intensity of this light source, at the point worldP.
				Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

				// We only need to do the following if the light isn't an ambient light.
				if (light->getType() != Light::Type::AMBIENT) {

					// Take the dot product of the normal with the light direction.
					float dotProd = normP.dot(-light->getDirection(worldP));

					// We don't want negative light - if dot product less than 0, set it to 0.
					dotProd = std::max(dotProd, 0.0f);

					// Multiply the light intensity by the dot product.
					lightIntensity *= dotProd;
				}

				// Now add the intensity times the albedo.
				color += coeffWiseMultiply(lightIntensity, albedo);
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



void drawMesh(std::vector<unsigned char>& image,
	std::vector<float>& zBuffer,
	const Mesh& mesh, 
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight,
	const Eigen::Matrix4f& modelToWorld, 
	const Eigen::Matrix4f& worldToClip, 
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
		t.verts[0] = (modelToWorld * vec3ToVec4(v0)).block<3, 1>(0, 0);
		t.verts[1] = (modelToWorld * vec3ToVec4(v1)).block<3, 1>(0, 0);
		t.verts[2] = (modelToWorld * vec3ToVec4(v2)).block<3, 1>(0, 0);

		// ======= Subtask 2: The Transformation Chain ======
		//*** YOUR CODE HERE ***
		// We've worked out the vertices in *world* space above.
		// You need to do the rest of the transformation chain!
		// Work out the vClip vectors, which are the vectors in clip space
		// Multiply by worldToClip, and do the perspective divide by the w component.
		// Check that all 3 vertices are in the clip box (-1 to 1 in x, y and z) and if not,
		// skip drawing this triangle.
		// Hint: use the outsideClipBox function to do this.
		// Finally, work out the screen space coordinates based on the image height and width.

		// Work out the clip space coordinates, by multiplying by worldToClip and doing the 
		// perspective divide.
		Eigen::Vector4f vClip0 = worldToClip * modelToWorld * vec3ToVec4(v0);
		vClip0 /= vClip0.w();
		Eigen::Vector4f vClip1 = worldToClip * modelToWorld * vec3ToVec4(v1);
		vClip1 /= vClip1.w();
		Eigen::Vector4f vClip2 = worldToClip * modelToWorld * vec3ToVec4(v2);
		vClip2 /= vClip2.w();



		// Check that all 3 vertices are in the clip box (-1 to 1 in x, y and z) and if not,
		// skip drawing this triangle.
		// Hint: I've made a function outsideClipBox in LinAlg.hpp to help with this!
		if (outsideClipBox(vClip0) || outsideClipBox(vClip1) || outsideClipBox(vClip2)) continue;

		// Work out the screen space coordinates based on the image height and width.
		// Set the z component of each screen coordinate to be the clip-space z (for example
		// t.screen[0].z() == vClip0.z());
		t.screen[0] = Eigen::Vector3f((vClip0.x() + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
		t.screen[1] = Eigen::Vector3f((vClip1.x() + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
		t.screen[2] = Eigen::Vector3f((vClip2.x() + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());
	
		// *** END YOUR CODE ***

		// transform the normals (using the inverse transpose of the upper 3x3 block)
		t.norms[0] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n0).normalized();
		t.norms[1] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n1).normalized();
		t.norms[2] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n2).normalized();

		t.texs[0] = mesh.texs[mesh.tFaces[i][0]];
		t.texs[1] = mesh.texs[mesh.tFaces[i][1]];
		t.texs[2] = mesh.texs[mesh.tFaces[i][2]];

		drawTriangle(image, width, height, zBuffer, t, lights, albedoTexture, texWidth, texHeight);
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
	std::vector<float> zBuffer(height * width);

	// This line sets the image to black initially.
	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
			zBuffer[r * width + c] = 1.0f;
		}
	}

	// ========== Subtask 3: Camera Matrices ========

	// *** YOUR CODE HERE ***
	// This makes the projection matrix, using the function you implemented. Once the code is working,
	// try changing the FoV!
	Eigen::Matrix4f projection = projectionMatrix(height, width);

	// This matrix rotates the camera, tilting it down, then translates it up to make it look down on the scene.
	// Once your code is working, try changing this to move the camera around!
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.f, 0.8f, 0.f)) * rotateXMatrix(0.4f);

	Eigen::Vector3f cameraWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);

	// The main important task = set up the worldToCamera and worldToClip matrices here!
	// Set up worldToCamera, based on cameraToWorld above
	Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();
	// Set up worldToClip, using the projection and worldToCamera matrices
	Eigen::Matrix4f worldToClip = projection * worldToCamera;

	// *** END YOUR CODE ***

	std::string bunnyFilename = "../models/stanford_bunny_texmapped.obj";

	std::vector<std::unique_ptr<Light>> lights;
	// I've already added an ambient light for you!
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.1f, 0.1f, 0.1f)));

	//lights.emplace_back(new PointLight(Eigen::Vector3f(1.1f, 1.1f, 1.1f), Eigen::Vector3f(0.f, 1.0f, 0.f)));
	lights.emplace_back(new DirectionalLight(Eigen::Vector3f(0.4f, 0.4f, 0.4f), Eigen::Vector3f(1.f, 0.f, 0.0f)));
	//lights.emplace_back(new SpotLight(Eigen::Vector3f(10.0f, 0.0f, 0.0f), Eigen::Vector3f(0.f, 1.f, 0.0f), Eigen::Vector3f(0, -1, 0), M_PI/8));

	Mesh bunnyMesh = loadMeshFile(bunnyFilename);


	Eigen::Matrix4f bunnyTransform; 

	std::vector<uint8_t> bunnyTexture;
	unsigned int bunnyTexWidth, bunnyTexHeight;
	lodepng::decode(bunnyTexture, bunnyTexWidth, bunnyTexHeight, "../models/stanford_bunny_albedo.png");

	bunnyTransform = translationMatrix(Eigen::Vector3f(-1.0f, -1.0f, 3.f)) * rotateYMatrix(M_PI);
	drawMesh(imageBuffer, zBuffer, bunnyMesh, bunnyTexture, bunnyTexWidth, bunnyTexHeight, bunnyTransform, worldToClip, lights, width, height);
	bunnyTransform = translationMatrix(Eigen::Vector3f(-1.0f, -1.0f, 5.f)) * rotateYMatrix(M_PI);
	drawMesh(imageBuffer, zBuffer, bunnyMesh, bunnyTexture, bunnyTexWidth, bunnyTexHeight, bunnyTransform, worldToClip, lights, width, height);
	bunnyTransform = translationMatrix(Eigen::Vector3f(-1.0f, -1.0f, 7.f)) * rotateYMatrix(M_PI);
	drawMesh(imageBuffer, zBuffer, bunnyMesh, bunnyTexture, bunnyTexWidth, bunnyTexHeight, bunnyTransform, worldToClip, lights, width, height);
	bunnyTransform = translationMatrix(Eigen::Vector3f(1.0f, -1.0f, 3.f)) * rotateYMatrix(M_PI);
	drawMesh(imageBuffer, zBuffer, bunnyMesh, bunnyTexture, bunnyTexWidth, bunnyTexHeight, bunnyTransform, worldToClip, lights, width, height);
	bunnyTransform = translationMatrix(Eigen::Vector3f(1.0f, -1.0f, 5.f)) * rotateYMatrix(M_PI);
	drawMesh(imageBuffer, zBuffer, bunnyMesh, bunnyTexture, bunnyTexWidth, bunnyTexHeight, bunnyTransform, worldToClip, lights, width, height);
	bunnyTransform = translationMatrix(Eigen::Vector3f(1.0f, -1.0f, 7.f)) * rotateYMatrix(M_PI);
	drawMesh(imageBuffer, zBuffer, bunnyMesh, bunnyTexture, bunnyTexWidth, bunnyTexHeight, bunnyTransform, worldToClip, lights, width, height);

	// For debug - draw point lights as colored circles so we can see where they are
	drawPointLights(imageBuffer, width, height, lights);

	// Save the image to png.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	saveZBufferImage("zBuffer.png", zBuffer, width, height);

	return 0;
}
