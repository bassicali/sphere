
#include <cassert>
#include <chrono>
#include <random>
#include <intrin.h>

#include "Word.h"

using namespace std;
using namespace sphere;

Word::Word()
	: wordLen(0)
	, rangeBitLen(0)
	, rangeSize(0)
	, numSubWords(0)
	, lastSubwordLen(0)
{
}

/**
 Construct a random word of a given bit length N
*/
Word::Word(int N, int RangeBits) 
	: wordLen(N)
	, rangeBitLen(RangeBits)
{
	rangeSize = uint8_t(1 << rangeBitLen);
	int total_len = N * RangeBits;

	numSubWords = total_len / SUBWORD_NUM_DIMENSIONS;
	lastSubwordLen = total_len % SUBWORD_NUM_DIMENSIONS;

	if (lastSubwordLen > 0)
		numSubWords++;

	subwords = vector<SUBWORD>(numSubWords);
	RandomizeSubwords(subwords);

	assert(subwords.size() == numSubWords);
}

/**
* Construct from raw binary data
*/
Word::Word(int N, int RangeBits, uint8_t* Ptr, int Len)
	: wordLen(N)
	, rangeBitLen(RangeBits)
{
	rangeSize = uint8_t(1 << rangeBitLen);
	int total_len = N * RangeBits;

	numSubWords = total_len / SUBWORD_NUM_DIMENSIONS;
	lastSubwordLen = total_len % SUBWORD_NUM_DIMENSIONS;

	if (lastSubwordLen > 0)
	{
		numSubWords++;
	}

	static_assert(sizeof(SUBWORD) / sizeof(uint8_t) == 4, "Cannot work with this subword size");

	int mod = Len % 4;
	SUBWORD sw;

	for (int i = 0; i < Len; i += 4)
	{
		if (mod != 0 && (i + 4) >= Len)
		{
			switch (mod)
			{
				case 3:
					sw = (Ptr[i] << 24) | (Ptr[i + 1] << 16) | (Ptr[i + 2] << 8);
					break;
				case 2:
					sw = (Ptr[i] << 24) | (Ptr[i + 1] << 16);
					break;
				case 1:
					sw = Ptr[i] << 24;
					break;
			}
		}
		else
		{
			sw = (Ptr[i] << 24) | (Ptr[i + 1] << 16) | (Ptr[i + 2] << 8) | Ptr[i + 3];
		}

		subwords.push_back(sw);
	}

	PadVector(subwords, numSubWords);

	assert(subwords.size() == numSubWords);
}

/**
 Construct from an string
*/
Word::Word(int N, int RangeBits, string StringData)
	: wordLen(N)
	, rangeBitLen(RangeBits)
{
	rangeSize = uint8_t(1 << rangeBitLen);
	numSubWords = wordLen / SUBWORD_NUM_DIMENSIONS;
	lastSubwordLen = wordLen % SUBWORD_NUM_DIMENSIONS;

	if (lastSubwordLen > 0)
		numSubWords++;

	if (StringData.length() > (N / sizeof(char)))
		throw exception("StringData too large for word size");

	static_assert(sizeof(SUBWORD) / sizeof(string::value_type) == 4, "Cannot work with this subword size");

	vector<char> acc;
	for (int i = 0; i < StringData.length(); i++)
	{
		acc.push_back(StringData[i]);

		if (acc.size() == 4)
		{
			SUBWORD sw = (acc[0] << 24) | (acc[1] << 16) | (acc[2] << 8) | acc[3];
			subwords.push_back(sw);
			acc.clear();
		}
	}

	if (acc.size() > 0)
	{
		SUBWORD sw = 0;
		for (int j = 0; j < acc.size(); j++)
		{
			sw = sw | (acc[j] << (32 - (j + 1) * 8));
		}
		subwords.push_back(sw);
	}

	PadVector(subwords, numSubWords);

	assert(subwords.size() == numSubWords);
}

/**
 Private constructor for supplying the subwords array
*/
Word::Word(int N, int RangeBits, vector<SUBWORD>& SubWords)
	: wordLen(N)
	, rangeBitLen(RangeBits)
{
	int total_len = N * RangeBits;

	numSubWords = total_len / SUBWORD_NUM_DIMENSIONS;
	lastSubwordLen = total_len % SUBWORD_NUM_DIMENSIONS;

	if (lastSubwordLen > 0)
		numSubWords++;

	subwords = move(SubWords);

	assert(subwords.size() == numSubWords);
}

