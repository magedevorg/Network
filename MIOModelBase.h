#pragma once

#include "MIOModelEvent.h"
#include "MDateTime.h"
#include "MIOModelEvent.h"




//---------------------------------------------------------
// IO Model 세팅 정보
// IOModel을 상속받는 쪽에서 필요한 정보가 더있다면 설정
//---------------------------------------------------------
struct MIOModelConfig
{
public:
    // 소켓수
    MSIZE NumberOfSocket = 1;

    // 버퍼 사이즈
    MSIZE SendBufferSize = 256;
    MSIZE RecvBufferSize = 256;

    // 리슨 소켓 큐
    MSIZE ListenSocketQueue = 5;

    // 접속시 타임아웃 초
    MINT32 ConnectTimeoutMilliseconds = 4000;
};



//--------------------------------------------------
// 이벤트 발생시 처리 리스너
//--------------------------------------------------
class MIOModelEventListener
{
public:
    virtual ~MIOModelEventListener() {}

public:
    virtual void OnConnect(const MIOModelEventOnConnect& inParam) {}
    virtual void OnDisconnected(const MIOModelEventOnDisconnected& inParam) {}
    virtual void OnListen(const MIOModelEventOnListen& inParam) {}
    virtual void OnAccept(const MIOModelEventOnAccept& inParam) {}
    virtual void OnPacket(MUINT64 inUniqueID, MUINT32 inPacketID, void* inData, MSIZE inSize) {}
};




