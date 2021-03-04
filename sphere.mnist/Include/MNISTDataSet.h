
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Sphere.h"
#include "QuantizedImage.h"

/**
 * Each image is 28x28 -> 784 integers in each word
 * Each pixel is 4 bits of grayscale
*/

namespace sphere
{
	class MNISTDataSet
	{
	public:
		MNISTDataSet();
		MNISTDataSet(const char* ImagesDataPath, const char* LabelsDataPath);
		MNISTDataSet(const char* ImagesDataPath);

		Word CreateAverageImage();

		bool IsLabelled;
		std::vector<QuantizedImage> Images;
		uint32_t Height;
		uint32_t Width;
		std::string ImagesFile;
		std::string LabelsFile;

		void Load();
		uint32_t ReverseBytes(uint32_t data);
	};
}