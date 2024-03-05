#pragma once


#include "MNetwork.h"
#include "MIOModelEvent.h"

#if (MPLATFORM == MPLATFORM_WINDOWS)

//-----------------------------------------------------------
// IOCP���� ó���ϴ� �޽��� Ÿ��
//-----------------------------------------------------------
enum class EMIOCPMessageType
{
	None		= 0,

	RequestFinish,			// ����
	RequestSend,			// ����

	CompletionAccept,		// Accept �Ϸ�
	CompletionRecv,			// Recv �Ϸ�
	CompletionSend,			// Send �Ϸ�
};






//-----------------------------------------------------------
// IOCP���� ó���ϴ� �޽���
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
	// �޽��� Ÿ��
	EMIOCPMessageType MessageType = EMIOCPMessageType::None;
};



//-----------------------------------------------------------
// ���� �޽���
//-----------------------------------------------------------
struct MIOCPRequestFinishMessage : public MIOCPMessage
{
public:
	MIOCPRequestFinishMessage()
		: MIOCPMessage(EMIOCPMessageType::RequestFinish)
	{}
};


//-----------------------------------------------------------
// ���� �޽���
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
// accept �Ϸ� �޽���
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
	// ���� ����
	class MIOCPSocket* ListenSocket = nullptr;

	// ���� ����
	MNATIVE_SOCKET AcceptSocket = MSOCKET_INVALID;

	// ���� ����
	WSABUF WSABuffer;
	MBYTE Buffer[1024] = { 0, };
};


//-----------------------------------------------------------
// recv �Ϸ� �޽���
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
	// ���� ����
	class MIOCPSocket* RecvSocket = nullptr;

	// ���� ����
	WSABUF WSABuffer;
	MBYTE Buffer[1024] = { 0, };
};


//-----------------------------------------------------------
// send �Ϸ� �޽���
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
	// ���� ����
	class MIOCPSocket* SendSocket = nullptr;

	// ���� ����
	WSABUF WSABuffer;
	MBYTE Buffer[1024] = { 0, };
};


#endif
