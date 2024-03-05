#pragma once


#include "MPacket.h"


class MIOModelUtil
{
public:
	static void PushPacket(MBuffer& inBuffer, MUINT32 inPacketID, MPacket& inPacket, MMemory& inPacketMemory);
    static MBOOL PopPacket(MPacketHeader& inHeader, class MMemory& inData, class MBuffer& inBuffer);
};