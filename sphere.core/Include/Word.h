
#pragma once

#include <cstdint>
#include <vector>
#include <random>
#include <functional>

#include "DArray.h"
#include "ISerializable.h"

typedef uint32_t SUBWORD;
#define SUBWORD_NUM_DIMENSIONS (8*sizeof(SUBWORD))
#define DECREMENT_UNMATCHED 0
#define MANHATTAN_DISTANCE 0

#if DECREMENT_UNMATCHED
typedef int8_t COUNTER;
#define COUNTER_MIN INT8_MIN
#define COUNTER_MAX INT8_MAX
#else
typedef uint16_t COUNTER;
#define COUNTER_MIN 0
#define COUNTER_MAX UINT16_MAX
#endif

namespace sphere
{
	class Word : public ISerializable
	{
	public:
		Word();

		Word(int N, int RangeBits);
			
		Word(int N, int RangeBits, std::string Data);
		Word(int N, int RangeBits, uint8_t* Ptr, int Len);

		Word(std::istream& stream);

		static Word FromCounters(const std::vector<COUNTER>& counters, int RangeLen, bool& Conclusive);

		const float DistanceTo(const Word& Other) const;

		const int Length() const { return wordLen; }
		const int RangeLength() const { return rangeBitLen; }
		const int RangeSize() const { return rangeSize; }
		const int SubWordLength() const { return sizeof(SUBWORD) * 8; }
		const int NumSubWords() const { return numSubWords; }
		const SUBWORD SubwordAt(int index) const { return subwords[index]; }
		const uint8_t IntAt(int index) const;
		void EnumerateInts(std::function<void(int, uint8_t)> func) const;
		void Imprint(const Word& other, float scale, int iterations);

		static bool RandomBit();

		virtual void Serialize(std::ostream& stream) override;

	private:
		Word(int N, int RangeBits, std::vector<SUBWORD>& subwords);

		uint16_t wordLen;
		uint8_t rangeBitLen;
		uint8_t rangeSize;

		uint16_t numSubWords;
		uint16_t lastSubwordLen;

		std::vector<SUBWORD> subwords;

		static std::mt19937 CreateRng();
		static void RandomizeSubwords(std::vector<SUBWORD>& subwords);

		static void PadVector(std::vector<SUBWORD>& arr, int len);
	};
}