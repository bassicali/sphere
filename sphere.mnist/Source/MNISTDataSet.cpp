
#include <cassert>
#include <cstdio>
#include <fstream>

#include "Word.h"
#include "MNISTDataSet.h"
#include "Constants.h"
#include "Visualizer.h"

using namespace std;
using namespace sphere;

MNISTDataSet::MNISTDataSet()
	: IsLabelled(false)
	, Height(0)
	, Width(0)
{
}

MNISTDataSet::MNISTDataSet(const char* ImagesDataPath)
{
	char images_path[_MAX_PATH];
	if (_fullpath(images_path, ImagesDataPath, _MAX_PATH) == nullptr)
	{
		throw exception("Error while loading images. Could not get full path.");
	}

	IsLabelled = false;
	ImagesFile = string(images_path);
	Load();
}

MNISTDataSet::MNISTDataSet(const char* ImagesDataPath, const char* LabelsDataPath)
{
	char images_path[_MAX_PATH];
	if (_fullpath(images_path, ImagesDataPath, _MAX_PATH) == nullptr)
	{
		throw exception("Error while loading images. Could not get full path.");
	}

	char labels_path[_MAX_PATH];
	if (_fullpath(labels_path, LabelsDataPath, _MAX_PATH) == nullptr)
	{
		throw exception("Error while loading labels. Could not get full path.");
	}

	IsLabelled = true;
	ImagesFile = string(images_path);
	LabelsFile = string(labels_path);
	Load();
}

uint32_t ReverseBytes(uint32_t data)
{
	return ((data & 0xFF000000) >> 24) | ((data & 0x00FF0000) >> 8) | ((data & 0x0000FF00) << 8) | ((data & 0x000000FF) << 28);
}

void MNISTDataSet::Load()
{
#define LOAD_INT(s,x) s.read(reinterpret_cast<char*>(&x),sizeof(uint32_t));x=ReverseBytes(x);

	uint32_t magic_number, num_elements;

	ifstream images_fin(ImagesFile, ios::binary | ios::in);

	if (images_fin.fail())
	{
		throw exception((string("Unable to open images file: ") + ImagesFile).c_str());
	}

	LOG_INFO("Reading images...");

	LOAD_INT(images_fin, magic_number);
	assert(magic_number == 0x803, "Images file magic number incorrect");

	LOAD_INT(images_fin, num_elements);

	LOAD_INT(images_fin, Height);
	LOAD_INT(images_fin, Width);

	LOG_INFO("\tNumber of images: %d", num_elements);
	LOG_INFO("\tImage dimensions: %dx%d", Width, Height);

	int size = Width * Height;
	uint8_t* buffer = new uint8_t[size];
	memset(buffer, 0, sizeof(uint8_t) * size);

	int bytes_read;
	int i = 0;
	while (images_fin.read((char*)buffer, size))
	{
		bytes_read = images_fin.gcount();

		// Make sure we've read an entire image's chunk of data, or else the file is corrupt or something
		assert(bytes_read == size);

		Images.push_back(QuantizedImage(buffer, bytes_read, RANGE_BIT_LEN));

		if (++i >= TRAINING_SET_LIMIT)
		{
			LOG_INFO("\tStopping early at %d parsed images", i);
			break;
		}
	}

	images_fin.close();

	if (IsLabelled)
	{
		LOG_INFO("Reading labels...");

		ifstream labels_fin(LabelsFile, ios::binary | ios::in);

		if (labels_fin.fail())
		{
			throw exception((string("Unable to open labels file: ") + LabelsFile).c_str());
		}

		LOAD_INT(labels_fin, magic_number);
		assert(magic_number == 0x801, "Labels file magic number incorrect");

		LOAD_INT(labels_fin, num_elements);

		LOG_INFO("\tNumber of labels: %d", num_elements);

		uint8_t label;
		int i = 0;
		while (i < Images.size() && labels_fin.read(reinterpret_cast<char*>(&label), sizeof(uint8_t)))
		{
			Images[i].Label = label;
			i++;
		}

		labels_fin.close();
	}

#undef LOAD_INT
}

Word MNISTDataSet::CreateWeightedAverageImage(int8_t target_label, float* WeightAdjustments)
{
	int counts[10];
	memset(counts, 0, sizeof(int) * 10);

	int* sums[10];
	for (int label = 0; label < 10; label++)
	{
		sums[label] = new int[WORD_NUM_DIMENSIONS];
		memset(sums[label], 0, sizeof(int) * WORD_NUM_DIMENSIONS);
	}

	uint8_t* buff = new uint8_t[WORD_NUM_DIMENSIONS];
	for (QuantizedImage& image : Images)
	{
		if (target_label != -1 && image.Label != target_label)
			continue;

		image.Unpack(buff, WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, 17);

		assert(image.Label < 10);
		counts[image.Label]++;

		for (int px = 0; px < WORD_NUM_DIMENSIONS; px++)
			sums[image.Label][px] += buff[px];
	}

	float* fbuff = new float[WORD_NUM_DIMENSIONS];
	memset(fbuff, 0, sizeof(float) * WORD_NUM_DIMENSIONS);

	for (int label = 0; label < 10; label++)
	{
		if (target_label != -1 && label != target_label)
			continue;

		float weight = WeightAdjustments ? WeightAdjustments[label] : 1.0f;

		for (int px = 0; px < WORD_NUM_DIMENSIONS; px++)
		{
			float average = weight * float(sums[label][px]) / counts[label];
			fbuff[px] += average;
		}
	}

	memset(buff, 0, sizeof(uint8_t) * WORD_NUM_DIMENSIONS);
	int divisor = WeightAdjustments || target_label != -1 ? 1 : 10;
	for (int px = 0; px < WORD_NUM_DIMENSIONS; px++)
	{
		buff[px] = (uint8_t)(fbuff[px] / divisor);
	}

	for (int i = 0; i < 10; i++)
		delete[] sums[i];

	QuantizedImage image(buff, WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, false);
	return Word(WORD_NUM_DIMENSIONS, RANGE_BIT_LEN, image.Data, image.Length);
}
