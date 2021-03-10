
#include <iostream>
#include <filesystem>
#include <cassert>

#include "Sphere.h"
#include "Trainer.h"
#include "Visualizer.h"
#include "Constants.h"
#include "Common.h"

using namespace std;
using namespace sphere;

Trainer::Trainer()
	: stopTraining(0)
	, isTraining(0)
	, numHardLocations(0)
{

}

Trainer::Trainer(const std::string& ImagesFile, const std::string& LabelsFile, int NumHardLocations)
	: data(ImagesFile.c_str() , LabelsFile.c_str())
	, stopTraining(0)
	, isTraining(0)
	, numHardLocations(NumHardLocations)
{
}

void Trainer::InitializeHardLocationsAddrs(float imprint_weight, float* label_weights, bool segment_imprints)
{
	LOG_INFO("Creating %d random hard locations", numHardLocations);
	sdm.Initialize(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, numHardLocations, RADIUS);


	LOG_INFO("Imprinting hard locations with data-set average");

	if (!segment_imprints)
	{
		if (label_weights)
		{
			float sum = 0;
			for (int label = 0; label < 10; label++)
			{
				sum += label_weights[label];
			}

			float norm_weights[10];
			for (int i = 0; i < 10; i++)
			{
				norm_weights[i] = label_weights[i] / sum;
			}

			average = data.CreateWeightedAverageImage(-1, norm_weights);
		}
		else
		{
			average = data.CreateWeightedAverageImage(-1, nullptr);
		}

		int hl_idx = 0;
		for (HardLocation& hl : sdm.HardLocations())
		{
			hl.Address().Imprint(average, imprint_weight, 1);

			if (++hl_idx % (numHardLocations / 10) == 0)
				LOG_INFO("Imprinting HL %d of %d", hl_idx, sdm.HardLocations().size());
		}
	}
	else
	{
		int hl_per_label = sdm.HardLocations().size() / 10;
		int hl_idx = 0;

		for (int label = 0; label < 10; label++)
		{
			LOG_INFO("Imprinting %d HLs with label '%d' average", hl_per_label, label);

			averages[label] = data.CreateWeightedAverageImage(label, nullptr);

			for (int idx = 0; 
					 idx < hl_per_label && idx < sdm.HardLocations().size();
					 idx++)
			{
				sdm.HardLocations()[idx + hl_idx].Address().Imprint(averages[label], imprint_weight, 1);
			}

			hl_idx += hl_per_label;
		}
	}

	LOG_INFO("Finished initializing hard locations");
}

void Trainer::TrainMemory(const char* filename, int start_from, int limit, int log_distances, int save_bitmaps)
{
	if (log_distances > 0)
		LogDistances(log_distances);

	if (save_bitmaps > 0)
		CreateVisualizations(save_bitmaps);

	const int data_len = (WORD_NUM_DIMENSIONS * RANGE_BIT_LEN) / 8;
	assert(WORD_NUM_DIMENSIONS * RANGE_BIT_LEN % 8 == 0);
	uint8_t* buff = new uint8_t[data_len];

	int training_limit = limit > 0 && limit < data.Images.size() ? limit : data.Images.size();
	LOG_INFO("Training started: %d images", training_limit);
	isTraining.store(1);

	int count = 0;
	for (QuantizedImage& image : data.Images)
	{
		if (count >= training_limit)
		{
			LOG_INFO("Training limit reached, stopping: %d", limit);
			break;
		}

		if (stopTraining.load() == 1)
		{
			LOG_INFO("Training interrupted at %d images", count);
			break;
		}

		auto& image = data.Images[count];

		if (image.Data == nullptr)
			continue;

		// Compose a data word for storing the label; a repeating 8 bit (the label) sequence
		uint8_t pattern = (image.Label << 4) | image.Label;
		memset(buff, pattern, data_len);
		Word image_data(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, buff, data_len);
		Word address(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, image.Data, image.Length);
		sdm.Write(address, image_data);

		LOG_INFO("Stored image for '%d' (%d of %d) - Write Stats: Activated: %d (%.3f%%) | D_avg: %.2f | D_min: %.2f", 
			int(image.Label), 
			count, 
			training_limit,
			sdm.LastOPStats.Activations,
			float(sdm.LastOPStats.Activations) / sdm.HardLocations().size() * 100,
			sdm.LastOPStats.AverageDistance,
			sdm.LastOPStats.MinimumDistance);

		count++;
	}

	LOG_INFO("Analyzing hard locations");
	HLStats stats = AnalyzeHardLocations();
	stats.Print();

	if (filename)
	{
		LOG_INFO("Saving memory to disk: %s", filename);
		sdm.SaveToFile(filename);
	}

	isTraining.store(0);
	stopTraining.store(0);
}