const float Word::DistanceTo(const Word& Other) const
{
	if (Other.Length() != Length())
	{
		throw exception("Incompatible word lengths");
	}

	int sub_len = subwords.size();
	float dist = 0.0f;

	if (rangeBitLen == 1)
	{
		// When each dimension is 1 bit, use hamming distance

		for (int i = 0; i < sub_len; i++)
		{
			SUBWORD xored = subwords[i] ^ Other.SubwordAt(i);

			if (lastSubwordLen > 0 && i == sub_len - 1)
			{
				// TODO: need this?
				xored = xored & ((1 << lastSubwordLen) - 1);
			}

			dist += __popcnt(xored);
		}
	}
	else
	{
		int ints_per_sw = SUBWORD_NUM_DIMENSIONS / rangeBitLen;
		const uint8_t base_mask = (1 << rangeBitLen) - 1;
		float running_sum = 0.0f;
		int shift;

		for (int i = 0; i < sub_len; i++)
		{
			for (int j = 0; j < ints_per_sw; j++)
			{
				int shift = 32 - (j + 1) * rangeBitLen;
				uint32_t mask = base_mask << shift;

				uint8_t value_this = (subwords[i] & mask) >> shift;
				uint8_t value_other = (Other.SubwordAt(i) & mask) >> shift;

				uint8_t dist_1 = (value_this - value_other) % rangeSize;
				uint8_t dist_2 = (value_other - value_this) % rangeSize;

				if (dist_1 <= dist_2)
				{
#if MANHATTAN_DISTANCE
					running_sum += dist_1;
#else
					running_sum += (dist_1 * dist_1);
#endif
				}
				else
				{
#if MANHATTAN_DISTANCE
					running_sum += dist_2;
#else
					running_sum += (dist_2 * dist_2);

#endif
				}
			}
		}

#if MANHATTAN_DISTANCE
		dist = running_sum;
#else
		dist = sqrtf(running_sum);
#endif
	}

	return dist;
}

/*static*/
Word Word::FromCounters(const vector<COUNTER>& counters, int RangeLen, bool& Conclusive)
{
	Conclusive = false;
	int word_len = counters.size() / (1 << RangeLen);

	if (RangeLen == 1)
	{
		Conclusive = true;
		vector<SUBWORD> subwords;
		int bit_index = 0;
		SUBWORD w = 0;

		for (int i = 0; i < word_len; i++)
		{
			bit_index = i % SUBWORD_NUM_DIMENSIONS;

			if (i > 0 && bit_index == 0)
			{
				subwords.push_back(w);
				w = 0;
			}

			if (counters[i] > 0)
			{
				w = w | (1 << bit_index);
			}
			else if (counters[i] < 0)
			{
				w = w & ~(1 << bit_index);
			}
			else
			{
				if (RandomBit())
					w = w | (1 << bit_index);
				else
					w = w & ~(1 << bit_index);
			}
		}

		return Word(word_len, RangeLen, subwords);
	}
	else
	{
		vector<uint8_t> values;
		int range_size = 1 << RangeLen;

		for (int ctr_idx = 0; ctr_idx < counters.size();)
		{
			int max = 0;
			uint8_t value_at_max = 0;
			
			for (uint8_t val = 0; val < range_size; val++, ctr_idx++)
			{
				if (counters[ctr_idx] > max)
				{
					max = counters[ctr_idx];
					value_at_max = val;
					Conclusive = true;
				}
			}

			values.push_back(value_at_max);
		}

		if (!Conclusive)
		{
			return Word();
		}

		assert(values.size() == word_len);
		
		// Pack the individual values into subwords

		vector<SUBWORD> subwords;
		int ints_per_sw = SUBWORD_NUM_DIMENSIONS / RangeLen;
		int values_len = values.size();

		for (int val_index = 0; val_index < values.size();)
		{
			SUBWORD sw = 0;

			for (int j = 0; j < ints_per_sw; j++)
			{
				int shift = SUBWORD_NUM_DIMENSIONS - ((j + 1) * RangeLen);
				sw = sw | (values[val_index] << shift);

				if (shift == 0)
				{
					subwords.push_back(sw);
					sw = 0;
				}

				val_index++;
			}
		}

		return Word(word_len, RangeLen, subwords);
	}

}

void Word::Serialize(ostream& stream)
{
	STREAM_WRITE_INT16(stream, wordLen);
	STREAM_WRITE_INT8(stream, rangeBitLen);
	STREAM_WRITE_INT16(stream, numSubWords);
	STREAM_WRITE_INT16(stream, lastSubwordLen);

	for (int i = 0; i < numSubWords; i++)
	{
		STREAM_WRITE_SUBWORD(stream, subwords[i]);
	}
}

