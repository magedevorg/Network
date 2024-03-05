#include "MIOModelIOCP.h"
#include "MPacket.h"
#include "MIOModelIOCPMessage.h"

#include "MIOModelIOCPWorkerThread.h"
#include "MIOModelUtil.h"


/*
#if !defined(DEBUG_NEW)
#define DEBUG_NEW new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

*/

#if (MPLATFORM == MPLATFORM_WINDOWS)

EMError MIOModelIOCP::Start(MIOModelEventListener* inListener)
{
	EMError error = MIOModel::Start(inListener);
	if (EMError::None != error) {
		return error;
	}

	// IOCP ��ü ����
	IOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (INVALID_HANDLE_VALUE == IOCP) {
		return EMError::CreateIOCPFailed;
	}

	//-------------------------------------------------------
	// Ǯ
	//-------------------------------------------------------
	TempMemoryPool.InitPool(MFALSE, 2, MTRUE, 
		[]()->MMemory*
		{
			return new MMemory(64);
		});

	// ���� �Ŵ��� �ʱ�ȭ
	SocketManager.InitSocketManager(Config.NumberOfSocket);
	for (MINDEX i = 0; i < Config.NumberOfSocket; ++i) {
		SocketManager.SocketList[i]->SetIOCP(IOCP);
	}
	
	// �̺�Ʈ ���� ����
	EventPushBuffer = new MBuffer(Config.EventBufferSize);
	EventProcessBuffer = new MBuffer(Config.EventBufferSize);
	
	// �۾� ������ ����
	{
		MSIZE workThreadCount = 1;
		{
			SYSTEM_INFO sysInfo;
			::GetSystemInfo(&sysInfo);

			workThreadCount = sysInfo.dwNumberOfProcessors;
		}

		WorkThreadList.resize(workThreadCount);
		for (MINDEX i = 0; i < workThreadCount; ++i) 
		{
			WorkThreadList[i] = new MIOModelIOCPWorkerThread(this);
			WorkThreadList[i]->Run();
		}
	}
	
	return EMError::None;
}


void MIOModelIOCP::Update()
{	
	MPoolObject<MMemory> tempMemory(TempMemoryPool);
	
	for (auto& pair : SocketManager.ActiveSocketMap)
	{
		MIOCPSocket* socket = pair.second;
		
		// �ش� ���� ����
		socket->Update();
		
		// ���� ���۸� ��´�
		MBuffer* readBuffer = socket->GetRecvBuffer();
		while (true)
		{
			const MSIZE readableSize = readBuffer->GetReadableSize();
			if (readableSize < sizeof(MPacketHeader)) {
				break;
			}

			MPacketHeader header;
			readBuffer->Read(&header, sizeof(header));

			const MSIZE totalSize = sizeof(header) + header.PacketSize;
			if (readableSize < totalSize ) {
				break;
			}

			readBuffer->Read(&header, sizeof(header));
			readBuffer->Pop(sizeof(header));

			// �޸� �Ҵ�
			tempMemory->Alloc(header.PacketSize, MTRUE);

			readBuffer->Read(tempMemory->GetPointer(), header.PacketSize);
			readBuffer->Pop(header.PacketSize);
			
			EventListener->OnPacket(socket->GetUniqueID(), header.PacketID, tempMemory->GetPointer(), header.PacketSize);
		}
	}
	
	ProcessEvent();
}

void MIOModelIOCP::Release()
{
	// ������ ���� ó��
	{
		const MSIZE count = WorkThreadList.size();
		for(MINDEX i=0;i<count;++i)  {
			WorkThreadList[i]->Stop();
		}

		for (MINDEX i = 0; i < count; ++i) {
			delete WorkThreadList[i];
		}

		WorkThreadList.clear();
	}
	
	// �����ִ� ��� ������ �ݴ´�
	const MSIZE activeCount = SocketManager.ActiveUniqueIDMap.size();
	if (0 < activeCount)
	{
		std::vector<MUINT64> activeList;
		activeList.reserve(activeCount);

		for (auto& pair : SocketManager.ActiveUniqueIDMap) {
			activeList.push_back(pair.first);
		}

		for (MINDEX i = 0; i < activeCount; ++i) {
			Disconnect(activeList[i]);
		}
	}

	// �̺�Ʈ ���۸� ����
	MSAFE_DELETE(EventPushBuffer);
	MSAFE_DELETE(EventProcessBuffer);

	// IOCP ��ü ����
	if (INVALID_HANDLE_VALUE != IOCP)
	{
		::CloseHandle(IOCP);
		IOCP = INVALID_HANDLE_VALUE;
	}

	MIOModel::Release();
}

void MIOModelIOCP::Connect(const MIOModelEventConnect& inParam)
{

}


void MIOModelIOCP::Disconnect(const MIOModelEventDisconnect& inParam)
{
	// �������� ã�´�
	MIOCPSocket* socket = SocketManager.FindByUniqueID(inParam.UniqueID);
	if (nullptr == socket) {
		return;
	}

	// ������ �ݴ´�
	MNATIVE_SOCKET nativeSocket = socket->GetNativeSocket();

	CloseNativeSocket(nativeSocket);

	// �ݴ´�
	SocketManager.RemoveByNativeSocket(nativeSocket);
}


