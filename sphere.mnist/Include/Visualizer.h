
#pragma once

#include "Word.h"
#include "QuantizedImage.h"

namespace sphere
{
	typedef unsigned char PIXEL;

	void CreateBitmap(const Word& data, const char* filename, int width, int height, int resize_scale, int px_scale = 17);
	void CreateBitmap(const QuantizedImage& image, const char* filename, int width, int height, int resize_scale, int px_scale = 17);
	void CreateBitmap(const PIXEL* pixels, const char* filename, int width, int height, int resize_scale);

	void PrintDataAsASCII(const QuantizedImage& image, std::ostream& output, int width, int height);
}