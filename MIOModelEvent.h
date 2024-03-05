#pragma once


#include "MNetwork.h"
#include "MString.h"

#include <string>


//---------------------------------------------------
// 이벤트 타입
//---------------------------------------------------
enum class EMIOModelEventType
{
    None        = 0,

    // 요청
    Connect,            // 접속
    Disconnect,	        // 접속 끊기(응답은 오지 않는다)
    Listen,             // 리슨
    
    // 응답
    OnConnect,	        // 접속 요청에 대한 응답
    OnDisconnected,     // 상대방에서 접속을 끊음
    OnListen,           // 리슨 응답
    OnAccept,           // accept
};



//---------------------------------------------------
// 이벤트
//---------------------------------------------------
struct MIOModelEvent
{
public:
    MIOModelEvent() {}
    MIOModelEvent(EMIOModelEventType inEventType)
        : EventType(inEventType)
    {}

public:
    EMIOModelEventType EventType = EMIOModelEventType::None;
};


//---------------------------------------------------
// 접속 요청
//---------------------------------------------------
struct MIOModelEventConnect : public MIOModelEvent
{
public:
    MIOModelEventConnect() {}
    MIOModelEventConnect(const MCHAR* inIP, MINT16 inPort)
        : MIOModelEvent(EMIOModelEventType::Connect)
        , Port(inPort)
    {
        ::strcpy(IP, inIP);
    }

public:
    MCHAR IP[64] = { 0, };
    MINT16 Port = 0;
};


//---------------------------------------------------
// Disconnect
//---------------------------------------------------
struct MIOModelEventDisconnect : public MIOModelEvent
{
public:
    MIOModelEventDisconnect() {}
    MIOModelEventDisconnect(MUINT64 inUniqueID)
        : MIOModelEvent(EMIOModelEventType::Disconnect)
        , UniqueID(inUniqueID)
    {}

public:
    MUINT64 UniqueID = 0;
};


//---------------------------------------------------
// OnDisconnected
//---------------------------------------------------
struct MIOModelEventOnDisconnected : public MIOModelEvent
{
public:
    MIOModelEventOnDisconnected() {}
    MIOModelEventOnDisconnected(MUINT64 inUniqueID)
        : MIOModelEvent(EMIOModelEventType::OnDisconnected)
        , UniqueID(inUniqueID)
    {}

public:
    MUINT64 UniqueID = 0;
};


//---------------------------------------------------
// listen
//---------------------------------------------------
struct MIOModelEventListen : public MIOModelEvent
{
public:
    MIOModelEventListen() {}
    MIOModelEventListen(const MCHAR* inIP, MINT16 inPort)
        : MIOModelEvent(EMIOModelEventType::Listen)
        , Port(inPort)
    {
        ::strcpy(IP, inIP);
    }

public:
    MCHAR IP[64] = { 0, };
    MINT16 Port = 0;
};

struct MIOModelEventOnListen : public MIOModelEvent
{
public:
    MIOModelEventOnListen() {}
    MIOModelEventOnListen(EMError inError)
        : MIOModelEvent(EMIOModelEventType::OnListen)
        , Error(inError)
    {}

public:
    EMError Error = EMError::None;		// 에러
};




//---------------------------------------------------
// 접속 요청 응답
//---------------------------------------------------
struct MIOModelEventOnConnect : public MIOModelEvent
{
public:
    MIOModelEventOnConnect() {}
    MIOModelEventOnConnect(EMError inError, MUINT64 inUniqueID)
        : MIOModelEvent(EMIOModelEventType::OnConnect)
        , Error(inError)
        , UniqueID(inUniqueID)
    {}

public:
    EMError Error = EMError::None;		// 에러
    MUINT64 UniqueID = 0;
};

//---------------------------------------------------
// accept 처리
//---------------------------------------------------
struct MIOModelEventOnAccept : public MIOModelEvent
{
public:
    MIOModelEventOnAccept() {}
    MIOModelEventOnAccept(EMError inError, MNATIVE_SOCKET inSocket)
        : MIOModelEvent(EMIOModelEventType::OnAccept)
        , Error(inError)
        , Socket(inSocket)
    {}

public:
    EMError Error = EMError::None;		// 에러
    MNATIVE_SOCKET Socket = MSOCKET_INVALID;
};



