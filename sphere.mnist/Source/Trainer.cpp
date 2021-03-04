
#include <cassert>
#include <iostream>

#include "Sphere.h"
#include "Trainer.h"
#include "Visualizer.h"
#include "Params.h"
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

void Trainer::TrainMemory(const char* filename, int limit, int log_distances, int save_bitmaps)
{
	LOG_INFO("Creating %d random hard locations", numHardLocations);
	sdm.Initialize(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, numHardLocations, RADIUS);

	auto avg = data.CreateAverageImage();
	CreateBitmap(avg, "average.bmp", 28, 28, 10);

	LOG_INFO("Imprinting hard locations with data-set average");
	int hl_idx = 0;
	for (HardLocation& hl : sdm.HardLocations())
	{
		hl.Address().Imprint(avg, 0.3f, 1);

		if (++hl_idx % (numHardLocations/10) == 0)
			LOG_INFO("Imprinting HL %d of %d", hl_idx, sdm.HardLocations().size());
	}

	LOG_INFO("Finished initializing hard locations");

	if (log_distances > 0)
		LogDistances(log_distances);

	if (save_bitmaps > 0)
		CreateVisualizations(save_bitmaps);

	const int data_len = (WORD_NUM_DIMENSIONS * RANGE_BIT_LEN) / 8;
	assert(WORD_NUM_DIMENSIONS * RANGE_BIT_LEN % 8 == 0);
	uint8_t* buff = new uint8_t[data_len];

	LOG_INFO("Storing %d images", data.Images.size());
	int count = 0;
	isTraining.store(1);

	for (QuantizedImage& image : data.Images)
	{
		if (image.Data == nullptr)
			continue;

		// Compose a data word for storing the label; a repeating 8 bit (the label) sequence
		memset(buff, image.Label, data_len);
		Word image_data(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, buff, data_len);
		Word address(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, image.Data, image.Length);
		sdm.Write(address, image_data);
		count++;

		LOG_INFO("Stored image for '%d' (%d of %d)", int(image.Label), count, data.Images.size());

		if (limit > 0 && count >= limit)
		{
			LOG_INFO("Training limit reached, stopping: %d", limit);
			break;
		}

		if (stopTraining.load() == 1)
		{
			LOG_INFO("Training interrupted at %d images", count);
			break;
		}
	}

	LOG_INFO("Saving memory to disk: %s", filename);
	sdm.SaveToFile(filename);
	
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
	char filename[256];
	memset(filename, 0, sizeof(char) * 256);

	for (int i = 0; i < count; i++) 
	{
		sprintf_s(filename, "visual\\HL-%d.bmp", i);
		const Word& addr = sdm.HardLocations()[i].Address();
		CreateBitmap(addr, filename, 28, 28, 6);
	}

	for (int i = 0; i < count; i++)
	{
		QuantizedImage& image = data.Images[i];
		sprintf_s(filename, "visual\\Image-%d-%d.bmp", i, image.Label);
		CreateBitmap(image, filename, 28, 28, 6);
	}
}
