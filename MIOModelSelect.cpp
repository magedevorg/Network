#include "MIOModelSelect.h"
#include "MBuffer.h"
#include "MPacket.h"


EMError MIOModelSelect::Start(MIOModelEventListener* inListener)
{
    EMError error = MIOModel::Start(inListener);
    if( EMError::None != error) {
        return error;
    }

    // 소켓 매니저 초기화
    SocketManager.InitSocketManager(Config.NumberOfSocket);
    
    for(MSelectSocket* socket : SocketManager.SocketList) 
    {
        socket->SendBuffer.Alloc(Config.SendBufferSize);
        socket->RecvBuffer.Alloc(Config.RecvBufferSize);
    }

    // 처리에 사용되는 임시 메모리
	TempMemoryPool.InitPool(MFALSE, 1, MTRUE,
		[]()->MMemory*
		{
            return new MMemory(256);
		});

    // 처리에 사용되는 임시 버퍼
	TempBufferPool.InitPool(MFALSE, 1, MTRUE,
		[]()->MBuffer*
		{
            return new MBuffer(256);
		});

    return EMError::None;
}


void MIOModelSelect::Update()
{
    MPoolObject<MMemory> tempMemory(TempMemoryPool);

    // 타임아웃 체크
    CheckTimeout();
   
    timeval timeOut = { 0,0 };

    // 값 초기화
    fd_set checkReadSet = ReadSet;
    fd_set checkWriteSet = WriteSet;

    // select 처리
    const int result = ::select(CheckFDCount, &checkReadSet, &checkWriteSet, nullptr, &timeOut);
    if (0 < result)
    {
        // 읽기 이벤트 처리
        ReadEvent(checkReadSet);

        // 쓰기 이벤트 처리
        WriteEvent(checkWriteSet);
    }

    // 활성화 세션을 루프돌면서 패킷 처리
    {
        MPacketHeader header;

        for(auto& pair : SocketManager.ActiveSocketMap)
        {
            MSelectSocket* socket = pair.second;

			if (EMSocketState::Run == socket->GetState())
			{
				// 대상 버퍼
				MBuffer* buffer = &socket->RecvBuffer;
				while (MTRUE)
				{
					if (MFALSE == PopPacket(header, *tempMemory.Get(), *buffer)) {
						break;
					}

					OnPacket(socket->GetUniqueID(), header.PacketID, tempMemory.Get()->GetPointer(), header.PacketSize);
				}
			}
        }
    }
}


void MIOModelSelect::Release()
{   
    const MSIZE activeCount = SocketManager.ActiveUniqueIDMap.size();
    if( 0 < activeCount)
    {
        std::vector<MUINT64> activeList;
        activeList.reserve(activeCount);

        for (auto& pair : SocketManager.ActiveUniqueIDMap) {
            activeList.push_back(pair.first);
        }

        for(MINDEX i=0;i<activeCount;++i) {
            Disconnect(activeList[i]);
        }
    }
    
    MIOModel::Release();
}


void MIOModelSelect::Connect(const MIOModelEventConnect& inParam)
{
    // 사용할수 있는 소켓이 있는지 체크
    if (0 == SocketManager.GetAvailableSocketCount()) 
    {  
        // 소켓이 부족함
        MIOModelEventOnConnect param(EMError::LackOfSocket, MINDEX_NONE);
        EventListener->OnConnect(param);
        return;
    }

    // 접속 소켓을 생성
    MNATIVE_SOCKET nativeSocket = CreateNativeSocket();
    if (MSOCKET_INVALID == nativeSocket)
    {
        // 소켓 생성 실패
        MIOModelEventOnConnect param(EMError::CreateSocketFailed, MINDEX_NONE);
        EventListener->OnConnect(param);
        return;
    }

    // 논블러킹 처리
    SetSocketBlockingMode(nativeSocket, MFALSE);

    // 주소 설정
    sockaddr_in addr = {0,};
    SetAddress(addr, AF_INET, inParam.IP, inParam.Port);

    // 접속
    int flag = ::connect(nativeSocket, (sockaddr*)&addr, sizeof(addr));
    if (MSOCKET_ERROR == flag)
    {
        EMError error = GetLastError();

        switch (error)
        {
        case EMError::WouldBlock:
        {
            // 에러 아님. 진행중
        } break;
        default:
        {
            // 소켓 종료
            CloseNativeSocket(nativeSocket);

            // 접속 콜백은 메인쓰레드에서 처리
            MIOModelEventOnConnect param(error, MINDEX_NONE);
            EventListener->OnConnect(param);
            return;
        }
        }
    }

    // 소켓 객체를 얻는다
    MSelectSocket* socket = SocketManager.Add(nativeSocket);
    
    // 상태 설정
    socket->SetState(EMSocketState::Connecting);

    // 타임아웃 체크에 등록
    CheckTimeoutSet.insert(socket);
    
    // 쓰기 대상 등록
    AddSelectEvent(WriteSet, nativeSocket);
}