/**
 Deserialize from stream
*/
Word::Word(istream& stream)
{
	assert(subwords.size() == 0);

	STREAM_READ_INT16(stream, wordLen);
	STREAM_READ_INT8(stream, rangeBitLen);
	rangeSize = 1 << rangeBitLen;
	STREAM_READ_INT16(stream, numSubWords);
	STREAM_READ_INT16(stream, lastSubwordLen);

	SUBWORD sw;
	for (int i = 0; i < numSubWords; i++)
	{
		STREAM_READ_SUBWORD(stream, sw);
		subwords.push_back(sw);
	}
}

/*static*/
bool Word::RandomBit()
{
	static uniform_int_distribution<SUBWORD> dist(0, 1);
	static mt19937 rng = CreateRng();

	return dist(rng) == 1;
}

/*static*/
void Word::RandomizeSubwords(vector<SUBWORD>& subwords)
{
	static uniform_int_distribution<SUBWORD> dist;
	static mt19937 rng = CreateRng();

	int len = subwords.size();
	for (int i = 0; i < len; i++)
	{
		subwords[i] = dist(rng);
	}
}

void Word::PadVector(vector<SUBWORD>& subwords, int len)
{
	int diff = len - subwords.size();
	if (diff > 0)
	{
		for (int i = 0; i < diff; i++)
		{
			subwords.push_back(0);
		}
	}
}

const uint8_t Word::IntAt(int index) const
{
	int bIndex = index * rangeBitLen;
	int swIndex = bIndex / SUBWORD_NUM_DIMENSIONS;

	int indexInSw = bIndex % SUBWORD_NUM_DIMENSIONS;
	int distFromRight = SUBWORD_NUM_DIMENSIONS - indexInSw - rangeBitLen + 1;
	SUBWORD mask = ((1 << rangeBitLen) - 1) << distFromRight;

	uint8_t result = uint8_t((subwords[swIndex] & mask) >> distFromRight);

	return result;
}

void Word::EnumerateInts(std::function<void(int, uint8_t)> func) const
{
	int ints_per_sw = SubWordLength() / rangeBitLen;
	assert(ints_per_sw % 2 == 0); // Add odd lenghts later
	int shift;
	uint8_t mask = (1 << rangeBitLen) - 1;
	uint8_t integer;
	int integer_index = 0;

	for (const SUBWORD& sw : subwords)
	{
		for (int i = 0; i < ints_per_sw; i++)
		{
			shift = SubWordLength() - (rangeBitLen * (i + 1));
			integer = (sw >> shift) & mask;
			func(integer_index, integer);
			integer_index++;
		}
	}
}

void Word::Imprint(const Word& other, float scale, int iterations)
{
	assert(other.wordLen == wordLen && other.rangeBitLen == rangeBitLen);

	int ints_per_sw = SubWordLength() / rangeBitLen;
	assert(ints_per_sw % 2 == 0);
	int shift;
	uint8_t mask = (1 << rangeBitLen) - 1;
	int integer_index = 0;

	for (int i = 0; i < subwords.size(); i++)
	{
		SUBWORD sw0 = subwords[i];
		SUBWORD sw1 = other.subwords[i];
		SUBWORD sw_new = 0;

		for (int j = 0; j < ints_per_sw; j++)
		{
			shift = SubWordLength() - (rangeBitLen * (j + 1));
			uint8_t int0 = (sw0 >> shift) & mask;
			uint8_t int1 = (sw1 >> shift) & mask;

			int imprinted;
			for (int k = 0; k < iterations; k++)
			{
				imprinted = int0 + ((int1 - int0) * scale);

				uint8_t int_new;
				if (imprinted >= UINT8_MAX)
					int0 = UINT8_MAX;
				else if (imprinted <= 0)
					int0 = 0;
				else
					int0 = imprinted;
			}

			sw_new = sw_new | (int0 << shift);
		}

		subwords[i] = sw_new;
	}
}

/*static*/
mt19937 Word::CreateRng()
{
	using namespace chrono;

	random_device dev;
	long seed = 0;
	try
	{
		seed = dev();
	}
	catch (exception& ex)
	{
		seed = (duration_cast<milliseconds>(system_clock::now().time_since_epoch())).count();
	}

	return mt19937(seed);
}
