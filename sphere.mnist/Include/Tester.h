
#pragma once

#include <string>

#include "Sphere.h"
#include "MNISTDataSet.h"

namespace sphere
{
	class Tester
	{
	public:
		Tester(const std::string& ImagesFile, const std::string& LabelsFile, const std::string& MemoryFile);

		void TestImages(int num_images);

	private:
		MNISTDataSet data;
		sphere::Memory sdm;
	};
}