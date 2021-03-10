
#pragma once

#include <istream>
#include <ostream>

#define STREAM_WRITE_INT32(s,d) s.write(reinterpret_cast<char*>(&d),sizeof(uint32_t))
#define STREAM_READ_INT32(s,d) if(!s.eof()){s.read(reinterpret_cast<char*>(&d),sizeof(uint32_t));}else{throw exception("Input stream ended to early");}

#define STREAM_WRITE_INT16(s,d) s.write(reinterpret_cast<char*>(&d),sizeof(uint16_t))
#define STREAM_READ_INT16(s,d) if(!s.eof()){s.read(reinterpret_cast<char*>(&d),sizeof(uint16_t));}else{throw exception("Input stream ended to early");}

#define STREAM_WRITE_INT8(s,d) s.write(reinterpret_cast<char*>(&d),sizeof(int8_t))
#define STREAM_READ_INT8(s,d) if(!s.eof()){s.read(reinterpret_cast<char*>(&d),sizeof(int8_t));}else{throw exception("Input stream ended to early");}

#define STREAM_WRITE_SUBWORD(s,d) s.write(reinterpret_cast<char*>(&d),sizeof(uint32_t))
#define STREAM_READ_SUBWORD(s,d) if(!s.eof()){s.read(reinterpret_cast<char*>(&d),sizeof(SUBWORD));}else{throw exception("Input stream ended to early");}

class ISerializable
{
public:
	virtual void Serialize(std::ostream& stream) = 0;
};