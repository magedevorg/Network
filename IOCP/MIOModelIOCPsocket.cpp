#include "MIOModelIOCPSocket.h"
#include "MIOModelUtil.h"

#if (MPLATFORM == MPLATFORM_WINDOWS)

MIOCPSocket::MIOCPSocket(MINDEX inIndex)
	: MSocket(inIndex)
	, RequestSendMessage(this)
	, CompletionAcceptMessage(this)
	, CompletionRecvMessage(this)
	, CompletionSendMessage(this)
{
	SendPushBuffer = new MBuffer(256);
	SendBuffer = new MBuffer(256);

	RecvPushBuffer = new MBuffer(256);
	RecvBuffer = new MBuffer(256);
}

MIOCPSocket::~MIOCPSocket()
{
	delete SendPushBuffer;
	delete SendBuffer;

	delete RecvPushBuffer;
	delete RecvBuffer;
}


// Accept ����
void MIOCPSocket::Accept()
{
	// �޽��� ���� ����
	CompletionAcceptMessage.Clear();
	CompletionAcceptMessage.AcceptSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	DWORD length = 0;

	// accpet ó��
	BOOL result = ::AcceptEx(
		NativeSocket,
		CompletionAcceptMessage.AcceptSocket,
		(LPVOID)CompletionAcceptMessage.Buffer, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		&length,
		&CompletionAcceptMessage);


	if (FALSE == result)
	{
		int error = ::WSAGetLastError();
		switch (error)
		{
		case ERROR_IO_PENDING:
		{
			// ����
		} break;

		default:
		{
			// ����
		}
		}
	}
}

// recv����
void MIOCPSocket::Recv()
{
	DWORD flags = 0;
	DWORD recvBytes = 0;
	::WSARecv(NativeSocket, &CompletionRecvMessage.WSABuffer, 1, &recvBytes, &flags, &CompletionRecvMessage, NULL);
}


void MIOCPSocket::Send(const MBYTE* inData, MSIZE inSize)
{
	memcpy(CompletionSendMessage.Buffer, inData, inSize);
	CompletionSendMessage.WSABuffer.len = inSize;

	int result = ::WSASend(NativeSocket, &CompletionSendMessage.WSABuffer, 1, &CompletionSendMessage.WSABuffer.len, 0, &CompletionSendMessage, NULL);

	SendFlag = false;
}



void MIOCPSocket::SendBufferData()
{
	const MSIZE size = SendBuffer->GetReadableSize();
	Send(SendBuffer->GetReadPointer(), size);
	SendBuffer->Pop(size);
}



void MIOCPSocket::PushSendPacket(MUINT32 inPacketID, class MPacket& inPacket, MMemory* inTempMemory)
{
	MIOModelUtil::PushPacket(*SendPushBuffer, inPacketID, inPacket, *inTempMemory);
}


void MIOCPSocket::PushRecvPacket(void* inData, MSIZE inSize)
{
	RecvPushBuffer->Push(inData, inSize);
	RecvFlag = true;
}


void MIOCPSocket::Update()
{
	// ������ �������� üũ�ϰ� �����ϴٸ� �۾� ������� ���� ��û
	const MSIZE sendSize = SendPushBuffer->GetReadableSize();
	if (0 < sendSize && false == SendFlag)
	{
		// ���� ����
		std::swap(SendPushBuffer, SendBuffer);

		// �÷��� ����
		SendFlag = true;

		// �۾� ������� �̺�Ʈ ����
		::PostQueuedCompletionStatus(IOCP, 0, 0, &RequestSendMessage);
	}

	// ���� ������ �ִٸ� ������ process���ۿ� push�ϰ� �÷��׸� false�� ����
	// �ٽ� recvó���� �ǵ��� ó��
	if (true == RecvFlag)
	{
		const MSIZE size = RecvPushBuffer->GetReadableSize();
		RecvBuffer->Push(RecvPushBuffer->GetReadPointer(), size);
		RecvPushBuffer->Pop(size);

		RecvFlag = false;

		// ���Ͽ� recv �۾� ����
		Recv();
	}
}

#endif