void Trainer::StopTraining()
{
	stopTraining.store(1);
}

bool Trainer::IsTraining()
{
	return isTraining.load() == 1;
}

void Trainer::LogDistances(uint8_t marker)
{
	int i = 0;
	QuantizedImage* marker_image = nullptr;
	Word* marker_word = nullptr;

	assert(data.Images[0].NumPixels == WORD_NUM_DIMENSIONS);
	assert(data.Images[0].Length >= (WORD_NUM_DIMENSIONS * RANGE_BIT_LEN) / 8);

	for (QuantizedImage& image : data.Images)
	{
		if (image.Label == marker && marker_image == nullptr && marker_word == nullptr)
		{
			LOG_INFO("Found marker image for %d", int(marker));
			sphere::PrintDataAsASCII(data.Images[i], cout, data.Width, data.Height);
			marker_image = &image;
			marker_word = new Word(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, image.Data, image.Length);
		}
		else if (marker_word != nullptr)
		{
			Word w1(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, image.Data, image.Length);
			float dist = w1.DistanceTo(*marker_word);

			if (image.Label == marker)
				LOG_INFO("**** Distance to self (%d) : %.2f ****", int(image.Label), dist);
			else
				LOG_INFO("Distance to %d : %.2f", int(image.Label), dist);
		}

		i++;
	}
}

void Trainer::CreateVisualizations(int count)
{
	using namespace std::filesystem;

	if (!exists("visuals"))
		create_directory("visuals");

	char filename[256];
	memset(filename, 0, sizeof(char) * 256);

	if (averages[0].Length())
	{
		// Save a visualization for each kind of HL if imprints were segmented
		int hls_per_label = sdm.HardLocations().size() / 10;
		for (int label = 0; label < 10; label++)
		{
			for (int i = 0; i < count; i++)
			{
				int idx = hls_per_label * label + i;
				sprintf_s(filename, "visuals\\HL-%d.bmp", idx);
				const Word& addr = sdm.HardLocations()[idx].Address();
				CreateBitmap(addr, filename, 28, 28, 6);
			}
		}
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			sprintf_s(filename, "visuals\\HL-%d.bmp", i);
			const Word& addr = sdm.HardLocations()[i].Address();
			CreateBitmap(addr, filename, 28, 28, 6);
		}
	}
	

	for (int i = 0; i < count; i++)
	{
		QuantizedImage& image = data.Images[i];
		sprintf_s(filename, "visuals\\Image-%d-%d.bmp", i, image.Label);
		CreateBitmap(image, filename, 28, 28, 6);
	}

	if (average.Length())
		CreateBitmap(average, "visuals\\average.bmp", 28, 28, 10);

	if (averages[0].Length())
	{
		char filename[128];
		for (int label = 0; label < 10; label++)
		{
			sprintf_s(filename, "visuals\\average-%d.bmp", label);
			CreateBitmap(averages[label], filename, 28, 28, 10);
		}
	}
}

HLStats Trainer::AnalyzeHardLocations()
{
	HLStats stats;
	stats.HLCount = sdm.HardLocations().size();

	bool labelCounted[10];

	for (HardLocation& hl : sdm.HardLocations())
	{
		if (hl.WriteCount() > 0)
		{
			stats.TotalWrites += hl.WriteCount();

			memset(labelCounted, 0, sizeof(bool) * 10);

			for (uint8_t data : hl.WriteHistory())
			{
				labelCounted[data] = true;
				stats.LabelWrites[data]++;
			}

			for (int label = 0; label < 10; label++)
			{
				if (labelCounted[label])
					stats.LabelPresence[label]++;
			}
		}
		else
		{
			stats.EmptyCount++;
		}
	}

	return stats;
}

HLStats::HLStats()
	: HLCount(0)
	, EmptyCount(0)
	, TotalWrites(0)
{
	memset(LabelPresence, 0, sizeof(int) * 10);
	memset(LabelWrites, 0, sizeof(int) * 10);
}

void HLStats::Print()
{
	LOG_INFO("HARD LOCATION STATS");
	LOG_INFO("\tEmpty: %d of %d (%.2f)", EmptyCount, HLCount, float(EmptyCount) / HLCount);
	LOG_INFO("\tTotal Writes: %d", TotalWrites);

	for (int label = 0; label < 10; label++)
	{
		LOG_INFO("\tLabel '%d': Presence: %d of %d (%.2f) - Representation: %d of %d (%.2f)",
			label,
			LabelPresence[label],
			HLCount,
			float(LabelPresence[label]) / HLCount,
			LabelWrites[label],
			TotalWrites,
			float(LabelWrites[label]) / TotalWrites);
	}
}
