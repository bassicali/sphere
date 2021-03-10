
#include <cstring>
#include <cmath>

#include "HardLocation.h"

using namespace std;
using namespace sphere;

uint32_t HardLocation::NumInstances = 0;

HardLocation::HardLocation(int WordLen, int RangeLen)
	: addr(WordLen, RangeLen)
	, writeCount(0)
	, id(NumInstances)
{
	int range_size = 1 << RangeLen;
	counters = vector<COUNTER>(WordLen * range_size);
	NumInstances++;
}

HardLocation::HardLocation(const Word& Address)
	: addr(Address)
	, counters(addr.Length() * addr.RangeSize(), 0)
	, writeCount(0)
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
				SUBWORD mask = base_mask << shift;
				uint8_t value = (sw & mask) >> shift;

#if DECREMENT_UNMATCHED
				int val_offset = (i * ints_per_sw) + j;
				int ctr_offset = val_offset * range_size;

				for (int k = ctr_offset; k < ctr_offset + range_size; k++)
				{
					if (k == ctr_offset + value)
					{
						if (counters[k] < COUNTER_MAX)
							counters[k]++;
					}
					else
					{
						if (counters[k] > COUNTER_MIN)
							counters[k]--;
					}
				}
#else
				int val_index = (i * ints_per_sw) + j;
				int ctr_index = val_index * range_size + value;
				counters[ctr_index]++;
#endif
			}
		}
	}

	writeCount++;
	dataWritten.push_back(Data.SubwordAt(0) & 0xF);
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
			int sum = OutCounters[i] + counters[i];
			if (sum > COUNTER_MAX)
				OutCounters[i] = COUNTER_MAX;
#if DECREMENT_UNMATCHED
			else if (sum < COUNTER_MIN)
				OutCounters[i] = COUNTER_MIN;
#endif
			else
				OutCounters[i] = (COUNTER)sum;
		}
	}
}

void HardLocation::Serialize(std::ostream& stream)
{
	STREAM_WRITE_INT32(stream, writeCount);
	addr.Serialize(stream);

	for (COUNTER ctr : counters)
	{
		STREAM_WRITE_INT16(stream, ctr);
	}
}

HardLocation::HardLocation(std::istream& stream)
{
	STREAM_READ_INT32(stream, writeCount);
	
	addr = Word(stream);
	int ctr_len = addr.Length() * addr.RangeSize();

	COUNTER ctr = 0;
	for (int i = 0; i < ctr_len; i++)
	{
		STREAM_READ_INT16(stream, ctr);
		counters.push_back(ctr);
	}
}