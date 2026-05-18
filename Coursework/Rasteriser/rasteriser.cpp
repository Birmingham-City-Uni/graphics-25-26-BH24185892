#define _USE_MATH_DEFINES
#include <math.h>
#include <array>
#include <iostream>
#include <lodepng.h>
#include "gif.h"
#include "Image.hpp"
#include "LinAlg.hpp"
#include "Light.hpp"
#include "Mesh.hpp"
#include "Shading.hpp"

struct Triangle {
	std::array<Eigen::Vector3f, 3> screen;
	std::array<float, 3> clipW;
	std::array<Eigen::Vector3f, 3> verts;
	std::array<Eigen::Vector3f, 3> norms;
	std::array<Eigen::Vector2f, 3> texs;
	// Per-triangle material info so we can draw it later
	const std::vector<uint8_t>* albedoTexture;
	int texWidth;
	int texHeight;
	Eigen::Vector3f specularColor;
	float specularExponent;
	bool backfaceCull;
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
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight,
	const Eigen::Vector3f& specularColor,
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
	triangleArea = fabsf(triangleArea);

	for (int x = minX; x <= maxX; ++x)
		for (int y = minY; y <= maxY; ++y) {
			Eigen::Vector2f p(x, y);

			float a0 = 0.5f * fabsf(vec2Cross(v2(t.screen[1]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a1 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[2]), p - v2(t.screen[2])));
			float a2 = 0.5f * fabsf(vec2Cross(v2(t.screen[0]) - v2(t.screen[1]), p - v2(t.screen[1])));

			float b0 = a0 / triangleArea;
			float b1 = a1 / triangleArea;
			float b2 = a2 / triangleArea;

			float sum = b0 + b1 + b2;
			if (sum > 1.0001f) continue;

			float depth = t.screen[0].z() * b0 + t.screen[1].z() * b1 + t.screen[2].z() * b2;
			int depthIdx = static_cast<int>(p.x()) + static_cast<int>(p.y()) * width;
			if (depth > zBuffer[depthIdx]) continue;
			zBuffer[depthIdx] = depth;

			Eigen::Vector3f worldP = t.verts[0] * b0 + t.verts[1] * b1 + t.verts[2] * b2;

			Eigen::Vector3f normP = (t.norms[0] * b0 + t.norms[1] * b1 + t.norms[2] * b2).normalized();
			Eigen::Vector3f viewDir = (camWorldPos - worldP).normalized();
			if (normP.dot(viewDir) < 0.0f) normP = -normP;

			Eigen::Vector2f texP = t.texs[0] * b0 + t.texs[1] * b1 + t.texs[2] * b2;

			int texC = std::max(0, std::min((int)(texP.x() * texWidth),  texWidth  - 1));
			int texR = std::max(0, std::min((int)((1.0f - texP.y()) * texHeight), texHeight - 1));

			Color texColor = getPixel(albedoTexture, texC, texR, texWidth, texHeight);
			Eigen::Vector3f albedo(powf(texColor.r / 255.0f, 2.2f), powf(texColor.g / 255.0f, 2.2f), powf(texColor.b / 255.0f, 2.2f));

			Eigen::Vector3f color = Eigen::Vector3f::Zero();
			for (auto& light : lights) {
				Eigen::Vector3f lightIntensity = light->getIntensityAt(worldP);
				if (light->getType() != Light::Type::AMBIENT) {
					Eigen::Vector3f incomingLightDir = light->getDirection(worldP);
					float specularTerm = blinnPhongSpecularTerm(incomingLightDir, normP, viewDir, specularExponent);

					Eigen::Vector3f specularOut = specularColor * specularTerm;
					specularOut = coeffWiseMultiply(specularOut, lightIntensity);

					float dotProd = std::max(normP.dot(-incomingLightDir), 0.0f);

					Eigen::Vector3f diffuseOut = lightIntensity * dotProd;
					diffuseOut = coeffWiseMultiply(diffuseOut, albedo);

					color += specularOut + diffuseOut;
				}
				else {
					color += coeffWiseMultiply(lightIntensity, albedo);
				}
			}

			Color c;
			c.r = std::min(powf(color.x(), 1.0f / 2.2f), 1.0f) * 255;
			c.g = std::min(powf(color.y(), 1.0f / 2.2f), 1.0f) * 255;
			c.b = std::min(powf(color.z(), 1.0f / 2.2f), 1.0f) * 255;
			c.a = 255;
			setPixel(image, x, y, width, height, c);
		}
}
// Collects transformed triangles from a mesh into an output vector, without drawing
void collectTriangles(
	std::vector<Triangle>& outTriangles,
	const Mesh& mesh,
	const std::vector<uint8_t>& albedoTexture, int texWidth, int texHeight,
	const Eigen::Vector3f& specularColor,
	float specularExponent,
	const Eigen::Matrix4f& modelToWorld,
	const Eigen::Matrix4f& worldToClip,
	int width, int height,
	bool backfaceCull = true)
{
	for (int i = 0; i < (int)mesh.vFaces.size(); ++i)
	{
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
		t.clipW[0] = vClip0.w();
		vClip0 /= vClip0.w();
		Eigen::Vector4f vClip1 = worldToClip * modelToWorld * vec3ToVec4(v1);
		t.clipW[1] = vClip1.w();
		vClip1 /= vClip1.w();
		Eigen::Vector4f vClip2 = worldToClip * modelToWorld * vec3ToVec4(v2);
		t.clipW[2] = vClip2.w();
		vClip2 /= vClip2.w();

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

		t.albedoTexture  = &albedoTexture;
		t.texWidth       = texWidth;
		t.texHeight      = texHeight;
		t.specularColor  = specularColor;
		t.specularExponent = specularExponent;
		t.backfaceCull   = backfaceCull;

		outTriangles.push_back(t);
	}
}

