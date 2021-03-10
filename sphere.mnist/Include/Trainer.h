
#pragma once

#include <string>
#include <atomic>

#include "Sphere.h"
#include "MNISTDataSet.h"

namespace sphere
{
	struct HLStats
	{
		int HLCount;
		int EmptyCount;
		int LabelPresence[10];	// Counts how many HLs a label is present in
		int LabelWrites[10];	// Counts how many times a label has been written among all HLs
		long TotalWrites;		// Total writes among all HLs

		HLStats();
		void Print();
	};

	class Trainer
	{
	public:
		Trainer();
		Trainer(const std::string& ImagesFile, const std::string& LabelsFile, int NumHardLocations);
		
		void InitializeHardLocationsAddrs(float imprint_weight, float* label_weights, bool segment_imprints);
		void TrainMemory(const char* filename, int start_from = 0, int limit = 0, int sanity_check = 0, int save_bitmaps = 0);
		void StopTraining();
		bool IsTraining();

		void LogDistances(uint8_t marker);
		void CreateVisualizations(int count);

		HLStats AnalyzeHardLocations();

		sphere::Memory& Memory() { return sdm; }
		sphere::MNISTDataSet& DataSet() { return data; }

	private:
		MNISTDataSet data;
		sphere::Memory sdm;

		int numHardLocations;
		std::atomic<int> stopTraining;
		std::atomic<int> isTraining;

		Word average;
		Word averages[10];
	};
}