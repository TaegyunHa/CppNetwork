#pragma once
#include <cstdint>

#define LOOPBACK_IPv4 "127.0.0.1" // host name
#define DEFAULT_PORT "27015"
// #define DEFAULT_BUFLEN 512

#define DEFAULT_BUFLEN 2048

namespace pkt
{
// Custom Data structure
typedef struct _Data
{
	char testStr[DEFAULT_BUFLEN];
} Data;

// Custom Data structure
typedef struct _StringData
{
	uint8_t stringId;
	char firstString[512];
	char secondString[512];
} StringData;

typedef struct _CustomData
{
	uint8_t uint8Val;
	int64_t int64Val;
	float floatVal;
	double doubleVal;
} CustomData;

// All packet must have a header at the begining
typedef struct _Header
{
	uint8_t versionMajor;
	uint8_t versionMinor;
	uint8_t packetType;
	uint32_t seq;
	uint32_t size;
} Header;

// Packet with data
typedef struct _Packet
{
	Header header;
	Data data;
	uint8_t checksum;
} Packet;

typedef struct _CustomPacket
{
	Header header;
	CustomData data;
	uint8_t checksum;
} CustomPacket;

typedef struct _StringPacket
{
	Header header;
	StringData data;
	uint8_t checksum;
} StringPacket;

// get a entire packet size from the header


}// namespace pkt end
