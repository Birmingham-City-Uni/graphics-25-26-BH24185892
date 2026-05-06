#define _USE_MATH_DEFINES
#include <math.h>
#include <array>

#include <iostream>
#include <lodepng.h>
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "Shading.hpp"

struct Triangle {
	std::array<Eigen::Vector3f, 3> screen; // Coordinates of the triangle in screen space.
	std::array<Eigen::Vector3f, 3> verts; // Vertices of the triangle in world space.
	std::array<Eigen::Vector3f, 3> norms; // Normals of the triangle corners in world space.
	std::array<Eigen::Vector2f, 3> texs; // Texture coordinates of the triangle corners.
};

Eigen::Matrix4f projectionMatrix(int height, int width, float horzFov = 70.f * M_PI / 180.f, float zFar = 10.f, float zNear = 0.1f)
{
	float vertFov = horzFov * float(height) / width;
	Eigen::Matrix4f projection;
	projection <<
		1.0f / tanf(0.5f * horzFov), 0, 0, 0,
		0, 1.0f / tanf(0.5f * vertFov), 0, 0,
		0, 0, zFar / (zFar - zNear), -zFar * zNear / (zFar - zNear),
		0, 0, 1, 0;
	return projection;
}

void findScreenBoundingBox(const Triangle& t, int width, int height, int& minX, int& minY, int& maxX, int& maxY)
{
	// Find a bounding box around the triangle
	minX = std::min(std::min(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	minY = std::min(std::min(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());
	maxX = std::max(std::max(t.screen[0].x(), t.screen[1].x()), t.screen[2].x());
	maxY = std::max(std::max(t.screen[0].y(), t.screen[1].y()), t.screen[2].y());

	// Constrain it to lie within the image.
	minX = std::min(std::max(minX, 0), width - 1);
	maxX = std::min(std::max(maxX, 0), width - 1);
	minY = std::min(std::max(minY, 0), height - 1);
	maxY = std::min(std::max(maxY, 0), height - 1);
}
void drawTriangle(std::vector<uint8_t>& image, int width, int height,
	std::vector<float>& zBuffer,
	const Triangle& t,
	const std::vector<std::unique_ptr<Light>>& lights,
	const Eigen::Vector3f& albedo, const Eigen::Vector3f& specularColor,
	float specularExponent,
	const Eigen::Vector3f& camWorldPos,
	bool backfaceCull = true)
{
	int minX, minY, maxX, maxY;
	findScreenBoundingBox(t, width, height, minX, minY, maxX, maxY);

	Eigen::Vector2f edge1 = v2(t.screen[2] - t.screen[0]);
	Eigen::Vector2f edge2 = v2(t.screen[1] - t.screen[0]);
	float triangleArea = 0.5f * vec2Cross(edge2, edge1);
	if (backfaceCull && triangleArea < 0) {
		return;
	}
	triangleArea = fabsf(triangleArea); // use absolute area for barycentric math

	for (int x = minX; x <= maxX; ++x)
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

			float depth = t.screen[0].z() * b0 + t.screen[1].z() * b1 + t.screen[2].z() * b2;
			int depthIdx = static_cast<int>(p.x()) + static_cast<int>(p.y()) * width;
			if (depth > zBuffer[depthIdx]) continue;
			zBuffer[depthIdx] = depth;

			Eigen::Vector3f normP = t.norms[0] * b0 + t.norms[1] * b1 + t.norms[2] * b2;
			normP.normalize();

			// Work out colour at this position.
			Eigen::Vector3f color = Eigen::Vector3f::Zero();

			// Iterate over lights, and sum to find colour.
			for (auto& light : lights) {

				// Work out the contribution from this light source, and add it to the color variable.

				// Work out the intensity of this light source, at the point worldP.
				Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);

				// We only need to do the following if the light isn't an ambient light.
				if (light->getType() != Light::Type::AMBIENT) {

					// Subtask 3: Work out correct inputs for the phongSpecularTerm function inside drawTriangle, and draw an image!
					// *** YOUR CODE HERE ***
					// Work out the incoming light dir (from the light into the surface point).
					Eigen::Vector3f incomingLightDir = light->getDirection(worldP);
					// Work out the view direction (from surface point towards camera). Make sure it's normalized!
					Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();
					// Find the specular term by calling phongSpecularTerm.
					float specularTerm = phongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);
					// *** END YOUR CODE ***

					Eigen::Vector3f specularOut = specularColor * specularTerm;
					specularOut = coeffWiseMultiply(specularOut, lightIntensity);

					// Take the dot product of the normal with the light direction.
					float dotProd = normP.dot(-incomingLightDir);

					// We don't want negative light - if dot product less than 0, set it to 0.
					dotProd = std::max(dotProd, 0.0f);

					// Multiply the light intensity by the dot product.
					Eigen::Vector3f diffuseOut = lightIntensity * dotProd;
					diffuseOut = coeffWiseMultiply(diffuseOut, albedo);

					// Add both diffuse and specular components to the colour.
					color += specularOut;
					color += diffuseOut;
				}
				else {
					// Light is ambient - just multiply light intensity with albedo.
					color += coeffWiseMultiply(lightIntensity, albedo);
				}
			}

			Color c;
			// Gamma-correcting colours.
			c.r = std::min(powf(color.x(), 1 / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1 / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1 / 2.2f), 1.0f) * 255;

			c.a = 255;

			setPixel(image, x, y, width, height, c);
		}
}
void drawMesh(std::vector<unsigned char>& image,
	std::vector<float>& zBuffer,
	const Mesh& mesh,
	const Eigen::Vector3f& albedo, const Eigen::Vector3f& specularColor,
	float specularExponent,
	const Eigen::Vector3f& camWorldPos,
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

		Eigen::Vector4f vClip0 = worldToClip * modelToWorld * vec3ToVec4(v0);
		vClip0 /= vClip0.w();
		Eigen::Vector4f vClip1 = worldToClip * modelToWorld * vec3ToVec4(v1);
		vClip1 /= vClip1.w();
		Eigen::Vector4f vClip2 = worldToClip * modelToWorld * vec3ToVec4(v2);
		vClip2 /= vClip2.w();

		// Only skip if ALL 3 vertices are outside the SAME clip plane
		bool cull =
			(vClip0.x() < -1.f && vClip1.x() < -1.f && vClip2.x() < -1.f) ||
			(vClip0.x() >  1.f && vClip1.x() >  1.f && vClip2.x() >  1.f) ||
			(vClip0.y() < -1.f && vClip1.y() < -1.f && vClip2.y() < -1.f) ||
			(vClip0.y() >  1.f && vClip1.y() >  1.f && vClip2.y() >  1.f) ||
			(vClip0.z() < -1.f && vClip1.z() < -1.f && vClip2.z() < -1.f) ||
			(vClip0.z() >  1.f && vClip1.z() >  1.f && vClip2.z() >  1.f);
		if (cull) continue;

		t.screen[0] = Eigen::Vector3f((vClip0.x() + 1.0f) * width / 2, (-vClip0.y() + 1.0f) * height / 2, vClip0.z());
		t.screen[1] = Eigen::Vector3f((vClip1.x() + 1.0f) * width / 2, (-vClip1.y() + 1.0f) * height / 2, vClip1.z());
		t.screen[2] = Eigen::Vector3f((vClip2.x() + 1.0f) * width / 2, (-vClip2.y() + 1.0f) * height / 2, vClip2.z());

		t.norms[0] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n0).normalized();
		t.norms[1] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n1).normalized();
		t.norms[2] = (modelToWorld.block<3, 3>(0, 0).inverse().transpose() * n2).normalized();

		t.texs[0] = mesh.texs[mesh.tFaces[i][0]];
		t.texs[1] = mesh.texs[mesh.tFaces[i][1]];
		t.texs[2] = mesh.texs[mesh.tFaces[i][2]];

		drawTriangle(image, width, height, zBuffer, t, lights, albedo, specularColor, specularExponent, camWorldPos);
	}
}


