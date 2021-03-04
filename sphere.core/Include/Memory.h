#pragma once

#include <vector>

#include "ISerializable.h"

#include "HardLocation.h"
#include "Word.h"

#define FILE_PREFIX "?!SPHERE!?"
#define FILE_PREFIX_LEN (sizeof(FILE_PREFIX)/sizeof(char))

namespace sphere
{
	class Memory : public ISerializable
	{
	public:
		Memory();
		Memory(std::istream& stream);

		void Initialize(int WordSize, int RangeLen, int NumHardLocations, int Radius);
		void InitializeFixedHardLocations(int WordLen, int RangeLen, int NumHardLocations, int Radius, const std::vector<Word>& Addrs);

		bool Write(const Word& Addr, const Word& Data);
		Word Read(const Word& Addr);

		int RangeBitLength() const { return rangeLen; }
		std::vector<HardLocation>& HardLocations() { return storage; }

		void SaveToFile(const std::string& FilePath);
		static Memory LoadFromFile(const std::string& FilePath);

		virtual void Serialize(std::ostream& stream) override;
	private:
		Memory(int WordSize, int NumHardLocations, int Radius, std::vector<HardLocation> Storage);
			
		std::vector<HardLocation> storage;
		int radius;
		int wordLen;
		int rangeLen;
		int writeCount;
		bool initialized;
	};
}