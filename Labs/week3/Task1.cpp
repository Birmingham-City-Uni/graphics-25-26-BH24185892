#include <iostream>
#include <lodepng.h>
#include <fstream>
#include <sstream>
#include "Vector3.hpp"
#include "Vector2.hpp"

// The goal for this lab is to draw a triangle mesh loaded from an OBJ file from scratch.
// This time, we'll draw the mesh as a solid object, rather than just a wireframe or collection
// of vertex points.
//
// *** Your Code Here! ***
// Task 1: As with last week, this is actually in the header files - this time we'll need both 2D and 3D vector classes!
//         Implement the dot and cross products in Vector2.hpp and Vector3.hpp so we can use them to draw our mesh.

void setPixel(std::vector<uint8_t>& image, int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	int pixelIdx = x + y * width;
	image[pixelIdx * 4 + 0] = r;
	image[pixelIdx * 4 + 1] = g;
	image[pixelIdx * 4 + 2] = b;
	image[pixelIdx * 4 + 3] = a;
}

// Task 2: Implement this barycentric-coordinate-based triangle drawing function, based on the algorithm
// described in the slides. I've broken down the steps involved here in comments added to the function.
void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	const Vector2& p0, const Vector2& p1, const Vector2& p2,
	uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	// Find a bounding box around the triangle
	// (that is, find minX, minY and maxX, maxY that are the min and max x and y-coordinates present in the triangle)
	// You can use the std::min and std::max functions if you wish.
	
	// YOUR CODE HERE
	int minX = 0, minY = 0, maxX = 0, maxY = 0;

	minX = std::min(p0.x(), std::min(p1.x(), p2.x()));

	minY = std::min(p0.y(), std::min(p1.y(), p2.y()));

	maxX = std::max(p0.x(), std::max(p1.x(), p2.x()));

	maxY = std::max(p0.y(), std::max(p1.y(), p2.y()));
	// Check your minX, minY, maxX and maxY values don't lie outside the image!
	// This would cause errors if you attempt to draw there.
	// That is, clamp these values so that 0 <= x < width and 0 <= y < height.
	if (maxY > height - 1) {
		maxY = height - 1;
	}
	if (maxY < 0) {
		maxY = 0;
	}

	if (maxX > width - 1) {
		maxX = width - 1;
	}
	if (maxX < 0) {
		maxX = 0;
	}

	if (minY > height - 1) {
		minY = height - 1;
	}
	if (minY < 0) {
		minY = 0;
	}

	if (minX > width - 1) {
		minX = width - 1;
	}
	if (minX < 0) {
		minX = 0;
	}
	// YOUR CODE HERE

	// Find vectors going along two edges of the triangle
	// from p0 to p1, and from p1 to p2.

	// YOUR CODE HERE
	Vector2 edge1, edge2;
	edge1 = p0 - p1;
	edge2 = p1 - p2;

	// Find the area of the triangle using a cross product.
	// Optional: You can use the sign of the cross product to see if this triangle is facing towards
	// or away from the camera. If you avoid drawing the triangles that face away, this may improve 
	// the quality of your render. (Note this optional feature is backface culling, one of the requirements
	// for your coursework!)

	// YOUR CODE HERE
	float triangleArea = 0.0f;

	// Now let's actually draw the triangle!
	// We'll do a for loop over all pixels in the bounding box.
	// Fill in the details in the for loop below.
	for(int x = minX; x <= maxX; ++x) 
		for (int y = minY; y <= maxY; ++y) {
			// Find the barycentric coordinates of the triangle!

			Vector2 p(x, y); // This is the 2D location of the pixel we are drawing.

			// Find the area of each of the three sub-triangles, using a cross product
			// YOUR CODE HERE - set the value of these three area variables.
			float a0;
			float a1;
			float a2;

			// Find the barycentrics b0, b1, and b2 by dividing by triangle area.
			// YOUR CODE HERE - do the division and find b0, b1, b2.
			float b0;
			float b1;
			float b2;

			// Check if the sum of b0, b1, b2 is bigger than 1 (or ideally a number just over 1 
			// to account for numerical error).
			// If it's bigger, skip to the next pixel as we are outside the triangle.
			// YOUR CODE HERE
			float sum;

			// Now we're sure we're inside the triangle, and we can draw this pixel!
			setPixel(image, x, y, width, height, r, g, b, a);
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

	// This line sets the memory block occupied by the image to all zeros.
	memset(&imageBuffer[0], 0, width * height * nChannels * sizeof(uint8_t));

	std::string bunnyFilename = "../models/stanford_bunny_simplified.obj";

	std::ifstream bunnyFile(bunnyFilename);

	std::vector<Vector3> vertices;
	std::vector<std::vector<unsigned int>> faces;
	std::string line;
	while (!bunnyFile.eof())
	{
		std::getline(bunnyFile, line);
		std::stringstream lineSS(line.c_str());
		char lineStart;
		lineSS >> lineStart;
		char ignoreChar;
		if (lineStart == 'v') {
			Vector3 v;
			for (int i = 0; i < 3; ++i) lineSS >> v[i];
			vertices.push_back(v);
		}

		if (lineStart == 'f') {
			std::vector<unsigned int> face;
			unsigned int idx, idxTex, idxNorm;
			while (lineSS >> idx >> ignoreChar >> idxTex >> ignoreChar >> idxNorm) {
				face.push_back(idx - 1);
			}
			if (face.size() > 0) faces.push_back(face);
		}
	}

	//drawTriangle(imageBuffer, width, height, Vector2(10, 10), Vector2(100, 10), Vector2(10, 100), 255, 0, 0, 255);

	for (const auto& face : faces) {
		Vector2 p0(vertices[face[0]].x() * 250 + width / 2, -vertices[face[0]].y() * 250 + height / 2);
		Vector2 p1(vertices[face[1]].x() * 250 + width / 2, -vertices[face[1]].y() * 250 + height / 2);
		Vector2 p2(vertices[face[2]].x() * 250 + width / 2, -vertices[face[2]].y() * 250 + height / 2);

		// Task 3: Draw the bunny!
		// Now you've finished your triangle drawing function, you'll see a red bunny, drawn using the code below:
		drawTriangle(imageBuffer, width, height, p0, p1, p2, 255, 0, 0, 255);

		// This is a bit boring. Try replacing this code to draw two different bunny types.

		// Bunny 1: Random Colour Bunny
		// Assign a random colour to each triangle in the bunny. You can do this by making use of 
		// the rand() function in C++.
		// Hint: Remember rand() returns an int, but we want our colour values to lie between 0 and 255.
		// How can we make sure our random r, g, b values stick to the right range?

		// Bunny 2: (Sort of) Diffuse Lighting Bunny
		// For the final task we'll do a bit of a preview of session 5 on diffuse lighting.
		// The idea here is that we colour each triangle according to how much it points towards the camera.
		// 
		// To do this, first find a vector pointing out at 90 degrees from the triangle, and normalise it
		// This special perpendicular vector is called the triangle's *normal*.
		//
		// Once you have your normal, take the dot product with (0,0,1). This will effectively measure how much
		// the normal points down the positive z-axis.
		// Use this value to set the brightness of the triangle (remember to scale it back to the [0,255] range).
	}


	// *** Encoding image data ***
	// PNG files are compressed to save storage space. 
	// The lodepng::encode function applies this compression to the image buffer and saves the result 
	// to the filename given.
	int errorCode;
	errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
