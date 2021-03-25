
#include <fstream>
#include <cstring>

#include "Common.h"
#include "Memory.h"

using namespace std;
using namespace sphere;

Memory::Memory()
	: addrDims(0)
	, dataDims(0)
	, rangeLen(0)
	, radius(0)
	, writeCount(0)
	, storage()
	, initialized(false)
{
}

void Memory::Initialize(int AddrWordDims, int DataWordDims, int RangeBitLen, int NumHardLocations, int Radius)
{
	if (initialized) 
		throw exception("Memory cannot be initialized more than once");

	addrDims = AddrWordDims;
	dataDims = DataWordDims;
	rangeLen = RangeBitLen;
	radius = Radius;

	for (int i = 0; i < NumHardLocations; i++)
	{
		storage.push_back(HardLocation(addrDims, dataDims, rangeLen));
	}

	initialized = true;
}

void Memory::InitializeFixedHardLocations(int AddrWordDims, int DataWordDims, int RangeBitLen, int NumHardLocations, int Radius, const vector<Word>& Addrs)
{
	if (initialized)
		throw exception("Memory cannot be initialized more than once");

	if (Addrs.size() < NumHardLocations)
		throw exception("Not enough hard locations provided");

	addrDims = AddrWordDims;
	dataDims = DataWordDims;
	rangeLen = RangeBitLen;
	radius = Radius;
	for (int i = 0; i < NumHardLocations; i++)
	{
		const Word& addr = Addrs[i];
		storage.push_back(HardLocation(addrDims, dataDims, rangeLen));
	}

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

	LastOPStats.Activations = activated;
	LastOPStats.AverageDistance = sum / len;
	LastOPStats.MinimumDistance = min;
	
	// TODO: return false when at capacity
	writeCount++;
	return true;
}

Word Memory::Read(const Word& Addr, bool& Conclusive)
{
	if (!initialized) 
		throw exception("Memory has not been initialized");

	int len = storage.size();
	float sum = 0.0f;
	float min = FLT_MAX;
	int activated = 0;

	// TODO: impl iterative reading based on the SD of activated locations

	vector<COUNTER> counters(dataDims * (1 << rangeLen), 0);

	// Find each HL that's within the activation radius and accumulate its values to the counters array
	for (int i = 0; i < len; i++)
	{
		float dist = Addr.DistanceTo(storage[i].Address());
		
		if (dist <= radius)
		{
			storage[i].Read(counters);
			activated++;
		}

		sum += dist;

		if (dist < min)
			min = dist;
	}

	LastOPStats.Activations = activated;
	LastOPStats.AverageDistance = sum / len;
	LastOPStats.MinimumDistance = min;

	return Word::FromCounters(counters, rangeLen, Conclusive);
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
	STREAM_WRITE_INT32(stream, addrDims);
	STREAM_WRITE_INT32(stream, dataDims);
	STREAM_WRITE_INT32(stream, rangeLen);
	STREAM_WRITE_INT32(stream, radius);
	STREAM_WRITE_INT32(stream, writeCount);

	int hl_count = storage.size();
	STREAM_WRITE_INT32(stream, hl_count);

	int hl_idx = 0;
	for (HardLocation& hl : storage)
	{
		hl.Serialize(stream);

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

	STREAM_READ_INT32(stream, addrDims)
	STREAM_READ_INT32(stream, dataDims)
	STREAM_READ_INT32(stream, rangeLen)
	STREAM_READ_INT32(stream, radius)
	STREAM_READ_INT32(stream, writeCount)

	int hl_count;
	STREAM_READ_INT32(stream, hl_count)

	for (int idx = 0; idx < hl_count; idx++)
	{
		storage.push_back(HardLocation(stream));

		if (idx % (hl_count / 10) == 0)
		{
			float progress = (float(idx) / hl_count) * 100;
			LOG_INFO("Load progress: %.0f%%", progress);
		}
	}

	LOG_INFO("Load completed. Memory has %d total writes", writeCount);

	initialized = true;
}