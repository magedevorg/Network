
#include "MIOModelIOCPWorkerThread.h"
#include "MIOModelIOCPMessage.h"
#include "MIOModelIOCP.h"

#if (MPLATFORM == MPLATFORM_WINDOWS)

MIOModelIOCPWorkerThread::MIOModelIOCPWorkerThread(class MIOModelIOCP* inOwner)
: Owner(inOwner)
, IOCP(inOwner->GetIOCP())
{
	TempBuffer.Alloc(32);
	TempMemory.Alloc(32);
}


MIOModelIOCPWorkerThread::~MIOModelIOCPWorkerThread()
{
	if (true == WorkerThread.joinable())
	{
		// ���� ���
		WorkerThread.join();
	}
}


void MIOModelIOCPWorkerThread::Run()
{
	// ������ ����
	WorkerThread = std::thread(&MIOModelIOCPWorkerThread::Update, this);
}


void MIOModelIOCPWorkerThread::Update()
{
	bool isRun = true;
	while (true == isRun)
	{
		DWORD ioSize = 0;
		ULONG_PTR key;

		// �̺�Ʈ ����
		MIOCPMessage* message = nullptr;
		BOOL result = ::GetQueuedCompletionStatus(IOCP, &ioSize, &key, (WSAOVERLAPPED**)&message, INFINITE);
		if (FALSE == result) {
			continue;
		}

		// �޽��� Ÿ�Կ� ���� ó��
		switch (message->MessageType)
		{
		case EMIOCPMessageType::RequestFinish:
		{	
			// ���� ó��
			MIOCPRequestFinishMessage* finishMessage = static_cast<MIOCPRequestFinishMessage*>(message);
			isRun = false;
			delete finishMessage;
		} break;
		case EMIOCPMessageType::RequestSend:
		{
			// ���� ó��
			MIOCPRequestSendMessage* sendMessage = static_cast<MIOCPRequestSendMessage*>(message);
			
			// ���μ��� ���������� ����
			sendMessage->SendSocket->SendBufferData();

		} break;

		case EMIOCPMessageType::CompletionAccept:
		{
			// accpet ó��
			MIOCPCompletionAcceptMessage* completionAcceptMessage = static_cast<MIOCPCompletionAcceptMessage*>(message);
			ProcessCompletionAcceptMessage(completionAcceptMessage);
		} break;

		case EMIOCPMessageType::CompletionRecv:
		{
			// ���ú� ó��
			MIOCPCompletionRecvMessage* completionRecvMessage = static_cast<MIOCPCompletionRecvMessage*>(message);
			ProcessCompletionRecvMessage(completionRecvMessage, ioSize);
		} break;
		}
	}
}


void MIOModelIOCPWorkerThread::Stop()
{
	MIOCPRequestFinishMessage* message = new MIOCPRequestFinishMessage();
	::PostQueuedCompletionStatus(IOCP, 0, 0, message);
}





void MIOModelIOCPWorkerThread::ProcessCompletionAcceptMessage(MIOCPCompletionAcceptMessage* inMessage)
{
	MNATIVE_SOCKET acceptSocket = inMessage->AcceptSocket;

	// �̺�Ʈ�� ���� ������ �ʱ�ȭ
	inMessage->AcceptSocket = MSOCKET_INVALID;

	// �������Ͽ� �ٽ� accept ����
	inMessage->ListenSocket->Accept();


	// ���ξ������ ����
	MIOModelEventOnAccept acceptEvent(EMError::None, acceptSocket);
	Owner->PushEvent(&acceptEvent, sizeof(acceptEvent));
}



void MIOModelIOCPWorkerThread::ProcessCompletionRecvMessage(struct MIOCPCompletionRecvMessage* inMessage, DWORD inSize)
{
	MIOCPSocket* socket = inMessage->RecvSocket;
	if (0 == inSize)
	{
		// ��Ŀ��Ʈ ó��
		MIOModelEventOnDisconnected onDisconnectedEvent(socket->GetUniqueID());
		Owner->PushEvent(&onDisconnectedEvent, sizeof(onDisconnectedEvent));
	}
	else
	{
		socket->PushRecvPacket(inMessage->Buffer, inSize);
	}
}


#endif
