
#include <cstring>
#include <cmath>

#include "HardLocation.h"

using namespace std;
using namespace sphere;

uint32_t HardLocation::NumInstances = 0;

HardLocation::HardLocation(int WordLen, int RangeLen)
	: addr(WordLen, RangeLen)
	, writes(0)
	, id(NumInstances)
{
	int range_size = 1 << RangeLen;
	counters = vector<COUNTER>(WordLen * range_size);
	NumInstances++;
}

HardLocation::HardLocation(const Word& Address)
	: addr(Address)
	, counters(addr.Length() * addr.RangeSize(), 0)
	, writes(0)
	, id(NumInstances)
{
	NumInstances++;
}

void HardLocation::Write(const Word& Data)
{
	if (counters.size() != (Data.Length() * addr.RangeSize()))
		throw exception("Invalid number of counters");

	// TODO: optimize

	int sub_len = Data.NumSubWords();
	int range_len = Data.RangeLength();

	if (range_len == 1)
	{
		int ctr_index;
		for (int i = 0; i < sub_len; i++)
		{
			SUBWORD w = Data.SubwordAt(i);
			SUBWORD masked;

			for (int j = 0; j < SUBWORD_NUM_DIMENSIONS; j++)
			{
				ctr_index = i * SUBWORD_NUM_DIMENSIONS + j;
				masked = w & (1 << j);

				if (masked == 0)
				{
					if (counters[ctr_index] > COUNTER_MIN) 
						counters[ctr_index]--;
				}
				else
				{
					if (counters[ctr_index] < COUNTER_MAX) 
						counters[ctr_index]++;
				}
			}
		}
	}
	else
	{
		int ints_per_sw = SUBWORD_NUM_DIMENSIONS / range_len;
		const uint8_t base_mask = (1 << range_len) - 1;
		int range_size = Data.RangeSize();

		for (int i = 0; i < sub_len; i++)
		{
			SUBWORD sw = Data.SubwordAt(i);

			for (int j = 0; j < ints_per_sw; j++)
			{
				int shift = 32 - (j + 1) * range_len;
				uint8_t mask = base_mask << shift;
				uint8_t value = sw & mask;

				int val_index = (i * ints_per_sw) + j;
				int ctr_index = val_index * range_size + value;
				counters[ctr_index]++;
			}
		}
	}

	writes++;
}

void HardLocation::Read(vector<COUNTER>& OutCounters)
{
	int len = counters.size();

	if (len != OutCounters.size())
		throw exception("Incompatible counters lengths");

	int range_len = addr.RangeLength();

	if (range_len == 1)
	{
		for (int i = 0; i < len; i++)
		{
			if (counters[i] > 0)
			{
				if (OutCounters[i] < COUNTER_MAX) 
					OutCounters[i]++;
			}
			else if (counters[i] < 0)
			{
				if (OutCounters[i] > COUNTER_MIN) 
					OutCounters[i]--;
			}
			else
			{
				if (Word::RandomBit())
				{
					if (OutCounters[i] < COUNTER_MAX) 
						OutCounters[i]++;
				}
				else
				{
					if (OutCounters[i] > COUNTER_MIN) 
						OutCounters[i]--;
				}
			}
		}
	}
	else
	{
		int ctr_len = counters.size();

		for (int i = 0; i < ctr_len; i++)
		{
			OutCounters[i] += counters[i];
		}
	}
}

void HardLocation::Serialize(std::ostream& stream)
{
	STREAM_WRITE_INT32(stream, writes);
	addr.Serialize(stream);

	for (COUNTER ctr : counters)
	{
		STREAM_WRITE_INT8(stream, ctr);
	}
}

HardLocation::HardLocation(std::istream& stream)
{
	STREAM_READ_INT32(stream, writes);
	
	addr = Word(stream);
	int ctr_len = addr.Length() * addr.RangeSize();

	COUNTER ctr = 0;
	for (int i = 0; i < ctr_len; i++)
	{
		STREAM_READ_INT8(stream, ctr);
		counters.push_back(ctr);
	}
}