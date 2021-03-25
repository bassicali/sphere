#pragma once

#include <vector>

#include "ISerializable.h"

#include "HardLocation.h"
#include "Word.h"

#define FILE_PREFIX "?!SPHERE!?"
#define FILE_PREFIX_LEN (sizeof(FILE_PREFIX)/sizeof(char))

namespace sphere
{
	struct RWStats
	{
		int Activations;
		float AverageDistance;
		float MinimumDistance;
	};

	class Memory : public ISerializable
	{
	public:
		Memory();
		Memory(std::istream& stream);

		void Initialize(int AddrWordDims, int DataWordDims, int RangeBitLen, int NumHardLocations, int Radius);
		void InitializeFixedHardLocations(int AddrWordDims, int DataWordDims, int RangeBitLen, int NumHardLocations, int Radius, const std::vector<Word>& Addrs);

		bool Write(const Word& Addr, const Word& Data);
		Word Read(const Word& Addr, bool& Conclusive);

		int RangeBitLength() const { return rangeLen; }
		std::vector<HardLocation>& HardLocations() { return storage; }

		void SaveToFile(const std::string& FilePath);
		static Memory LoadFromFile(const std::string& FilePath);

		RWStats LastOPStats;

		virtual void Serialize(std::ostream& stream) override;
	private:
		Memory(int WordSize, int NumHardLocations, int Radius, std::vector<HardLocation> Storage);
			
		std::vector<HardLocation> storage;
		int radius;
		int addrDims;
		int dataDims;
		int rangeLen;
		int writeCount;
		bool initialized;

	};
}