void MIOModelSelect::Listen(const MIOModelEventListen& inParam)
{
    // 사용할수 있는 소켓이 있는지 체크
    if (0 == SocketManager.GetAvailableSocketCount())
    {
        // 소켓이 부족함
        MIOModelEventOnListen param(EMError::LackOfSocket);
        EventListener->OnListen(param);
        return;
    }

    // 소켓 생성
    MNATIVE_SOCKET nativeSocket = CreateNativeSocket();
    if( MSOCKET_INVALID == nativeSocket)
    {
        // 소켓 생성에 실패
        MIOModelEventOnListen param(EMError::CreateSocketFailed);
        EventListener->OnListen(param);
        return;
    }

    // 블러킹 모드 설정
    SetSocketBlockingMode(nativeSocket, MFALSE);
    
    // 바인드
    sockaddr_in addr = { 0, };
    SetAddress(addr, AF_INET, inParam.IP, inParam.Port);

    if (MSOCKET_ERROR == ::bind(nativeSocket, (sockaddr*)&addr, sizeof(addr)))
    {
        // 에러 메시지
        return;
    }

    

    // 연결대기
    // 두번째 인자는 연결 요청 대기 큐의 크기
    if (MSOCKET_ERROR == ::listen(nativeSocket, 5))
    {
        // 에러 메시지
        return;
    }

    // 소켓 객체를 얻는다
    MSelectSocket* socket = SocketManager.Add(nativeSocket);
    socket->SetState(EMSocketState::Listen);

    // 리드셋 설정
    AddSelectEvent(ReadSet, nativeSocket);

    // 소켓 생성에 실패
    MIOModelEventOnListen param(EMError::None);
    EventListener->OnListen(param);
}


void MIOModelSelect::Disconnect(const MIOModelEventDisconnect& inParam)
{
    // 대상소켓을 찾는다
    MSelectSocket* socket = SocketManager.FindByUniqueID(inParam.UniqueID);
    if( nullptr == socket ) {
        return;
    }

    // 타임아웃 셋에서 제거
    CheckTimeoutSet.erase(socket);

    MNATIVE_SOCKET nativeSocket = socket->GetNativeSocket();

    // 네이티브 소켓 해제
    if (MSOCKET_INVALID != nativeSocket) 
    {
        // 셋에서 제거
        RemoveSelectEvent(WriteSet, nativeSocket);
        RemoveSelectEvent(ReadSet, nativeSocket);

        // 소켓 제거
        CloseNativeSocket(nativeSocket);
    }

    // 소켓 클리어
    socket->Clear();
    
    // 제거
    SocketManager.RemoveByNativeSocket(nativeSocket);
}



void MIOModelSelect::SendPacket(MUINT64 inUniqueID, MUINT32 inPacketID, class MPacket& inPacket)
{
    // 대상을 찾는다
    MSelectSocket* socket = SocketManager.FindByUniqueID(inUniqueID);
    if( nullptr == socket ) {
        return;
    }
    
    MPoolObject<MMemory> tempMemory(TempMemoryPool);

    // 패킷 전달
    PushPacket(socket->SendBuffer, inPacketID, inPacket, *tempMemory.Get());
}


MNATIVE_SOCKET MIOModelSelect::CreateNativeSocket()
{
    // 소켓 생성
    return ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
}

void MIOModelSelect::CloseNativeSocket(MNATIVE_SOCKET inNativeSocket)
{
    if (MSOCKET_INVALID == inNativeSocket) {
        return;
    }

#if (MPLATFORM == MPLATFORM_WINDOWS)
    {
        ::closesocket(inNativeSocket);
    }
#else
    {
        ::close(inNativeSocket);
    }
#endif
}


