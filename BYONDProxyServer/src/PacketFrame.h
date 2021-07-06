#pragma once
#include <vector>


class PacketFrame
{
	public:
		bool hasSeq;
		uint16_t sequence;
		uint16_t type;
		uint16_t len;
		std::vector<unsigned char> data;


		PacketFrame()
		{
			hasSeq = false;
			sequence = 0;
			type = 0;
			len = 0;
			data.reserve(0xFFFF);
			data.resize(0xFFFF);
		}

		void read(char buf[])
		{
			uint16_t offset = 0;
			if (hasSeq)
			{
				memcpy(&sequence, &buf[0], sizeof(uint16_t));
				offset = 2;
			}
			memcpy(&type, &buf[0+offset], sizeof(uint16_t));
			type = _byteswap_ushort(type);
			
			memcpy(&len, &buf[2+offset], sizeof(uint16_t));
			len = _byteswap_ushort(len);
			memcpy(&data[0], &buf[4 + offset], len*sizeof(char));
		}
};