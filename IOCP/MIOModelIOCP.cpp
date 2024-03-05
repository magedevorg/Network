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

	// IOCP 객체 생성
	IOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (INVALID_HANDLE_VALUE == IOCP) {
		return EMError::CreateIOCPFailed;
	}

	//-------------------------------------------------------
	// 풀
	//-------------------------------------------------------
	TempMemoryPool.InitPool(MFALSE, 2, MTRUE, 
		[]()->MMemory*
		{
			return new MMemory(64);
		});

	// 소켓 매니저 초기화
	SocketManager.InitSocketManager(Config.NumberOfSocket);
	for (MINDEX i = 0; i < Config.NumberOfSocket; ++i) {
		SocketManager.SocketList[i]->SetIOCP(IOCP);
	}
	
	// 이벤트 버퍼 생성
	EventPushBuffer = new MBuffer(Config.EventBufferSize);
	EventProcessBuffer = new MBuffer(Config.EventBufferSize);
	
	// 작업 쓰레드 생성
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
		
		// 해당 소켓 갱신
		socket->Update();
		
		// 수신 버퍼를 얻는다
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

			// 메모리 할당
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
	// 쓰레드 종료 처리
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
	
	// 열려있는 모든 소켓을 닫는다
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

	// 이벤트 버퍼를 제거
	MSAFE_DELETE(EventPushBuffer);
	MSAFE_DELETE(EventProcessBuffer);

	// IOCP 객체 해제
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
	// 대상소켓을 찾는다
	MIOCPSocket* socket = SocketManager.FindByUniqueID(inParam.UniqueID);
	if (nullptr == socket) {
		return;
	}

	// 소켓을 닫는다
	MNATIVE_SOCKET nativeSocket = socket->GetNativeSocket();

	CloseNativeSocket(nativeSocket);

	// 닫는다
	SocketManager.RemoveByNativeSocket(nativeSocket);
}


void MIOModelIOCP::Listen(const MIOModelEventListen& inParam)
{
	// 사용할수 있는 소켓이 있는지 체크
	if (0 == SocketManager.GetAvailableSocketCount()) 
	{
		MIOModelEventOnListen param(EMError::LackOfSocket);
		EventListener->OnListen(param);
		return;
	}

	// 여기서 바로 처리
	MNATIVE_SOCKET listenSocket = CreateNativeSocket();
	if (MSOCKET_INVALID == listenSocket)
	{
		MIOModelEventOnListen param(EMError::CreateSocketFailed);
		EventListener->OnListen(param);
		return;
	}

	// 주소를 만들고 바인드
	sockaddr_in addr = { 0, };
	SetAddress(addr, AF_INET, inParam.IP, inParam.Port);

	if (MSOCKET_ERROR == ::bind(listenSocket, (sockaddr*)&addr, sizeof(addr)))
	{
		// 소켓을 닫는다
		CloseNativeSocket(listenSocket);

		// 이벤트 전송
		MIOModelEventOnListen param(EMError::BindFailed);
		EventListener->OnListen(param);
		return;
	}

	// 연결대기
	// 두번째 인자는 연결 요청 대기 큐의 크기
	if (MSOCKET_ERROR == ::listen(listenSocket, 5))
	{
		CloseNativeSocket(listenSocket);

		MIOModelEventOnListen param(EMError::ListenFailed);
		EventListener->OnListen(param);
		return;
	}

	// 소켓 등록
	MIOCPSocket* socket = AddSocket(listenSocket);

	// 리슨 성공
	MIOModelEventOnListen param(EMError::None);
	PushEvent(&param, sizeof(param));

	// accept 처리
	socket->Accept();
}


void MIOModelIOCP::SendPacket(MUINT64 inUniqueID, MUINT32 inPacketID, class MPacket& inPacket)
{
	// 대상 소켓을 얻는다
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
	// 소켓 정보 등록
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
	// 버퍼를 스왑
	{
		std::lock_guard<std::mutex> guard(Mutex_EventBuffer);
		std::swap(EventPushBuffer, EventProcessBuffer);
	}

	MPoolObject<MMemory> tempMemory(TempMemoryPool);

	while (false == EventProcessBuffer->IsEmpty())
	{
		// 데이터가 있는경우 처리
		EMIOModelEventType eventType = EMIOModelEventType::None;
		EventProcessBuffer->Read(&eventType, sizeof(eventType));


		// 이벤트 타입에 따라 캐스팅해서 처리
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

			// 대상을 찾는다
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
				// 사용할 소킷이 남아있는지 체크
				if (0 == SocketManager.GetAvailableSocketCount())
				{
					CloseNativeSocket(param.Socket);

					param.Error = EMError::LackOfSocket;
					param.Socket = INVALID_SOCKET;
				}	
				else
				{
					// 정상적으로 접속 된경우

					MIOCPSocket* socket = AddSocket(param.Socket);
					socket->Recv();

					// 클라이언트로 onConnect 패킷 전송
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
