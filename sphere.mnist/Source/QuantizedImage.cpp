
#include "QuantizedImage.h"
#include "Constants.h"
#include "Word.h"

#include <cassert>
#include <exception>

using namespace sphere;
using namespace std;

QuantizedImage::QuantizedImage(const QuantizedImage& Other)
	: Label(Other.Label)
	, Length(Other.Length)
	, NumPixels(Other.NumPixels)
{
	if (Length == 0)
		Data = nullptr;
	else
		Data = new uint8_t[Length];

	memcpy(Data, Other.Data, Other.Length);
}

int QuantizedImage::DistanceTo(const QuantizedImage& Other)
{
	Word w1(Length * sizeof(uint8_t) * 8, RANGE_BIT_LEN, this->Data, Length);
	Word w2(Other.Length * sizeof(uint8_t) * 8, RANGE_BIT_LEN, Other.Data, Other.Length);
	return w1.DistanceTo(w2);
}

/*static*/
uint8_t QuantizedImage::Quantize(uint8_t Value, int Levels)
{
	if (Levels == 0xFF)
		return Value;

	if (Value == 0)
		return 0;

	if (Value == 0xFF)
		return Levels - 1;

	int step = (0xFF + 1) / Levels;

	for (uint8_t L = 0; L < Levels; L++)
	{
		int threshold = (L + 1) * step;
		if (Value < threshold)
			return L;
	}

	return 0;
}

QuantizedImage::QuantizedImage(uint8_t* Ptr, int Len, int QuantizedBitLength, bool Invert)
	: Label(0xFF)
{
	if (QuantizedBitLength > 8)
		throw exception("QuantizedBitLength cannot exceed 8");

	int bit_count = Len * QuantizedBitLength;
	int arr_len = bit_count / 8;

	// Add an extra byte if not a multiple of 8
	if (bit_count % 8 != 0)
		arr_len++;

	Data = new uint8_t[arr_len];
	Length = arr_len;
	NumPixels = Len;

	Pack(Ptr, Len, QuantizedBitLength, Invert);
}

void QuantizedImage::Pack(uint8_t* ptr, int len, int q, bool invert)
{
	uint8_t acc = 0;
	int shift = 8;
	int cursor = 0;
	int levels = 1 << q;

	for (int i = 0; i < len; i++)
	{
		uint8_t val = invert ? 255 - ptr[i] : ptr[i];
		uint8_t pixel = Quantize(val, levels);
		shift -= q;

		if (shift >= 0)
		{
			acc = acc | (pixel << shift);

			if (shift == 0)
			{
				Data[cursor++] = acc;
				acc = 0;
				shift = 8;
			}
		}
		else
		{
			// This else block is only visited when QuantizedBitLength isn't an even number
			// and it won't be visited back to back either (since Q <= 8)

			int right = q + shift;
			acc = acc | (pixel >> right);
			Data[cursor++] = acc;

			shift = 8 - right;
			acc = pixel << shift;
		}
	}
}

void QuantizedImage::Unpack(uint8_t* dest, int len, int q, int scale) const
{
	uint8_t pixel = 0;
	uint8_t mask;
	int shift = 8;
	int cursor = 0;

	for (int i = 0; i < NumPixels; i++)
	{
		mask = shift < 8 ? (1 << shift) - 1 : 0xFF;
		shift -= q;

		if (shift >= 0)
		{
			pixel = (Data[cursor] & mask) >> shift;
			pixel *= scale;
			dest[i] = pixel;
			pixel = 0;

			if (shift == 0)
			{
				cursor++;
				shift = 8;
			}
		}
		else
		{
			assert(i != Length - 1);

			int left = -1 * shift;
			pixel = Data[cursor] << left;

			shift = 8 - left;
			pixel |= Data[cursor + 1] >> shift;
			pixel *= scale;
			dest[i] = pixel;
			pixel = 0;
			cursor++;
		}
	}
}

QuantizedImage::~QuantizedImage()
{
	if (Data != nullptr) delete[] Data;
}