void MIOModelSelect::AddSelectEvent(fd_set& outSet, MNATIVE_SOCKET inSocket)
{
    const u_int id = (u_int)inSocket;
    FD_SET(id, &outSet);

#if(MPLATFORM != MPLATFORM_WINDOWS)
    {
        // 큰 값을 설정
        const u_int count = id + 1;
        CheckFDCount = MMAX_VALUE(count, CheckFDCount);
    }
#endif
}


void MIOModelSelect::RemoveSelectEvent(fd_set& outSet, MNATIVE_SOCKET inSocket)
{
    const u_int id = (u_int)inSocket;
    FD_CLR(id, &outSet);

#if(MPLATFORM != MPLATFORM_WINDOWS)
    {
        const u_int count = id + 1;
        if (count == CheckFDCount)
        {
            // 삭제되는 FD의 카운트가 체크할 최대 카운트였다면 재설정 해준다
            CheckFDCount = 0;

            for (MINDEX i = id - 1; 0 <= i; --i)
            {
                if (0 < FD_ISSET(i, &ReadSet) || 0 < FD_ISSET(i, &WriteSet))
                {
                    CheckFDCount = i + 1;
                    break;
                }
            }
        }
    }
#endif
}


void MIOModelSelect::ReadEvent(fd_set& inReadSet)
{
    // 임시 메모리
    MPoolObject<MMemory> tempMemory(TempMemoryPool);

    // 소켓 리스트를 돌면서 전송된 정보가 있는지 체크
    const MSIZE count = GetEventSocketList(TempEventSocketList, inReadSet);
    for (MINDEX i = 0; i < count; ++i)
    {
        // 소켓을 얻었다
        MNATIVE_SOCKET nativeSocket = TempEventSocketList[i];
        
        MSelectSocket* socket = SocketManager.FindByNativeSocket(nativeSocket);
        if( nullptr == socket) 
        {
            // 무효한 소켓인경우 처리
            continue;
        }
        
        if (EMSocketState::Listen == socket->GetState())
        {
            // 리슨 소켓인경우 accept처리
            sockaddr_in addr = { 0, };
            MSOCKET_LEN_TYPE size = sizeof(addr);
            MNATIVE_SOCKET nativeAcceptSocket = ::accept(nativeSocket, (sockaddr*)&addr, &size);

            EMError error = EMError::None;

            if( 0 == SocketManager.GetAvailableSocketCount() )
            {
                // 소켓을 모두 사용
                error = EMError::ServerLackOfSocket;
            }
            else
            {
                // 성공
                
                // 블러킹 모드 설정
                SetSocketBlockingMode(nativeAcceptSocket, MFALSE);

                MSelectSocket* acceptSocket = SocketManager.Add(nativeAcceptSocket);
                acceptSocket->SetState(EMSocketState::Run);

                // 이벤트 등록
                AddSelectEvent(ReadSet, nativeAcceptSocket);
                AddSelectEvent(WriteSet, nativeAcceptSocket);
            }

            // 클라이언트로 정보 전송
            {
                MPacketConnectResponse packet;
                packet.Error = error;

                // 임시 버퍼
                MPoolObject<MBuffer> tempBuffer(TempBufferPool);
                tempBuffer.Get()->Reset();

                PushPacket(*tempBuffer.Get(), (MUINT32)EMPacket::ConnectResponse, packet, *tempMemory.Get());

                // 읽기 사이즈
                const MSIZE readableSize = tempBuffer.Get()->GetReadableSize();

                // 처리
                ::send(nativeAcceptSocket, (const char*)tempBuffer.Get()->GetReadPointer(), readableSize, 0);
                tempBuffer.Get()->Pop(readableSize);

                // 에러가 발생한경우 소켓을 닫는다
                if( EMError::None != error ) {
                    CloseNativeSocket(nativeAcceptSocket);
                }
            }
          
            // accept 통지
            MIOModelEventOnAccept param(error, nativeAcceptSocket);
            EventListener->OnAccept(param);
        }
        else
        {
            // 동작 상태
            const int size = ::recv(nativeSocket, (char*)tempMemory.Get()->GetPointer(), tempMemory.Get()->GetSize(), 0);
            if (-1 == size)
            {
                EMError error = GetLastError();
                switch (error)
                {
                case EMError::WouldBlock:
                {
                    // 대기
                } break;
                default:
                {
                    // 에러 발생
                } break;
                }
            }
            else if (0 < size)
            {
                // 우선 받은 데이터를 수신 버퍼에 write
                socket->RecvBuffer.Push(tempMemory.Get()->GetPointer(), size);
            }
            else
            {
                // 클라이언트에서 접속을 끊음
                MUINT64 uniqueID = socket->GetUniqueID();

                // 접속 끊김 알림
                MIOModelEventOnDisconnected param(uniqueID);
                EventListener->OnDisconnected(param);

                // 접속 끊김 처리
                Disconnect(uniqueID);
            }
        }
    }
}