int main()
{
	std::string outputFilename = "output.png";

	const int width = 1920, height = 1080;
	const int nChannels = 4;

	// Set up an image buffer
	std::vector<uint8_t> imageBuffer(height*width*nChannels);
	std::vector<float> zBuffer(height * width);

    // **** Replace this bit with your lovely rasteriser code ****

	Color black{ 0,0,0,255 };
	for (int r = 0; r < height; ++r) {
		for (int c = 0; c < width; ++c) {
			setPixel(imageBuffer, c, r, width, height, black);
			zBuffer[r * width + c] = 1.0f;
		}
	}

	Eigen::Matrix4f projection = projectionMatrix(height, width);

	// This matrix rotates the camera, tilting it down, then translates it up to make it look down on the scene.
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.1f, -0.7f, 2.f));

	Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);

	// The main important task = set up the worldToCamera and worldToClip matrices here!
	// Set up worldToCamera, based on cameraToWorld above
	Eigen::Matrix4f worldToCamera = cameraToWorld.inverse();
	// Set up worldToClip, using the projection and worldToCamera matrices
	Eigen::Matrix4f worldToClip = projection * worldToCamera;

	// *** END YOUR CODE ***

	std::string TidusModel = "../models/TidusModel/Tidus.obj";
	std::string TidusArmModel = "../models/TidusModel/TidusArm.obj";
	std::string YunaModel = "../models/YunaModel/Yuna.obj";

	std::string BGModel = "../models/Assets/BG/BG.obj";
	std::string BranchModel = "../models/Assets/Branch/Branch.obj";
	std::string Crystal1Model = "../models/Assets/Crystals/Crystal1.obj";
	std::string Crystal2Model = "../models/Assets/Crystals/Crystal2.obj";
	std::string WaterModel = "../models/Assets/Water/Water.obj";

	// Subtask 4: Try re-rendering your image with different lighting setups, and specular exponents, and see how it changes!
	// You can modify the lighting setup here....
	std::vector<std::unique_ptr<Light>> lights;
	//lights.emplace_back(new AmbientLight(Eigen::Vector3f(0.1f, 0.1f, 0.1f)));
	lights.emplace_back(new DirectionalLight(Eigen::Vector3f(0.4f, 0.4f, 0.4f), Eigen::Vector3f(1.f, -1.f, 0.0f)));
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(2.f, 2.f, 2.f)));

	Mesh TidusMesh = loadMeshFile(TidusModel);
	Mesh TidusArmMesh = loadMeshFile(TidusArmModel);

	Mesh YunaMesh = loadMeshFile(YunaModel);

	Mesh BGMesh = loadMeshFile(BGModel);
	Mesh BranchMesh = loadMeshFile(BranchModel);
	Mesh Crystal1Mesh = loadMeshFile(Crystal1Model);
	Mesh Crystal2Mesh = loadMeshFile(Crystal2Model);
	Mesh WaterMesh = loadMeshFile(WaterModel);

	//Models
	Eigen::Matrix4f ModelsTransform;
	ModelsTransform = translationMatrix(Eigen::Vector3f(0.0f, -1.0f, 3.f));
	// .... and change the specular exponent here!
	drawMesh(imageBuffer, zBuffer, TidusMesh, Eigen::Vector3f(0.f, 0.5f, 0.8f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		ModelsTransform, worldToClip, lights, width, height);

	drawMesh(imageBuffer, zBuffer, TidusArmMesh, Eigen::Vector3f(0.f, 0.5f, 0.8f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		ModelsTransform, worldToClip, lights, width, height);

	drawMesh(imageBuffer, zBuffer, YunaMesh, Eigen::Vector3f(0.8f, 0.5f, 0.f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		ModelsTransform, worldToClip, lights, width, height);

	//Branch
	Eigen::Matrix4f BranchTransform;
	BranchTransform = translationMatrix(Eigen::Vector3f(-.63f, -0.98f, 7.f));

	drawMesh(imageBuffer, zBuffer, BranchMesh, Eigen::Vector3f(0.5f, 0.0f, 0.f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		BranchTransform, worldToClip, lights, width, height);

	//Crystals
	Eigen::Matrix4f CrystalTransform;
	CrystalTransform = translationMatrix(Eigen::Vector3f(-.68f, -0.98f, 7.f));

	drawMesh(imageBuffer, zBuffer, Crystal1Mesh, Eigen::Vector3f(0.8f, 0.5f, 0.f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		CrystalTransform, worldToClip, lights, width, height);

	drawMesh(imageBuffer, zBuffer, Crystal2Mesh, Eigen::Vector3f(0.8f, 0.5f, 0.f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		CrystalTransform, worldToClip, lights, width, height);

	//Water
	Eigen::Matrix4f WaterTransform;
	WaterTransform = translationMatrix(Eigen::Vector3f(0.0f, -0.87f, 2.f)) * scaleMatrix(1.5f);
	drawMesh(imageBuffer, zBuffer, WaterMesh, Eigen::Vector3f(0.0f, 0.0f, 0.5f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		WaterTransform, worldToClip, lights, width, height);

	//Background
	Eigen::Matrix4f BGTransform;
	BGTransform = translationMatrix(Eigen::Vector3f(0.5f, -1.f, -0.8f)) * rotateYMatrix(M_PI) * scaleMatrix(2.0f);
	drawMesh(imageBuffer, zBuffer, BGMesh, Eigen::Vector3f(0.f, 0.5f, -0.5f),
		Eigen::Vector3f::Ones() * 1.0f, 10.f, camWorldPos,
		BGTransform, worldToClip, lights, width, height);


	// For debug - draw point lights as colored circles so we can see where they are
	drawPointLights(imageBuffer, width, height, lights);
  

    // **** End lovely rasteriser code ****

    // Save the image
    int errorCode;
        errorCode = lodepng::encode(outputFilename, imageBuffer, width, height);
        if (errorCode) { // check the error code, in case an error occurred.
            std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
            return errorCode;
        }

		saveZBufferImage(outputFilename + "_zBuffer.png", zBuffer, width, height);

    return 0;
}
