#pragma once


#include "MNetwork.h"
#include "MString.h"

#include <string>


//---------------------------------------------------
// �̺�Ʈ Ÿ��
//---------------------------------------------------
enum class EMIOModelEventType
{
    None        = 0,

    // ��û
    Connect,            // ����
    Disconnect,	        // ���� ����(������ ���� �ʴ´�)
    Listen,             // ����
    
    // ����
    OnConnect,	        // ���� ��û�� ���� ����
    OnDisconnected,     // ���濡�� ������ ����
    OnListen,           // ���� ����
    OnAccept,           // accept
};



//---------------------------------------------------
// �̺�Ʈ
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
// ���� ��û
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
    EMError Error = EMError::None;		// ����
};




//---------------------------------------------------
// ���� ��û ����
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
    EMError Error = EMError::None;		// ����
    MUINT64 UniqueID = 0;
};

//---------------------------------------------------
// accept ó��
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
    EMError Error = EMError::None;		// ����
    MNATIVE_SOCKET Socket = MSOCKET_INVALID;
};