void MIOModelIOCP::Listen(const MIOModelEventListen& inParam)
{
	// ����Ҽ� �ִ� ������ �ִ��� üũ
	if (0 == SocketManager.GetAvailableSocketCount()) 
	{
		MIOModelEventOnListen param(EMError::LackOfSocket);
		EventListener->OnListen(param);
		return;
	}

	// ���⼭ �ٷ� ó��
	MNATIVE_SOCKET listenSocket = CreateNativeSocket();
	if (MSOCKET_INVALID == listenSocket)
	{
		MIOModelEventOnListen param(EMError::CreateSocketFailed);
		EventListener->OnListen(param);
		return;
	}

	// �ּҸ� ����� ���ε�
	sockaddr_in addr = { 0, };
	SetAddress(addr, AF_INET, inParam.IP, inParam.Port);

	if (MSOCKET_ERROR == ::bind(listenSocket, (sockaddr*)&addr, sizeof(addr)))
	{
		// ������ �ݴ´�
		CloseNativeSocket(listenSocket);

		// �̺�Ʈ ����
		MIOModelEventOnListen param(EMError::BindFailed);
		EventListener->OnListen(param);
		return;
	}

	// ������
	// �ι�° ���ڴ� ���� ��û ��� ť�� ũ��
	if (MSOCKET_ERROR == ::listen(listenSocket, 5))
	{
		CloseNativeSocket(listenSocket);

		MIOModelEventOnListen param(EMError::ListenFailed);
		EventListener->OnListen(param);
		return;
	}

	// ���� ���
	MIOCPSocket* socket = AddSocket(listenSocket);

	// ���� ����
	MIOModelEventOnListen param(EMError::None);
	PushEvent(&param, sizeof(param));

	// accept ó��
	socket->Accept();
}


void MIOModelIOCP::SendPacket(MUINT64 inUniqueID, MUINT32 inPacketID, class MPacket& inPacket)
{
	// ��� ������ ��´�
	MIOCPSocket* socket = SocketManager.FindByUniqueID(inUniqueID);
	if (nullptr == socket) {
		return;
	}

	MPoolObject<MMemory> tempMemory(TempMemoryPool);
	socket->PushSendPacket(inPacketID, inPacket, tempMemory.Get());
}

MNATIVE_SOCKET MIOModelIOCP::CreateNativeSocket()
{
	return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

void MIOModelIOCP::CloseNativeSocket(MNATIVE_SOCKET inNativeSocket)
{
	::closesocket(inNativeSocket);
}






MIOCPSocket* MIOModelIOCP::AddSocket(MNATIVE_SOCKET inSocket)
{
	// ���� ���� ���
	MIOCPSocket* temp = SocketManager.Add(inSocket);
	if (nullptr != temp)
	{
		HANDLE ret = ::CreateIoCompletionPort((HANDLE)inSocket, IOCP, (DWORD)0, 0);
	}

	return temp;
}



void MIOModelIOCP::PushEvent(void* inEvent, MSIZE inSize)
{
	std::lock_guard<std::mutex> guard(Mutex_EventBuffer);
	EventPushBuffer->Push(inEvent, inSize, MTRUE);
}


void MIOModelIOCP::ProcessEvent()
{
	// ���۸� ����
	{
		std::lock_guard<std::mutex> guard(Mutex_EventBuffer);
		std::swap(EventPushBuffer, EventProcessBuffer);
	}

	MPoolObject<MMemory> tempMemory(TempMemoryPool);

	while (false == EventProcessBuffer->IsEmpty())
	{
		// �����Ͱ� �ִ°�� ó��
		EMIOModelEventType eventType = EMIOModelEventType::None;
		EventProcessBuffer->Read(&eventType, sizeof(eventType));


		// �̺�Ʈ Ÿ�Կ� ���� ĳ�����ؼ� ó��
		switch (eventType)
		{
		case EMIOModelEventType::OnConnect:
		{
			MIOModelEventOnConnect param;
			EventProcessBuffer->Read(&param, sizeof(param));
			EventProcessBuffer->Pop(sizeof(param));

			EventListener->OnConnect(param);
		} break;

		case EMIOModelEventType::OnListen:
		{
			MIOModelEventOnListen param;
			EventProcessBuffer->Read(&param, sizeof(param));
			EventProcessBuffer->Pop(sizeof(param));

			EventListener->OnListen(param);
		} break;

		case EMIOModelEventType::OnDisconnected:
		{
			MIOModelEventOnDisconnected param;
			EventProcessBuffer->Read(&param, sizeof(param));
			EventProcessBuffer->Pop(sizeof(param));

			// ����� ã�´�
			if (MIOCPSocket* socket = SocketManager.FindByUniqueID(param.UniqueID))
			{
				MNATIVE_SOCKET nativeSocket = socket->GetNativeSocket();
				CloseNativeSocket(nativeSocket);
				SocketManager.RemoveByNativeSocket(nativeSocket);

				EventListener->OnDisconnected(param);
			}

		} break;

		case EMIOModelEventType::OnAccept:
		{
			MIOModelEventOnAccept param;
			EventProcessBuffer->Read(&param, sizeof(param));
			EventProcessBuffer->Pop(sizeof(param));

			if (EMError::None == param.Error)
			{
				// ����� ��Ŷ�� �����ִ��� üũ
				if (0 == SocketManager.GetAvailableSocketCount())
				{
					CloseNativeSocket(param.Socket);

					param.Error = EMError::LackOfSocket;
					param.Socket = INVALID_SOCKET;
				}	
				else
				{
					// ���������� ���� �Ȱ��

					MIOCPSocket* socket = AddSocket(param.Socket);
					socket->Recv();

					// Ŭ���̾�Ʈ�� onConnect ��Ŷ ����
					MPacketConnectResponse packet;
					packet.Error = EMError::None;

					socket->PushSendPacket((MUINT32)EMPacket::ConnectResponse, packet, tempMemory.Get());
				}
			}
			
			EventListener->OnAccept(param);
		} break;
		}
	}
}


#endif
