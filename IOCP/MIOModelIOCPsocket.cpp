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


// Accept 설정
void MIOCPSocket::Accept()
{
	// 메시지 정보 설정
	CompletionAcceptMessage.Clear();
	CompletionAcceptMessage.AcceptSocket = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	DWORD length = 0;

	// accpet 처리
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
			// 성공
		} break;

		default:
		{
			// 에러
		}
		}
	}
}

// recv설정
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
	// 전송이 가능한지 체크하고 가능하다면 작업 쓰레드로 전송 요청
	const MSIZE sendSize = SendPushBuffer->GetReadableSize();
	if (0 < sendSize && false == SendFlag)
	{
		// 버퍼 스왑
		std::swap(SendPushBuffer, SendBuffer);

		// 플래그 설정
		SendFlag = true;

		// 작업 쓰레드로 이벤트 전달
		::PostQueuedCompletionStatus(IOCP, 0, 0, &RequestSendMessage);
	}

	// 받을 정보가 있다면 정보를 process버퍼에 push하고 플래그를 false로 설정
	// 다시 recv처리가 되도록 처리
	if (true == RecvFlag)
	{
		const MSIZE size = RecvPushBuffer->GetReadableSize();
		RecvBuffer->Push(RecvPushBuffer->GetReadPointer(), size);
		RecvPushBuffer->Pop(size);

		RecvFlag = false;

		// 소켓에 recv 작업 설정
		Recv();
	}
}

#endif