int main()
{
	const int width = 1920, height = 1080;
	const int	 nChannels = 4;

	// How many triangles to add per GIF frame — increase for fewer frames, decrease for more detail
	const int trianglesPerFrame = 20;
	// Delay per frame in centiseconds (4 = ~25fps)
	const int frameDelayCs = 10;
	const char* gifFilename = "output.gif";

	// Camera setup
	Eigen::Matrix4f projection = projectionMatrix(height, width);
	Eigen::Matrix4f cameraToWorld = translationMatrix(Eigen::Vector3f(0.1f, -0.7f, 2.f));
	Eigen::Vector3f camWorldPos = (cameraToWorld * Eigen::Vector4f(0, 0, 0, 1)).block<3, 1>(0, 0);
	Eigen::Matrix4f worldToClip = projection * cameraToWorld.inverse();

	// Load textures
	std::vector<uint8_t> TidusTexture;
	unsigned int TidusTexWidth, TidusTexHeight;
	lodepng::decode(TidusTexture, TidusTexWidth, TidusTexHeight, "../models/TidusModel/TidusTex.png");
	std::vector<uint8_t> TidusArmTexture;
	unsigned int TidusArmTexWidth, TidusArmTexHeight;
	lodepng::decode(TidusArmTexture, TidusArmTexWidth, TidusArmTexHeight, "../models/TidusModel/TidusArm.png");
	std::vector<uint8_t> YunaTexture;
	unsigned int YunaTexWidth, YunaTexHeight;
	lodepng::decode(YunaTexture, YunaTexWidth, YunaTexHeight, "../models/YunaModel/YunaTex.png");
	std::vector<uint8_t> BGTexture;
	unsigned int BGTexWidth, BGTexHeight;
	lodepng::decode(BGTexture, BGTexWidth, BGTexHeight, "../models/Assets/BG/BG.png");
	std::vector<uint8_t> BranchTexture;
	unsigned int BranchTexWidth, BranchTexHeight;
	lodepng::decode(BranchTexture, BranchTexWidth, BranchTexHeight, "../models/Assets/Branch/Branch.png");
	std::vector<uint8_t> CrystalTexture;
	unsigned int CrystalTexWidth, CrystalTexHeight;
	lodepng::decode(CrystalTexture, CrystalTexWidth, CrystalTexHeight, "../models/Assets/Crystals/CrystalsTex.png");
	std::vector<uint8_t> WaterTexture;
	unsigned int WaterTexWidth, WaterTexHeight;
	lodepng::decode(WaterTexture, WaterTexWidth, WaterTexHeight, "../models/Assets/Water/Water.png");

	// Load meshes
	Mesh TidusMesh = loadMeshFile("../models/TidusModel/Tidus.obj");
	Mesh TidusArmMesh = loadMeshFile("../models/TidusModel/TidusArm.obj");
	Mesh YunaMesh = loadMeshFile("../models/YunaModel/Yuna.obj");
	Mesh BGMesh = loadMeshFile("../models/Assets/BG/BG.obj");
	Mesh BranchMesh = loadMeshFile("../models/Assets/Branch/Branch.obj");
	Mesh Crystal1Mesh = loadMeshFile("../models/Assets/Crystals/Crystal1.obj");
	Mesh Crystal2Mesh = loadMeshFile("../models/Assets/Crystals/Crystal2.obj");
	Mesh WaterMesh = loadMeshFile("../models/Assets/Water/Water.obj");

	// Lights
	std::vector<std::unique_ptr<Light>> lights;
	lights.emplace_back(new DirectionalLight(Eigen::Vector3f(0.3f, 0.3f, 0.3f), Eigen::Vector3f(1.f, -1.f, 0.0f)));
	lights.emplace_back(new AmbientLight(Eigen::Vector3f(.05f, .05f, .05f)));
	lights.emplace_back(new PointLight(Eigen::Vector3f(1.2f, 2.0f, 3.0f) * 0.2f, Eigen::Vector3f(-1.428f, -0.8f, 4.614f)));
	lights.emplace_back(new PointLight(Eigen::Vector3f(1.2f, 2.f, 3.0f) * 0.2f, Eigen::Vector3f(-1.752f, -.9f, 4.870f)));
	lights.emplace_back(new PointLight(Eigen::Vector3f(1.0f, 0.95f, 0.85f) * 0.3f, Eigen::Vector3f(0.076f, -0.4f, 2.989f)));
	lights.emplace_back(new PointLight(Eigen::Vector3f(8.f, 2.f, 1.f) * .5f, Eigen::Vector3f(1.55f, 1.3f, 7.5f)));
	lights.emplace_back(new PointLight(Eigen::Vector3f(8.f, 2.f, 1.f) * .3f, Eigen::Vector3f(2.7f, .6f, 7.5f)));

	// Transforms
	Eigen::Matrix4f BGTransform = translationMatrix(Eigen::Vector3f(0.5f, -1.f, -.9f)) * rotateYMatrix(M_PI) * scaleMatrix(2.0f);
	Eigen::Matrix4f ModelsTransform = translationMatrix(Eigen::Vector3f(0.0f, -1.0f, 3.f));
	Eigen::Matrix4f BranchTransform = translationMatrix(Eigen::Vector3f(-.63f, -0.98f, 7.f));
	Eigen::Matrix4f CrystalTransform = translationMatrix(Eigen::Vector3f(-.68f, -0.98f, 7.f));
	Eigen::Matrix4f WaterTransform = translationMatrix(Eigen::Vector3f(-1.5f, -1.f, 2.f)) * scaleMatrix(1.5f);

	// Collect ALL triangles from the entire scene into one ordered list
	std::vector<Triangle> allTriangles;
	collectTriangles(allTriangles, BGMesh, BGTexture, BGTexWidth, BGTexHeight, Eigen::Vector3f::Ones() * 1.0f, 50.f, BGTransform, worldToClip, width, height);
	collectTriangles(allTriangles, BranchMesh, BranchTexture, BranchTexWidth, BranchTexHeight, Eigen::Vector3f::Ones() * 0.2f, 1000.f, BranchTransform, worldToClip, width, height);
	collectTriangles(allTriangles, Crystal1Mesh, CrystalTexture, CrystalTexWidth, CrystalTexHeight, Eigen::Vector3f::Ones() * 1.0f, 10.f, CrystalTransform, worldToClip, width, height);
	collectTriangles(allTriangles, Crystal2Mesh, CrystalTexture, CrystalTexWidth, CrystalTexHeight, Eigen::Vector3f::Ones() * 1.0f, 10.f, CrystalTransform, worldToClip, width, height);
	collectTriangles(allTriangles, WaterMesh, WaterTexture, WaterTexWidth, WaterTexHeight, Eigen::Vector3f::Ones() * 0.4f, 2000.f, WaterTransform, worldToClip, width, height, false);
	collectTriangles(allTriangles, TidusMesh, TidusTexture, TidusTexWidth, TidusTexHeight, Eigen::Vector3f::Ones() * 0.2f, 1000.f, ModelsTransform, worldToClip, width, height);
	collectTriangles(allTriangles, TidusArmMesh, TidusArmTexture, TidusArmTexWidth, TidusArmTexHeight, Eigen::Vector3f::Ones() * 1.0f, 20.f, ModelsTransform, worldToClip, width, height);
	collectTriangles(allTriangles, YunaMesh, YunaTexture, YunaTexWidth, YunaTexHeight, Eigen::Vector3f::Ones() * 0.2f, 1000.f, ModelsTransform, worldToClip, width, height);

	std::cout << "Total triangles to draw: " << allTriangles.size() << std::endl;

	// Persistent buffers — NOT reset between frames so triangles accumulate
	std::vector<uint8_t> imageBuffer(height * width * nChannels);
	std::vector<float> zBuffer(height * width);
	Color black{ 0, 0, 0, 255 };
	for (int r = 0; r < height; ++r)
		for (int c = 0; c < width; ++c)
		{
			setPixel(imageBuffer, c, r, width, height, black);
			zBuffer[r * width + c] = 1.0f;
		}

	// Calculate total number of GIF frames
	int totalFrames = ((int)allTriangles.size() + trianglesPerFrame - 1) / trianglesPerFrame;
	std::cout << "Total GIF frames: " << totalFrames << std::endl;

	// Split into: no lights for build-up, full lights for final reveal
	std::vector<std::unique_ptr<Light>> noLights;  // empty — flat/unlit during drawing

	// Ambient-only lights for the build-up phase
	std::vector<std::unique_ptr<Light>> ambientOnlyLights;
	ambientOnlyLights.emplace_back(new AmbientLight(Eigen::Vector3f(.05f, .05f, .05f)));

	GifWriter gifWriter;
	GifBegin(&gifWriter, gifFilename, width, height, frameDelayCs);

	int trianglesDrawn = 0;
	for (int frame = 0; frame < totalFrames; ++frame)
	{
		int batchEnd = std::min(trianglesDrawn + trianglesPerFrame, (int)allTriangles.size());
		for (int i = trianglesDrawn; i < batchEnd; ++i)
		{
			const Triangle& t = allTriangles[i];
			drawTriangle(imageBuffer, width, height, zBuffer, t, ambientOnlyLights,  // <-- ambient only
				*t.albedoTexture, t.texWidth, t.texHeight,
				t.specularColor, t.specularExponent, camWorldPos, t.backfaceCull);
		}
		trianglesDrawn = batchEnd;

		GifWriteFrame(&gifWriter, imageBuffer.data(), width, height, frameDelayCs);
		std::cout << "Frame " << (frame + 1) << "/" << totalFrames
			<< " (" << trianglesDrawn << "/" << allTriangles.size() << " triangles)" << std::endl;
	}

	// Reset and re-render with all lights for the final frame
	std::cout << "Rendering final lit frame..." << std::endl;
	std::fill(imageBuffer.begin(), imageBuffer.end(), 0);
	std::fill(zBuffer.begin(), zBuffer.end(), 1.0f);
	for (int r = 0; r < height; ++r)
		for (int c = 0; c < width; ++c)
			setPixel(imageBuffer, c, r, width, height, black);

	for (const Triangle& t : allTriangles)
		drawTriangle(imageBuffer, width, height, zBuffer, t, lights,  // <-- full lights
			*t.albedoTexture, t.texWidth, t.texHeight,
			t.specularColor, t.specularExponent, camWorldPos, t.backfaceCull);

	const int finalFrameDelayCs = 300;
	GifWriteFrame(&gifWriter, imageBuffer.data(), width, height, finalFrameDelayCs);

	GifEnd(&gifWriter);
}
