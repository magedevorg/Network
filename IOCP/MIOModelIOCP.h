#pragma once


#include "MIOModel.h"
#include "MIOModelIOCPSocket.h"
#include "MPool.h"

#if (MPLATFORM == MPLATFORM_WINDOWS)

struct MIOModelIOCPConfig : public MIOModelConfig
{
public:
	MSIZE EventBufferSize = 128;
};


//---------------------------------------------------------------------
// IOCP 처리
//---------------------------------------------------------------------
class MIOModelIOCP : public MIOModel
{
public:
	// 초기화
	virtual EMError Start(MIOModelEventListener* inListener) override;

	// 갱신 처리
	virtual void Update() override;

	// 해제
	virtual void Release() override;


	//-----------------------------------------------------
	// 인터페이스
	//-----------------------------------------------------
	// 접속
	virtual void Connect(const MIOModelEventConnect& inParam) override;

	// 접속 끊기
	virtual void Disconnect(const MIOModelEventDisconnect& inParam) override;

	// 리슨
	virtual void Listen(const MIOModelEventListen& inParam) override;

	// 패킷 전송
	virtual void SendPacket(MUINT64 inUniqueID, MUINT32 inPacketID, class MPacket& inPacket) override;
	
public:
	// 소켓 생성/제거
	MNATIVE_SOCKET CreateNativeSocket();
	void CloseNativeSocket(MNATIVE_SOCKET inSocketID);

	//-----------------------------------------------------
	// 설정 정보
	//-----------------------------------------------------
	virtual const MIOModelConfig* GetConfig() override {
		return &Config;
	}

	//-----------------------------------------------------
	// 소켓 추가
	//-----------------------------------------------------
	MIOCPSocket* AddSocket(MNATIVE_SOCKET inSocket);
	
	//-----------------------------------------------------
	// 이벤트 버퍼 처리
	//-----------------------------------------------------
	// work thread
	void PushEvent(void* inEvent, MSIZE inSize);

	// 이벤트 처리
	void ProcessEvent();

	//-----------------------------------------------------
	// IOCP 핸들
	//-----------------------------------------------------
	HANDLE GetIOCP() const {
		return IOCP;
	}

protected:
	// IOCP 핸들
	HANDLE IOCP = INVALID_HANDLE_VALUE;

	//-------------------------------------------------------
	// 쓰레드 처리
	//-------------------------------------------------------
	std::vector<class MIOModelIOCPWorkerThread*> WorkThreadList;
	 
	//-------------------------------------------------------
	// 소켓 정보 매니저
	//-------------------------------------------------------
	MSocketManager<MIOCPSocket> SocketManager;
	
	//-------------------------------------------------------
	// work thread -> main thread로 전달되는 이벤트 정보
	//-------------------------------------------------------
	MBuffer* EventPushBuffer = nullptr;
	MBuffer* EventProcessBuffer = nullptr;

	std::mutex Mutex_EventBuffer;


	//-------------------------------------------------------
	// 풀
	//-------------------------------------------------------
	// 메모리 풀
	MPool<MMemory> TempMemoryPool;

public:
	// 세팅 정보
	MIOModelIOCPConfig Config;
};


#endif
