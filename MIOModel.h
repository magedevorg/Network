#pragma once


#include "MError.h"
#include "MIOModelBase.h"
#include "MBuffer.h"
#include "MPacket.h"

#include "MIOModelEvent.h"

#include <vector>
#include <stack>
#include <map>
#include <set>




//---------------------------------------------------------
// IOModel 
// 통신관련 인터페이스와 유틸 함수 제공
//---------------------------------------------------------
class MIOModel
{
public:
    MIOModel() {}
    virtual ~MIOModel() {}

public:
    //-----------------------------------------------------
    // 플로우
    //-----------------------------------------------------
    // 시작 
    virtual EMError Start(MIOModelEventListener* inListener);

    // 갱신 처리
    virtual void Update() = 0;

    // 해제
    virtual void Release();
   
    //-----------------------------------------------------
    // 인터페이스
    //-----------------------------------------------------
    // 접속
    virtual void Connect(const MIOModelEventConnect& inParam) = 0;

    // 접속 끊기
    virtual void Disconnect(const MIOModelEventDisconnect& inParam) = 0;

    // 리슨
    virtual void Listen(const MIOModelEventListen& inParam) = 0;

    // 패킷 전송    
    virtual void SendPacket(MUINT64 inUniqueID, MUINT32 inPacketID, class MPacket& inPacket) = 0;

public:

    // 블러킹 모드 설정
    virtual void SetSocketBlockingMode(MNATIVE_SOCKET inSocket, MBOOL inIsBlockingMode);

    // 주소 설정
    virtual void SetAddress(sockaddr_in& outAddr, MINT16 inFamily, const MCHAR* inIP, MINT16 inPort);

    // 설정 파일을 얻는다
    // 상속받는곳에서 거기에 사용되는 설정파일을 만들고 리턴 처리
    virtual const MIOModelConfig* GetConfig() = 0;

    // 패킷을 버퍼에 추가
    void PushPacket(MBuffer& inBuffer, MUINT32 inPacketID, MPacket& inPacket, MMemory& inPacketMemory);
    MBOOL PopPacket(MPacketHeader& inHeader, class MMemory& inData, class MBuffer& inBuffer);

    // 마지막 에러를 얻는다
    EMError GetLastError();


    // 공통적인 처리를 하고 리스너로 전달
    void OnPacket(MUINT64 inUniqueID, MUINT32 inPacketID, void* inData, MSIZE inSize);


protected:     
    //----------------------------------------------------
    // 이벤트 리스너
    //----------------------------------------------------
    class MIOModelEventListener* EventListener = nullptr;
};