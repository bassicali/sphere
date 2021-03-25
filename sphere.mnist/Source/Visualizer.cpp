
#include "Visualizer.h"
#include "Constants.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <cassert>

#define CHANNELS 1

void sphere::CreateBitmap(const Word& data, const char* filename, int width, int height, int resize_scale, int px_scale)
{
    assert(width * height == data.NumDimensions());

    PIXEL* buff = new PIXEL[width * height * CHANNELS];
    data.EnumerateInts([&](int index, uint8_t integer) -> void
    {
        buff[index * CHANNELS] = integer * px_scale;
    });

    CreateBitmap(buff, filename, width, height, resize_scale);
    delete[] buff;
}

void sphere::CreateBitmap(const QuantizedImage& image, const char* filename, int width, int height, int resize_scale, int px_scale)
{
    PIXEL* buff = new PIXEL[width * height * CHANNELS];
    image.Unpack(buff, width * height, RANGE_BIT_LEN, px_scale);
    CreateBitmap(buff, filename, width, height, resize_scale);
    delete[] buff;
}

void sphere::CreateBitmap(const PIXEL* pixels, const char* filename, int width, int height, int resize_scale)
{
    if (resize_scale > 1)
    {
        int new_size = (width * resize_scale) * (height * resize_scale) * CHANNELS;
        PIXEL* resized = new PIXEL[new_size];
        stbir_resize_uint8(pixels, width, height, 0, resized, width * resize_scale, height * resize_scale, 0, CHANNELS);
        stbi_write_bmp(filename, width * resize_scale, height * resize_scale, CHANNELS, resized);
        delete[] resized;
    }
    else
    {
        stbi_write_bmp(filename, width, height, 1, pixels);
    }
}

void sphere::PrintDataAsASCII(const QuantizedImage& image, std::ostream& output, int width, int height)
{
	uint8_t* unpacked_buffer = new uint8_t[width * height];
	uint8_t pixel;

    image.Unpack(unpacked_buffer, width * height, RANGE_BIT_LEN);

	output << "Label: " << image.Label << std::endl;

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			if (j == 0) output << "\t";

			pixel = unpacked_buffer[i * width + j];

			if (pixel > 0)
				output << "X";
			else
				output << "_";
		}

		output << std::endl;
	}

	delete[] unpacked_buffer;
}
