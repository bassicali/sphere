
#include <fstream>
#include <cstring>

#include "Common.h"
#include "Memory.h"

using namespace std;
using namespace sphere;

Memory::Memory()
	: wordLen(0)
	, rangeLen(0)
	, radius(0)
	, writeCount(0)
	, storage()
	, initialized(false)
{
}

void Memory::Initialize(int WordLen, int RangeLen, int NumHardLocations, int Radius)
{
	if (initialized) 
		throw exception("Memory cannot be initialized more than once");

	wordLen = WordLen;
	rangeLen = RangeLen;
	radius = Radius;

	for (int i = 0; i < NumHardLocations; i++)
	{
		storage.push_back(HardLocation(wordLen, rangeLen));
	}

	//auto& ref = storage[0];
	//for (int i = 0; i < 100; i++)
	//{
	//	float dist = ref.Address().DistanceTo(storage[i].Address());
	//	LOG_INFO("Dist from addr_0 to addr_%d: %.2f", i, dist);
	//}
	//
	initialized = true;
}

void Memory::InitializeFixedHardLocations(int WordLen, int RangeLen, int NumHardLocations, int Radius, const vector<Word>& Addrs)
{
	if (initialized)
		throw exception("Memory cannot be initialized more than once");

	if (Addrs.size() < NumHardLocations)
		throw exception("Not enough hard locations provided");

	wordLen = WordLen;
	rangeLen = RangeLen;
	radius = Radius;
	for (int i = 0; i < NumHardLocations; i++)
	{
		const Word& addr = Addrs[i];
		storage.push_back(HardLocation(addr));
	}
	storage = vector<HardLocation>(NumHardLocations, HardLocation(wordLen, rangeLen));

	initialized = true;
}

bool Memory::Write(const Word& Addr, const Word& Data)
{
	if (!initialized)
		throw exception("Memory has not been initialized");

	// TODO: optimize this search
	float sum = 0.0f;
	float min = FLT_MAX;
	int activated = 0;

	int len = storage.size();

	for (int i = 0; i < len; i++)
	{
		float dist = Addr.DistanceTo(storage[i].Address());

		if (dist <= radius)
		{
			storage[i].Write(Data);
			activated++;
		}

		sum += dist;

		if (dist < min) 
			min = dist;
	}

	LOG_INFO("[WRITE STATS] Activated: %d (%.2f) \t| D_avg: %.2f \t| D_min: %.2f", activated, float(activated) / storage.size(), sum / len, min);
	
	// TODO: return false when at capacity
	writeCount++;
	return true;
}

Word Memory::Read(const Word& Addr)
{
	if (!initialized) 
		throw exception("Memory has not been initialized");

	int len = storage.size();

	// TODO: impl iterative reading based on the SD of activated locations

	vector<COUNTER> counters(Addr.Length() * Addr.RangeSize(), 0);

	// Find each HL that's within the activation radius and accumulate its values to the counters array

	for (int i = 0; i < len; i++)
	{
		int d = Addr.DistanceTo(storage[i].Address());
		
		if (d <= radius)
		{
			storage[i].Read(counters);
		}
	}

	return Word::FromCounters(counters, rangeLen);
}

void Memory::SaveToFile(const string& FilePath)
{
	ofstream fout(FilePath, ios_base::binary);

	if (fout.fail())
	{
		throw exception("Could not open output file for writing");
	}

	Serialize(fout);
	float mbytes = float(fout.tellp()) / (1024 * 1024);
	fout.close();
	LOG_INFO("Saved memory to %s (size: %.2fMB)", FilePath.c_str(), mbytes);
}

Memory Memory::LoadFromFile(const string& FilePath)
{
	ifstream fin(FilePath, ios_base::binary);

	if (fin.fail())
	{
		throw exception("Could not open input file for reading");
	}

	Memory mem(fin);
	fin.close();

	return mem;
}

void Memory::Serialize(ostream& stream)
{
	stream.write(FILE_PREFIX, FILE_PREFIX_LEN);
	STREAM_WRITE_INT32(stream, wordLen);
	STREAM_WRITE_INT32(stream, rangeLen);
	STREAM_WRITE_INT32(stream, radius);
	STREAM_WRITE_INT32(stream, writeCount);

	int loc_count = storage.size();
	STREAM_WRITE_INT32(stream, loc_count);

	int hl_idx = 0;
	for (HardLocation& loc : storage)
	{
		loc.Serialize(stream);

		if (++hl_idx % (storage.size() / 10) == 0)
		{
			float progress = (float(hl_idx) / storage.size()) * 100;
			LOG_INFO("Save progress: %.0f%%", progress);
		}
	}
}

Memory::Memory(istream& stream)
{
	char buffer[FILE_PREFIX_LEN];

	stream.read(buffer, FILE_PREFIX_LEN);

	if (strncmp(buffer, FILE_PREFIX, FILE_PREFIX_LEN) != 0)
	{
		throw exception("Invalid file; prefix not found.");
	}

	STREAM_READ_INT32(stream, wordLen)
	STREAM_READ_INT32(stream, rangeLen)
	STREAM_READ_INT32(stream, radius)
	STREAM_READ_INT32(stream, writeCount)

	int loc_count;
	STREAM_READ_INT32(stream, loc_count)

	for (int idx = 0; idx < loc_count; idx++)
	{
		storage.push_back(HardLocation(stream));

		if (idx % (loc_count / 10) == 0)
		{
			float progress = (float(idx) / loc_count) * 100;
			LOG_INFO("Load progress: %.0f%%", progress);
		}
	}

	initialized = true;
}