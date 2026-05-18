#include <Eigen/Dense>
#include <lodepng.h>
#include <json/json.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>
#include "BVHNode.hpp"
#include "Triangle.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PointLight.hpp"
#include "DirectionalLight.hpp"
#include "LambertianShader.hpp"
#include "TexturedLambertianShader.hpp"
#include "PhongShader.hpp"
#include "TexturedPhongShader.hpp"
#include "MirrorShader.hpp"
#include "TexCoordTestShader.hpp"
#include "Model.hpp"
#include <fstream>
#include <SDL2/SDL.h>
#undef main

nlohmann::json loadConfig(const std::string& filename)
{
	std::ifstream configStream(filename);
	nlohmann::json config = nlohmann::json::parse(configStream);
	return config;
}

Eigen::Vector3f loadVec3FromConfig(const nlohmann::json& config)
{
	return Eigen::Vector3f(config[0], config[1], config[2]);
}

/// <summary>
/// Returns a flicker multiplier for the lights contribution only.
/// Uses smooth interpolation between random targets for organic transitions.
/// </summary>
float calcFlicker(float elapsedSeconds, std::mt19937& rng, std::uniform_real_distribution<float>& noiseDist)
{
	// Slow sine pulse — cycles every ~9 seconds
	float pulse = 0.5f + 0.5f * std::sin(elapsedSeconds * 1.1f);

	// Smooth noise: interpolate between two random targets over a fixed interval
	static float noiseFrom   = 0.f;
	static float noiseTo     = 0.f;
	static float noiseT      = 0.f;
	const  float noisePeriod = 0.9f; // seconds between each new noise target

	noiseT += 1.f / 60.f; // advance by one frame
	if (noiseT >= noisePeriod) {
		noiseFrom = noiseTo;
		noiseTo   = noiseDist(rng);
		noiseT    = 0.f;
	}

	// Smoothstep interpolation between noiseFrom and noiseTo
	float t = noiseT / noisePeriod;
	t = t * t * (3.f - 2.f * t); // smoothstep
	float noise = noiseFrom + t * (noiseTo - noiseFrom);

	return std::max(0.55f, std::min(1.0f, pulse * 0.38f + 0.62f + noise));
}

