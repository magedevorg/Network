#include "MIOModelUtil.h"


void MIOModelUtil::PushPacket(MBuffer& inBuffer, MUINT32 inPacketID, MPacket& inPacket, MMemory& inPacketMemory)
{
	// ��Ŷ�� �ø��������
	MMemoryWriteStream writeStream(inPacketMemory);
	writeStream.Process(inPacket);

	MPacketHeader header;
	header.PacketID = inPacketID;
	header.PacketSize = writeStream.GetPos();

	// ��� Ǫ��
	inBuffer.Push(&header, sizeof(header), MTRUE);

	// ������ push
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

	// ����� �д´�
	inBuffer.Read(&inHeader, sizeof(inHeader));

	// �����Ͱ� ���ο��� �ʾҴٸ� �Ѿ��
	if ((dataSize - sizeof(inHeader)) < inHeader.PacketSize) {
		return MFALSE;
	}

	// ��� ��
	inBuffer.Read(&inHeader, sizeof(inHeader));
	inBuffer.Pop(sizeof(inHeader));

	// ������ ��
	if (0 < inHeader.PacketSize)
	{
		inBuffer.Read(inData.GetPointer(), inHeader.PacketSize);
		inBuffer.Pop(inHeader.PacketSize);
	}
	return MTRUE;
}