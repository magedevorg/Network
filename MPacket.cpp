#include "MPacket.h"


// 메모리 풀
MPool<MMemory> MPacket::MemoryPool;



//-----------------------------------------------------------
// MPacketConnectResponse
//-----------------------------------------------------------
void MPacketConnectResponse::Serialize(MStream& inStream)
{
	inStream.Process(&Error, sizeof(Error));
}

