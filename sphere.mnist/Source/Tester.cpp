
#include <iostream>

#include "Params.h"
#include "Common.h"
#include "Word.h"
#include "Tester.h"
#include "Visualizer.h"

using namespace std;
using namespace sphere;

Tester::Tester(const std::string& ImagesFile, const std::string& LabelsFile, const std::string& MemoryFile)
	: data(ImagesFile.c_str(), LabelsFile.c_str())
{
	sdm = Memory::LoadFromFile(MemoryFile);
}

void Tester::TestImages(int num_images)
{
	int freq_counter[10];
	memset(freq_counter, 0, sizeof(int) * 10);

	int success = 0;
	int total = 0;

	for (int i = 0; i < data.Images.size(); i++)
	{
		QuantizedImage& image = data.Images[i];
		Word address(image.NumPixels, sdm.RangeBitLength(), image.Data, image.Length);
		
		LOG_INFO("Cuing memory for image with label '%d'", image.Label);
		Word data = sdm.Read(address);

		char filename[100];
		if (i < 10)
		{
			sprintf_s(filename, "visual\\Test-Image-%d.bmp", image.Label);
			CreateBitmap(image, filename, 28, 28, 10);
		}

		data.EnumerateInts([&](int index, uint8_t integer) -> void
		{
			if (integer > 9)
			{
				LOG_ERROR("Invalid integer found in memory");
				return;
			}

			freq_counter[integer]++;
		});

		int max_value = 0;
		uint8_t value_at_max;
		for (int j = 0; j < 10; j++)
		{
			if (freq_counter[j] > max_value)
			{
				max_value = freq_counter[j];
				value_at_max = j;
			}
		}

		bool is_match = value_at_max == image.Label;
		LOG_INFO("Result: '%d' -> %s", value_at_max, is_match ? "Success" : "Fail");

		if (is_match)
			success++;

		total++;
	}

	LOG_INFO("Test result: %d/%d (%.2f%)", success, total, float(success) / total);
}
