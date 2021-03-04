
#pragma once

#include <cstdint>

namespace sphere
{
	class QuantizedImage
	{
	public:
		QuantizedImage(uint8_t* Ptr, int Len, int QuantizedBitLength, bool Invert = true);
		QuantizedImage(const QuantizedImage& Other);

		int DistanceTo(const QuantizedImage& Other);
		static uint8_t Quantize(uint8_t Value, int Levels);

		const bool IsLabelled() const { return Label != 0xFF; }

		~QuantizedImage();

		uint8_t* Data;
		int Length;
		int NumPixels;
		std::uint8_t Label;

		void Unpack(uint8_t* dest, int len, int q, int scale = 1) const;
	private:
		void Pack(uint8_t* ptr, int len, int q, bool invert);
	};
}