#include <Eigen/Dense>
#include <lodepng.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "BVHNode.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"
#include <fstream>

/// <summary>
/// Load a JSON config file using the nlohmann library.
/// </summary>
nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

/// <summary>
/// Load an Eigen Vector3f from a config file.
/// Call as for example loadVec3FromConfig(config["myVector3"]);
/// </summary>
Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}

int main(int argc, char* argv[]) {

	// *** Load the config file ***
	auto config = loadConfig("../config/config.json");

	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;

	// *** Set up camera and output image ***
	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);


	std::vector<uint8_t> outImage(pixHeight * pixWidth * nChannels);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	// *** Load shaders and textures ***
	
	//Tidus
	std::vector<uint8_t> TidusTexture;
	unsigned int width, height;
	lodepng::decode(TidusTexture, width, height, "../models/TidusModel/TidusTex.png");
	TexturedLambertianShader tidusShader(&TidusTexture, width, height);

	std::vector<uint8_t> TidusArmTexture;
	lodepng::decode(TidusArmTexture, width, height, "../models/TidusModel/TidusArm.png");
	TexturedLambertianShader tidusArmShader(&TidusArmTexture, width, height);

	//Yuna
	std::vector<uint8_t> YunaTexture;
	lodepng::decode(YunaTexture, width, height, "../models/YunaModel/YunaTex.png");
	TexturedLambertianShader yunaShader(&YunaTexture, width, height);

	LambertianShader redLambertianShader(red);
	PhongShader bluePlasticShader(blue, Eigen::Vector3f(1.f, 1.f, 1.f), 100.f);
	LambertianShader aquaLambertianShader(aqua);
	LambertianShader lavenderLambertianShader(lavender);
	
	MirrorShader mirrorShader;
	TexCoordTestShader texCoordTestShader;

	// *** Set up scene ***
	Scene scene;

	// Optional code: here's how to add the spot mesh to the scene, using a BVH
	// Try enabling this and comparing it to the non-BVH version below!
	Model tidusModel("../models/TidusModel/Tidus.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tidusModel, &tidusShader, 4, rotateY(0)));
	Model tidusArmModel("../models/TidusModel/TidusArm.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tidusArmModel, &tidusArmShader, 4, rotateY(0)));

	Model yunaModel("../models/YunaModel/Yuna.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(yunaModel, &yunaShader, 4, rotateY(0)));
	
	// Here's how to add the mesh without using the BVH.
	// Try comparing performance to the BVH version above.
	//Model tidusModel("../models/TidusModel/Tidus.obj");
	//scene.renderables.push_back(std::make_shared<Mesh>(&tidusShader, &tidusModel));
	//scene.renderables.back()->modelToWorld(rotateY(M_PI / 4.0f));

	// *** Add lights to scene ***
	Eigen::Vector3f ambientLight(1.f, 1.f, 1.f);

	std::vector<std::unique_ptr<Light>> lightSources;
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-1.f, 3.f, -1.f), 3.f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	lightSources.push_back(std::make_unique<DirectionalLight>(Eigen::Vector3f(0.f, -1.f, 1.f), .5f * Eigen::Vector3f(1.f, 1.f, 1.f)));

	// *** Render the scene ***

	// Shuffling the scanline order gets better CPU usage between threads
	// when some lines take longer to render than others.
	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	Ray ray = cam.getRay(531, 325);
	HitInfo hitInfo;
	scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK);
	float x = hitInfo.hitT;


	#pragma omp parallel for
	for (int y = 0; y < pixHeight; ++y) {
		for (int x = 0; x < pixWidth; ++x) {
			Ray ray = cam.getRay(x, scanlines[y]);
			HitInfo hitInfo;
			if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
				Eigen::Vector3f color = hitInfo.shader->getColor(
					hitInfo, &scene,
					lightSources, ambientLight,
					0, config["maxBounces"]);

				color.x() = std::min(color.x(), 1.f);
				color.y() = std::min(color.y(), 1.f);
				color.z() = std::min(color.z(), 1.f);


				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = color.x() * 255;
				outImage[(x + line * pixWidth) * nChannels + 1] = color.y() * 255;
				outImage[(x + line * pixWidth) * nChannels + 2] = color.z() * 255;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
			else {
				int line = (pixHeight - scanlines[y]) - 1;
				outImage[(x + line * pixWidth) * nChannels + 0] = 0;
				outImage[(x + line * pixWidth) * nChannels + 1] = 0;
				outImage[(x + line * pixWidth) * nChannels + 2] = 0;
				outImage[(x + line * pixWidth) * nChannels + 3] = 255;
			}
		}
		if (omp_get_thread_num() == omp_get_num_threads()-1) {
			std::clog << "\rScanlines remaining: " << (pixHeight - y) << ' ' << std::flush;
		}

	}

	auto renderTime = std::chrono::steady_clock::now() - startTime;

	std::cout << "Render duration " << std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f << " seconds." << std::endl;

	// *** Save the output image ***
	int errorCode;
	errorCode = lodepng::encode(config["outputFilename"], outImage, pixWidth, pixHeight);
	if (errorCode) { // check the error code, in case an error occurred.
		std::cout << "lodepng error encoding image: " << lodepng_error_text(errorCode) << std::endl;
		return errorCode;
	}

	return 0;
}
