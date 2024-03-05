#pragma once


#include "MNetwork.h"
#include "MIOModelEvent.h"

#if (MPLATFORM == MPLATFORM_WINDOWS)

//-----------------------------------------------------------
// IOCP에서 처리하는 메시지 타입
//-----------------------------------------------------------
enum class EMIOCPMessageType
{
	None		= 0,

	RequestFinish,			// 종료
	RequestSend,			// 전송

	CompletionAccept,		// Accept 완료
	CompletionRecv,			// Recv 완료
	CompletionSend,			// Send 완료
};






//-----------------------------------------------------------
// IOCP에서 처리하는 메시지
//-----------------------------------------------------------
struct MIOCPMessage : public WSAOVERLAPPED
{
public:
	MIOCPMessage(EMIOCPMessageType inMessageType)
		: MessageType(inMessageType)
	{}

public:
	void Clear()
	{
		hEvent = 0;
	}

public:
	// 메시지 타입
	EMIOCPMessageType MessageType = EMIOCPMessageType::None;
};



//-----------------------------------------------------------
// 종료 메시지
//-----------------------------------------------------------
struct MIOCPRequestFinishMessage : public MIOCPMessage
{
public:
	MIOCPRequestFinishMessage()
		: MIOCPMessage(EMIOCPMessageType::RequestFinish)
	{}
};


//-----------------------------------------------------------
// 종료 메시지
//-----------------------------------------------------------
struct MIOCPRequestSendMessage : public MIOCPMessage
{
public:
	MIOCPRequestSendMessage(class MIOCPSocket* inSendSocket)
		: MIOCPMessage(EMIOCPMessageType::RequestSend)
		, SendSocket(inSendSocket)
	{}

public:
	class MIOCPSocket* SendSocket = nullptr;
};


//-----------------------------------------------------------
// accept 완료 메시지
//-----------------------------------------------------------
struct MIOCPCompletionAcceptMessage : public MIOCPMessage
{
public:
	MIOCPCompletionAcceptMessage(class MIOCPSocket* inListenSocket)
		: MIOCPMessage(EMIOCPMessageType::CompletionAccept)
		, ListenSocket(inListenSocket)
	{
		WSABuffer.len = sizeof(Buffer);
		WSABuffer.buf = (char*)Buffer;
	}

public:
	// 리슨 소켓
	class MIOCPSocket* ListenSocket = nullptr;

	// 연결 소켓
	MNATIVE_SOCKET AcceptSocket = MSOCKET_INVALID;

	// 버퍼 정보
	WSABUF WSABuffer;
	MBYTE Buffer[1024] = { 0, };
};


//-----------------------------------------------------------
// recv 완료 메시지
//-----------------------------------------------------------
struct MIOCPCompletionRecvMessage : public MIOCPMessage
{
public:
	MIOCPCompletionRecvMessage(class MIOCPSocket* inRecvSocket)
		: MIOCPMessage(EMIOCPMessageType::CompletionRecv)
		, RecvSocket(inRecvSocket)
	{
		WSABuffer.len = sizeof(Buffer);
		WSABuffer.buf = (char*)Buffer;
	}

public:
	// 리슨 소켓
	class MIOCPSocket* RecvSocket = nullptr;

	// 버퍼 정보
	WSABUF WSABuffer;
	MBYTE Buffer[1024] = { 0, };
};


//-----------------------------------------------------------
// send 완료 메시지
//-----------------------------------------------------------
struct MIOCPCompletionSendMessage : public MIOCPMessage
{
public:
	MIOCPCompletionSendMessage(class MIOCPSocket* inSendSocket)
		: MIOCPMessage(EMIOCPMessageType::CompletionSend)
		, SendSocket(inSendSocket)
	{
		WSABuffer.len = sizeof(Buffer);
		WSABuffer.buf = (char*)Buffer;
	}

public:
	// 리슨 소켓
	class MIOCPSocket* SendSocket = nullptr;

	// 버퍼 정보
	WSABUF WSABuffer;
	MBYTE Buffer[1024] = { 0, };
};


#endif
