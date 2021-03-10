
#pragma once

#include <vector>

#include "ISerializable.h"
#include "Word.h"


namespace sphere
{
	class HardLocation : ISerializable
	{
	public:
		HardLocation(int WordSize, int RangeSize);
		HardLocation(const Word& Addr);

		HardLocation(std::istream& stream);

		void Write(const Word& Data);
		void Read(std::vector<COUNTER>& OutCounters);

		int WriteCount() const { return writeCount; }
		const std::vector<uint8_t>& WriteHistory() const { return dataWritten; }

		Word& Address() { return addr; }

		virtual void Serialize(std::ostream& stream) override;

	private:
		static uint32_t NumInstances;

		uint32_t id;
		uint32_t writeCount;
		Word addr;
		std::vector<COUNTER> counters;

		std::vector<uint8_t> dataWritten;
	};
}