void MIOModelSelect::WriteEvent(fd_set& inWriteSet)
{
    const MSIZE count = GetEventSocketList(TempEventSocketList, inWriteSet);
    for (MINDEX i = 0; i < count; ++i)
    {
        // 이벤트가 발생한 소켓을 얻는다
        MNATIVE_SOCKET nativeSocket = TempEventSocketList[i];

        // 접속중인 세션에 대상이 있는지 체크
        MSelectSocket* socket = SocketManager.FindByNativeSocket(nativeSocket);
        if (nullptr != socket)
        {
            // 세션 상태
            const EMSocketState state = socket->GetState();
            switch(state)
            {
            case EMSocketState::Connecting:
            {
                // 접속중이였다면 동작상태로 변경
                socket->SetState(EMSocketState::Run);

                // 리드셋 등록
                AddSelectEvent(ReadSet, nativeSocket);

                // 타임아웃 체크에서 제거
                CheckTimeoutSet.erase(socket);

            } break;
            case EMSocketState::Run:
            {
                // 사용할 전송 버퍼를 얻는다
                MBuffer* sendBuffer = &socket->SendBuffer;
                
                // 전송할 데이터가 있는지 체크
                const MSIZE readSize = sendBuffer->GetReadableSize();
                if( 0 < readSize)
                {
                    // 전송 처리
                    int result = ::send(nativeSocket, (const char*)sendBuffer->GetReadPointer(), readSize, 0);
                    if (0 < result)
                    {
                        // 전송되었다면 버퍼에서 제거
                        sendBuffer->Pop(readSize);
                    }
                }
            } break;
            }
        }
    }
}



MSIZE MIOModelSelect::GetEventSocketList(std::vector<MNATIVE_SOCKET>& outList, fd_set& inSet)
{
    outList.clear();

#if(MPLATFORM == MPLATFORM_WINDOWS)
    {
        for (MSIZE i = 0; i < (MSIZE)inSet.fd_count; ++i)
        {
            // 소켓을 얻었다
            MNATIVE_SOCKET socket = inSet.fd_array[i];
            outList.push_back(socket);
        }
    }
#else
    {
        for (MSIZE i = 0; i < CheckFDCount; ++i)
        {
            if (0 < FD_ISSET(i, &inSet))
            {
                outList.push_back(i);
            }
        }
    }
#endif
    
    return outList.size();
}



void MIOModelSelect::CheckTimeout()
{
    if (true == CheckTimeoutSet.empty()) {
        return;
    }

    // 사용할 정보를 따로 리스트로 저장
    TempSocketList.clear();
    for (MSelectSocket* socket : CheckTimeoutSet) {
        TempSocketList.push_back(socket);
    }

    // 프레임 시작 시간
    MDateTime frameStartTime = MDateTime::UtcNow();

    for (MSelectSocket* socket : TempSocketList)
    {
        // 아직 타임아웃 시간이 지나지 않았다면 넘어간다
        MTimespan checkTime = frameStartTime - socket->GetStateStartTime();
        if (checkTime.GetMilliseconds() < Config.ConnectTimeoutMilliseconds) {
            continue;
        }

        // 타임아웃 처리
        MIOModelEventOnConnect param(EMError::Timeout, socket->GetUniqueID());
        EventListener->OnConnect(param);

        // 접속 끊기 처리
        Disconnect(socket->GetUniqueID());
    }
}


