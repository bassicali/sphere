
#include <iostream>

#include "Constants.h"
#include "Common.h"
#include "Word.h"
#include "Tester.h"
#include "Visualizer.h"

using namespace std;
using namespace sphere;

Tester::Tester(const std::string& ImagesFile, const std::string& LabelsFile)
	: data(ImagesFile.c_str(), LabelsFile.c_str())
{
}

RecallStats Tester::TestImages(sphere::Memory& sdm, int limit)
{
	RecallStats stats;
	int freq_counter[10];

	limit = limit > 0 && limit <= data.Images.size() ? limit : data.Images.size();
	LOG_INFO("Recalling %d images", limit);

	for (int img_idx = 0; img_idx < limit; img_idx++)
	{
		QuantizedImage& image = data.Images[img_idx];

		uint8_t recall = CueMemory(image, sdm);

		if (recall != 0xFF)
		{
			bool is_match = recall == image.Label;
			LOG_INFO("(%d of %d) Result of '%d': '%d' -> %s", img_idx, limit, image.Label, recall, is_match ? "Success" : "Fail");

			if (is_match)
			{
				stats.Overall.Success++;
				stats.Scores[image.Label].Success++;
			}
		}
		else
		{
			LOG_INFO("(%d of %d) Result of '%d': ??? -> Inconclusive", img_idx, limit, image.Label);
			stats.Overall.Inconclusive++;
			stats.Scores[image.Label].Inconclusive++;
		}

		stats.Overall.Total++;
		stats.Scores[image.Label].Total++;
	}

	return stats;
}

uint8_t Tester::CueMemory(QuantizedImage& image, Memory& memory)
{
	static int freq_counter[10];

	Word address(image.NumPixels, memory.RangeBitLength(), image.Data, image.Length);

	bool found = false;
	Word data = memory.Read(address, found);

	if (found)
	{
		memset(freq_counter, 0, sizeof(int) * 10);
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

		return value_at_max;
	}

	return 0xFF;
}

RecallScore::RecallScore() 
	: Success(0)
	, Inconclusive(0)
	, Total(0)
{
}

void RecallStats::Print()
{
	LOG_INFO("RECALL SCORE: %d of %d (%.2f) - Inconclusive: %d of %d (%.2f)", 
		Overall.Success, 
		Overall.Total, 
		float(Overall.Success) / Overall.Total,
		Overall.Inconclusive,
		Overall.Total,
		float(Overall.Inconclusive) / Overall.Total);

	for (int label = 0; label < 10; label++)
	{
		LOG_INFO("\tLabel '%d': %d of %d (%.2f) - Inconclusive: %d of %d (%.2f)", 
			label, 
			Scores[label].Success, 
			Scores[label].Total, 
			float(Scores[label].Success) / Scores[label].Total,
			Scores[label].Inconclusive,
			Scores[label].Total, 
			float(Scores[label].Inconclusive) / Scores[label].Total);
	}
}

