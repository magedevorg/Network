
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
		// 종료 대기
		WorkerThread.join();
	}
}


void MIOModelIOCPWorkerThread::Run()
{
	// 쓰레드 동작
	WorkerThread = std::thread(&MIOModelIOCPWorkerThread::Update, this);
}


void MIOModelIOCPWorkerThread::Update()
{
	bool isRun = true;
	while (true == isRun)
	{
		DWORD ioSize = 0;
		ULONG_PTR key;

		// 이벤트 정보
		MIOCPMessage* message = nullptr;
		BOOL result = ::GetQueuedCompletionStatus(IOCP, &ioSize, &key, (WSAOVERLAPPED**)&message, INFINITE);
		if (FALSE == result) {
			continue;
		}

		// 메시지 타입에 따른 처리
		switch (message->MessageType)
		{
		case EMIOCPMessageType::RequestFinish:
		{	
			// 종료 처리
			MIOCPRequestFinishMessage* finishMessage = static_cast<MIOCPRequestFinishMessage*>(message);
			isRun = false;
			delete finishMessage;
		} break;
		case EMIOCPMessageType::RequestSend:
		{
			// 종료 처리
			MIOCPRequestSendMessage* sendMessage = static_cast<MIOCPRequestSendMessage*>(message);
			
			// 프로세스 버퍼정보를 전송
			sendMessage->SendSocket->SendBufferData();

		} break;

		case EMIOCPMessageType::CompletionAccept:
		{
			// accpet 처리
			MIOCPCompletionAcceptMessage* completionAcceptMessage = static_cast<MIOCPCompletionAcceptMessage*>(message);
			ProcessCompletionAcceptMessage(completionAcceptMessage);
		} break;

		case EMIOCPMessageType::CompletionRecv:
		{
			// 리시브 처리
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

	// 이벤트의 소켓 정보는 초기화
	inMessage->AcceptSocket = MSOCKET_INVALID;

	// 리슨소켓에 다시 accept 설정
	inMessage->ListenSocket->Accept();


	// 메인쓰레드로 전달
	MIOModelEventOnAccept acceptEvent(EMError::None, acceptSocket);
	Owner->PushEvent(&acceptEvent, sizeof(acceptEvent));
}



void MIOModelIOCPWorkerThread::ProcessCompletionRecvMessage(struct MIOCPCompletionRecvMessage* inMessage, DWORD inSize)
{
	MIOCPSocket* socket = inMessage->RecvSocket;
	if (0 == inSize)
	{
		// 디스커넥트 처리
		MIOModelEventOnDisconnected onDisconnectedEvent(socket->GetUniqueID());
		Owner->PushEvent(&onDisconnectedEvent, sizeof(onDisconnectedEvent));
	}
	else
	{
		socket->PushRecvPacket(inMessage->Buffer, inSize);
	}
}


#endif
