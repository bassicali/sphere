
#pragma once

#include <string>

#include "Sphere.h"
#include "MNISTDataSet.h"

namespace sphere
{
	struct RecallScore
	{
		int Success;
		int Inconclusive;
		int Total;
		RecallScore();
	};

	struct RecallStats
	{
		RecallScore Scores[10];
		RecallScore Overall;
		void Print();
	};

	class Tester
	{
	public:
		Tester(const std::string& ImagesFile, const std::string& LabelsFile);

		RecallStats TestImages(sphere::Memory& sdm, int num_images);

		static uint8_t CueMemory(QuantizedImage& image, Memory& memory);

	private:
		MNISTDataSet data;
		sphere::Memory sdm;
	};
}