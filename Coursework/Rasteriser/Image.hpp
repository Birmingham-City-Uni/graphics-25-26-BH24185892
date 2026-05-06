#pragma once
#include <vector>
#include <lodepng.h>
#include "Light.hpp"

struct Color {
	uint8_t r, g, b, a;
};


void setPixel(std::vector<uint8_t>& image, int x, int y, int width, int height, const Color& color)
{
	int pixelIdx = x + y * width;
	image[pixelIdx * 4 + 0] = color.r;
	image[pixelIdx * 4 + 1] = color.g;
	image[pixelIdx * 4 + 2] = color.b;
	image[pixelIdx * 4 + 3] = color.a;
}

Color getPixel(const std::vector<uint8_t>& image, int x, int y, int width, int height)
{
	int pixelIdx = x + y * width;
	Color color{
		image[pixelIdx * 4 + 0],
		image[pixelIdx * 4 + 1],
		image[pixelIdx * 4 + 2],
		image[pixelIdx * 4 + 3]
	};
	return color;
}

void drawCircle(std::vector<uint8_t>& image, int width, int height, int r, int c, int radius, const Color& color) {
	if (r < 0 || r > height || c < 0 || c > height) return;
	int minC = std::max(c - radius, 0);
	int minR = std::max(r - radius, 0);
	int maxC = std::min(c + radius, width);
	int maxR = std::min(r + radius, height);

	int radSq = radius * radius;

	for(int ir = minR; ir < maxR; ++ir)
		for (int ic = minC; ic < maxC; ++ic) {
			int distSq = (ic-c)*(ic-c) + (ir-r)*(ir-r);
			if (distSq <= radSq) {
				setPixel(image, ic, ir, width, height, color);
			}
		}
}

void saveZBufferImage(const std::string& filename, const std::vector<float>& zBuffer, int width, int height)
{
	// Find max depth value (that isn't the initial FLT_MAX)
	float minDepth = FLT_MAX;
	for (float depth : zBuffer) {
		if (depth < minDepth)
			minDepth = depth;
	}
	// Set up an image with intensities based on depth / max depth
	std::vector<uint8_t> image(width * height * 4);
	for (int i = 0; i < zBuffer.size(); ++i) {
		uint8_t intensity = static_cast<uint8_t>(((zBuffer[i] - minDepth) / (1.0f - minDepth)) * 255.0f);
		image[i * 4 + 0] = intensity;
		image[i * 4 + 1] = intensity;
		image[i * 4 + 2] = intensity;
		image[i * 4 + 3] = 255;
	}

	int errorCode = lodepng::encode(filename, image, width, height);
	if (errorCode) { // check the error code, in case an error occurred.
		throw std::runtime_error("lodepng error encoding image: " + std::string(lodepng_error_text(errorCode)));
	}
}

void drawPointLights(std::vector<uint8_t>& imageBuffer, int width, int height, const std::vector<std::unique_ptr<Light>>& lights)
{
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
}

