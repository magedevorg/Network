#include "MIOModelUtil.h"


void MIOModelUtil::PushPacket(MBuffer& inBuffer, MUINT32 inPacketID, MPacket& inPacket, MMemory& inPacketMemory)
{
	// 패킷을 시리얼라이즈
	MMemoryWriteStream writeStream(inPacketMemory);
	writeStream.Process(inPacket);

	MPacketHeader header;
	header.PacketID = inPacketID;
	header.PacketSize = writeStream.GetPos();

	// 헤더 푸시
	inBuffer.Push(&header, sizeof(header), MTRUE);

	// 데이터 push
	if (0 < header.PacketSize) {
		inBuffer.Push(inPacketMemory.GetPointer(), header.PacketSize, MTRUE);
	}
}

MBOOL MIOModelUtil::PopPacket(MPacketHeader& inHeader, class MMemory& inData, class MBuffer& inBuffer)
{
	const MSIZE dataSize = inBuffer.GetReadableSize();
	if (dataSize < sizeof(inHeader)) {
		return MFALSE;
	}

	// 헤더를 읽는다
	inBuffer.Read(&inHeader, sizeof(inHeader));

	// 데이터가 전부오지 않았다면 넘어간다
	if ((dataSize - sizeof(inHeader)) < inHeader.PacketSize) {
		return MFALSE;
	}

	// 헤더 팝
	inBuffer.Read(&inHeader, sizeof(inHeader));
	inBuffer.Pop(sizeof(inHeader));

	// 데이터 팝
	if (0 < inHeader.PacketSize)
	{
		inBuffer.Read(inData.GetPointer(), inHeader.PacketSize);
		inBuffer.Pop(inHeader.PacketSize);
	}
	return MTRUE;
}