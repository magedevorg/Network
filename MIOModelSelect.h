#pragma once

#include "MIOModel.h"
#include "MMemory.h"
#include "MBuffer.h"



//-------------------------------------------------------------
// 세팅 정보
//-------------------------------------------------------------
struct MIOModelSelectConfig : public MIOModelConfig
{
    // 패킷 처리시 사용되는 임시 메모리 사이즈
    MSIZE TempMemorySize = 1024;
    MSIZE TempMemoryCount = 2;
};



//-------------------------------------------------------------
// 소켓 정보
//-------------------------------------------------------------
class MSelectSocket : public MSocket
{
public:
    MSelectSocket(MINDEX inIndex)
        : MSocket(inIndex)
        {}
    
public:
    // 버퍼
    MBuffer SendBuffer;
    MBuffer RecvBuffer;
};




//-------------------------------------------------------------
// Select 모델
//-------------------------------------------------------------
class MIOModelSelect : public MIOModel
{
public:
    virtual EMError Start(MIOModelEventListener* inListener) override;

    // 갱신 처리 로직
    virtual void Update() override;

    // 해제
    virtual void Release() override;


    //-----------------------------------------------------
    // 인터페이스
    //-----------------------------------------------------
    // 접속
    virtual void Connect(const MIOModelEventConnect& inParam) override;

    // 리슨
    virtual void Listen(const MIOModelEventListen& inParam) override;

    // 접속 끊기
    virtual void Disconnect(const MIOModelEventDisconnect& inParam) override;

    // 패킷 전송
    virtual void SendPacket(MUINT64 inUniqueID, MUINT32 inPacketID, class MPacket& inPacket) override;

protected:
    //-----------------------------------------------------------
    // 네이티브 소켓 생성/제거
    //-----------------------------------------------------------
    MNATIVE_SOCKET CreateNativeSocket();
    void CloseNativeSocket(MNATIVE_SOCKET inNativeSocket);


    // 특정 셋에 등록/해제
    void AddSelectEvent(fd_set& outSet, MNATIVE_SOCKET inSocket);
    void RemoveSelectEvent(fd_set& outSet, MNATIVE_SOCKET inSocket);

    // 이벤트 처리
    void ReadEvent(fd_set& inReadSet);
    void WriteEvent(fd_set& inWriteSet);

    // 이벤트 소켓 리스트를 얻는다
    MSIZE GetEventSocketList(std::vector<MNATIVE_SOCKET>& outList, fd_set& inSet);

    // 타임아웃 체크
    void CheckTimeout();


    // 설정
    virtual const MIOModelConfig* GetConfig() override {
        return &Config;
    }
    
protected:

    // 읽기 / 쓰기 셋
	fd_set ReadSet;
	fd_set WriteSet;


    //--------------------------------------------------------------
    // 임시로 사용되는 정보
    //--------------------------------------------------------------
    // 임시로 사용되는 이벤트 소켓 리스트
    std::vector<MNATIVE_SOCKET> TempEventSocketList;

    // 처리에 사용되는 임시 메모리 풀
    MPool<MMemory> TempMemoryPool;

    // 처리에 사용되는 임시 버퍼 풀
    MPool<MBuffer> TempBufferPool;

    // 소켓 리스트
    std::vector<MSelectSocket*> TempSocketList;

    // 리눅스용
	MUINT32 CheckFDCount = 0;

    // 소켓 매니저
    MSocketManager<MSelectSocket> SocketManager;

    // 타임아웃 체크 셋
    std::set<MSelectSocket*> CheckTimeoutSet;

public:
    // 설정 정보
    MIOModelSelectConfig Config;
};




