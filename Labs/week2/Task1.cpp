#include <iostream>
#include <lodepng.h>
#include <fstream>
#include <sstream>
#include "Vector3.hpp"

// The goal for this lab is to draw a triangle mesh loaded from an OBJ file from scratch,
// building on the image drawing code from last week's lab.
// The mesh consists of a list of 3D vertices, that describe the points forming the mesh.
// It also has a list of triangle indices, that determine which vertices are used to form each triangle.
// For now, we will just try to draw all the vertices as points, ignoring the indices.

// *** Your Code Here! ***
// * Task 1: Your first task is actually in the Vector3.hpp file.
//           I've started off a class here to store and manipulate 3D floating-point vectors.
//           We'll need to use this to load and draw the triangle mesh from the OBJ file.
//           Switch over to Vector3.hpp in the Header Files section of this project, and 
//           complete writing the class.

void setPixel(std::vector<uint8_t>& image, int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	int pixelIdx = x + y * width;
	image[pixelIdx * 4 + 0] = r;
	image[pixelIdx * 4 + 1] = g;
	image[pixelIdx * 4 + 2] = b;
	image[pixelIdx * 4 + 3] = a;
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

	// *** Task 2 ***
	// Your next task is to load all the vertices from the OBJ file.
	// I've given you some starter code here that reads through each line of the
	// OBJ file and makes it into a stringstream.
	// The lines containing the vertices start with a 'v', so first check if each line
	// starts with a v.
	// For these V lines, you should load the X, Y and Z coordinates into a new vector
	// and push it back into your array of vertices.

	// I've set up a big std::vector here to store all the vertices.
	std::vector<Vector3> vertices;
	
	// This while loop processes all the lines of the file
	std::string line;
	while (!bunnyFile.eof())
	{
		std::getline(bunnyFile, line);
		// *** YOUR CODE HERE ***

		// Process each line of the file
		// Load it into a new Vector3, if the line starts with a V
		// Push this back into the std::vector of vertices
		// For debugging, maybe print out the vector too!
	}

	for (auto& v : vertices) {
		// **** Task 3 ****
		// Finally, let's draw the vertices!
		// For each vertex, we need to call setPixel to draw a point to the screen.
		// You should end up with an output similar to example.png.
		// Hint 1: You just need to use the x and y components of the vectors, you can ignore the z component for now.
		// Hint 2: Note the mesh coordinates range from about -0.5 to 0.5 in each axis (x, y and z). However,
		//         your image coordinates go from 0 to 512. You probably want to add on half of the width and height to move
		//         your vertices towards the centre of the screen, and multiply by a value (about 200 or so) to make the mesh
		//         big enough to see.

		// *** YOUR CODE HERE ***
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
