
#pragma once

#include <string>
#include <atomic>

#include "Sphere.h"
#include "MNISTDataSet.h"

namespace sphere
{
	class Trainer
	{
	public:
		Trainer();
		Trainer(const std::string& ImagesFile, const std::string& LabelsFile, int NumHardLocations);

		void TrainMemory(const char* filename, int limit = 0, int sanity_check = 0, int save_bitmaps = 0);
		void StopTraining();
		bool IsTraining();

		void LogDistances(uint8_t marker);
		void CreateVisualizations(int count);

		sphere::Memory& Memory() { return sdm; }
		sphere::MNISTDataSet& DataSet() { return data; }

	private:
		MNISTDataSet data;
		sphere::Memory sdm;

		int numHardLocations;
		std::atomic<int> stopTraining;
		std::atomic<int> isTraining;
	};
}