int main(int argc, char* argv[]) {

	auto config = loadConfig("../config/config.json");

	const int pixHeight = config["pixHeight"], pixWidth = config["pixWidth"];
	const int nChannels = 4;
	const int maxBounces = config["maxBounces"];
	const std::string outputFilename = config["outputFilename"];

	Camera cam(
		loadVec3FromConfig(config["cameraPos"]),
		loadVec3FromConfig(config["cameraForward"]),
		loadVec3FromConfig(config["cameraUp"]),
		pixWidth, pixHeight,
		config["cameraFov"]);

	// Two separate buffers: ambient-only pass and lights-only pass.
	// Each stores float values (not clamped yet) so we can blend them per frame.
	const int nPixels = pixHeight * pixWidth;
	std::vector<float> ambientBuf(nPixels * 3, 0.f);  // RGB ambient contribution
	std::vector<float> lightsBuf(nPixels * 3, 0.f);   // RGB lights contribution
	std::vector<uint8_t> outImage(nPixels * nChannels, 0);

	Eigen::Vector3f
		red(1.f, 0.f, 0.f),
		blue(0.f, 0.f, 1.f),
		aqua(0.f, .8f, .8f),
		lavender(178.f / 255.f, 164.f / 255.f, 212.f / 255.f);

	unsigned int width, height;

	std::vector<uint8_t> TidusTexture;
	lodepng::decode(TidusTexture, width, height, "../models/TidusModel/TidusTex.png");
	TexturedLambertianShader tidusShader(&TidusTexture, width, height);

	std::vector<uint8_t> TidusArmTexture;
	lodepng::decode(TidusArmTexture, width, height, "../models/TidusModel/TidusArm.png");
	TexturedPhongShader tidusArmShader(&TidusArmTexture, width, height, Eigen::Vector3f(8.f, 8.f, 8.f), 45.f);

	std::vector<uint8_t> YunaTexture;
	lodepng::decode(YunaTexture, width, height, "../models/YunaModel/YunaTex.png");
	TexturedLambertianShader yunaShader(&YunaTexture, width, height);

	std::vector<uint8_t> WaterTexture;
	lodepng::decode(WaterTexture, width, height, "../models/Assets/Water/Water.png");
	TexturedLambertianShader waterShader(&WaterTexture, width, height);

	std::vector<uint8_t> BGTexture;
	lodepng::decode(BGTexture, width, height, "../models/Assets/BG/BG.png");
	TexturedLambertianShader bgShader(&BGTexture, width, height);

	std::vector<uint8_t> BranchTexture;
	lodepng::decode(BranchTexture, width, height, "../models/Assets/Branch/Branch.png");
	TexturedLambertianShader branchShader(&BranchTexture, width, height);

	std::vector<uint8_t> CrystalsTexture;
	lodepng::decode(CrystalsTexture, width, height, "../models/Assets/Crystals/CrystalsTex.png");
	TexturedPhongShader crystalsShader(&CrystalsTexture, width, height, Eigen::Vector3f(8.f, 8.f, 8.f), 15.f);

	LambertianShader redLambertianShader(red);
	PhongShader bluePlasticShader(blue, Eigen::Vector3f(1.f, 1.f, 1.f), 1.f);
	LambertianShader aquaLambertianShader(aqua);
	LambertianShader lavenderLambertianShader(lavender);

	MirrorShader mirrorShader;
	TexCoordTestShader texCoordTestShader;

	Scene scene;

	Eigen::Matrix4f ModelsTransform = makeTranslationMatrix(Eigen::Vector3f(-.1f, -.2f, -4.3f));
	Eigen::Matrix4f BCTransform = makeTranslationMatrix(Eigen::Vector3f(-.6f, -.15f, -0.4f));
	Eigen::Matrix4f CrystalTransform = makeTranslationMatrix(Eigen::Vector3f(-0.3f, -.2f, -0.9f));
	Eigen::Matrix4f WaterTransform = makeTranslationMatrix(Eigen::Vector3f(-1.5f, -.2f, -2.f)) * rotateY(M_PI / 10.0f) * uniformScale(2.f);
	Eigen::Matrix4f BGTransform = makeTranslationMatrix(Eigen::Vector3f(1.12f, -.2f, -13.f)) * rotateY(M_PI) * uniformScale(5.f);

	Model tidusModel("../models/TidusModel/Tidus.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tidusModel, &tidusShader, 10, ModelsTransform));
	Model tidusArmModel("../models/TidusModel/TidusArm.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(tidusArmModel, &tidusArmShader, 10, ModelsTransform));

	Model yunaModel("../models/YunaModel/Yuna.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(yunaModel, &yunaShader, 10, ModelsTransform));

	Model waterModel("../models/Assets/Water/Water.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(waterModel, &waterShader, 10, WaterTransform));

	Model bgModel("../models/Assets/BG/BG.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(bgModel, &bgShader, 10, BGTransform));

	Model branchModel("../models/Assets/Branch/Branch.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(branchModel, &branchShader, 10, BCTransform));

	Model Crystal2Model("../models/Assets/Crystals/Crystal2.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(Crystal2Model, &crystalsShader, 10, BCTransform));

	Model Crystal1Model("../models/Assets/Crystals/Crystal1.obj");
	scene.renderables.push_back(std::make_shared<BVHNode>(Crystal1Model, &crystalsShader, 10, CrystalTransform));

	Eigen::Vector3f ambientLight(0.01f, 0.01f, 0.01f);
	Eigen::Vector3f noAmbient(0.f, 0.f, 0.f); // Used for the lights-only pass

	std::vector<std::unique_ptr<Light>> lightSources;
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(0.f, 1.f, -4.5f), .1f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-.048f, -0.07f, -4.4f), .003f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	lightSources.push_back(std::make_unique<DirectionalLight>(Eigen::Vector3f(0.f, -1.f, 1.f), 0.8f * Eigen::Vector3f(1.f, 1.f, 1.f)));
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-1.f, 0.1f, -3.2f), .05f * Eigen::Vector3f(1.f, 1.f, 2.f)));
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(-1.6f, 0.0001f, -2.7f), .008f * Eigen::Vector3f(1.f, 1.f, 2.f)));
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(2.5f, 4.8f, 7.f), 1.f * Eigen::Vector3f(8.f, 2.f, 1.f)));
	lightSources.push_back(std::make_unique<PointLight>(Eigen::Vector3f(5.9f, 3.8f, 7.f), 1.f * Eigen::Vector3f(8.f, 2.f, 1.f)));

	// *** Initialise SDL2 ***
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"Ray Tracer",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		pixWidth, pixHeight, 0);
	if (!window) {
		std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	SDL_Texture* texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		pixWidth, pixHeight);
	if (!texture) {
		std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	std::atomic<bool> renderDone(false);

	std::vector<unsigned int> scanlines(pixHeight);
	for (int i = 0; i < pixHeight; ++i) scanlines[i] = i;

	if (config["shuffleScanlines"]) {
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(scanlines.begin(), scanlines.end(), g);
	}

	auto startTime = std::chrono::steady_clock::now();

	std::thread renderThread([&]() {
		// Empty light list used for the ambient-only pass
		const std::vector<std::unique_ptr<Light>> noLights;

		#pragma omp parallel for schedule(dynamic)
		for (int y = 0; y < pixHeight; ++y) {
			for (int x = 0; x < pixWidth; ++x) {
				Ray ray = cam.getRay(x, scanlines[y]);
				HitInfo hitInfo;

				int line = (pixHeight - scanlines[y]) - 1;
				int pidx = (x + line * pixWidth) * 3;
				int oidx = (x + line * pixWidth) * nChannels;

				if (scene.intersect(ray, 1e-6f, 1e6f, hitInfo, VISIBLE_BITMASK)) {
					// Ambient only: real ambientLight, no lights, no bounces
					Eigen::Vector3f ambColor = hitInfo.shader->getColor(
						hitInfo, &scene, noLights, ambientLight, 0, 0);

					// Lights only: zero ambient, full light list, full bounces
					Eigen::Vector3f litColor = hitInfo.shader->getColor(
						hitInfo, &scene, lightSources, noAmbient, 0, maxBounces);

					ambientBuf[pidx + 0] = ambColor.x();
					ambientBuf[pidx + 1] = ambColor.y();
					ambientBuf[pidx + 2] = ambColor.z();

					lightsBuf[pidx + 0] = litColor.x();
					lightsBuf[pidx + 1] = litColor.y();
					lightsBuf[pidx + 2] = litColor.z();

					// Initial display at full brightness
					outImage[oidx + 0] = static_cast<uint8_t>(std::min(ambColor.x() + litColor.x(), 1.f) * 255);
					outImage[oidx + 1] = static_cast<uint8_t>(std::min(ambColor.y() + litColor.y(), 1.f) * 255);
					outImage[oidx + 2] = static_cast<uint8_t>(std::min(ambColor.z() + litColor.z(), 1.f) * 255);
					outImage[oidx + 3] = 255;
				}
				else {
					outImage[oidx + 0] = 0;
					outImage[oidx + 1] = 0;
					outImage[oidx + 2] = 0;
					outImage[oidx + 3] = 255;
				}
			}
		}
		renderDone = true;
	});

	// *** Flicker state ***
	std::mt19937 flickerRng(std::random_device{}());
	std::uniform_real_distribution<float> flickerNoise(-0.02f, 0.02f);
	auto flickerStart = std::chrono::steady_clock::now();

	bool quit = false;
	bool pngSaved = false;
	auto lastTextureUpdate = std::chrono::steady_clock::now();

	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				quit = true;
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
				quit = true;
		}

		float elapsed = std::chrono::duration<float>(
			std::chrono::steady_clock::now() - flickerStart).count();

		auto now = std::chrono::steady_clock::now();
		bool shouldUpdate = renderDone ||
			std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTextureUpdate).count() >= 100;

		if (renderDone) {
			// Composite ambient (fixed) + lights (flickered) into outImage each frame
			float flicker = calcFlicker(elapsed, flickerRng, flickerNoise);
			for (int i = 0; i < nPixels; ++i) {
				int pidx = i * 3;
				int oidx = i * nChannels;
				float r = ambientBuf[pidx + 0] + lightsBuf[pidx + 0] * flicker;
				float g = ambientBuf[pidx + 1] + lightsBuf[pidx + 1] * flicker;
				float b = ambientBuf[pidx + 2] + lightsBuf[pidx + 2] * flicker;
				outImage[oidx + 0] = static_cast<uint8_t>(std::min(r, 1.f) * 255);
				outImage[oidx + 1] = static_cast<uint8_t>(std::min(g, 1.f) * 255);
				outImage[oidx + 2] = static_cast<uint8_t>(std::min(b, 1.f) * 255);
				outImage[oidx + 3] = 255;
			}

			// Remove the color mod — compositing is now done manually
			SDL_SetTextureColorMod(texture, 255, 255, 255);

			if (!pngSaved) {
				auto renderTime = std::chrono::steady_clock::now() - startTime;
				std::cout << "Render duration "
					<< std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count() * 1e-3f
					<< " seconds." << std::endl;

				int errorCode = lodepng::encode(outputFilename, outImage, pixWidth, pixHeight);
				if (errorCode)
					std::cout << "lodepng error: " << lodepng_error_text(errorCode) << std::endl;

				pngSaved = true;
			}
		}

		if (shouldUpdate) {
			SDL_UpdateTexture(texture, nullptr, outImage.data(), pixWidth * nChannels);
			lastTextureUpdate = now;
		}

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);

		SDL_Delay(16);
	}

	renderThread.join();

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
