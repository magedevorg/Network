#pragma once


#include "MDateTime.h"




// 리눅스 계열 자료형 define
#if( MPLATFORM == MPLATFORM_LINUX || MPLATFORM == MPLATFORM_MAC)
#	define MNATIVE_SOCKET   			int
#	define MSOCKET_INVALID				-1
#	define MSOCKET_ERROR				-1
#	define MSOCKET_LEN_TYPE				socklen_t
#endif


#if (MPLATFORM == MPLATFORM_WINDOWS)
#	define MNATIVE_SOCKET  	    		SOCKET
#	define MSOCKET_INVALID				INVALID_SOCKET
#	define MSOCKET_ERROR				SOCKET_ERROR
#	define MSOCKET_LEN_TYPE				int
#elif (MPLATFORM == MPLATFORM_LINUX)
    // 리눅스 종속적 처리
#elif (MPLATFORM == MPLATFORM_MAC)
    // 맥 종속적 처리
#endif



//---------------------------------------------------------
// 소켓 상태 
//---------------------------------------------------------
enum class EMSocketState
{
    None = 0,
    Connecting,         // 접속중
    Listen,             // 리슨
    Run,                // 동작중
    Disconnecting,      // 접속 끊는중
};





//---------------------------------------------------------
// 소켓 정보 클래스
// 소켓에 추가적으로 필요한 정보를 가진다
// 각 IOModel에 따라서 상속받아 맞게 사용
//---------------------------------------------------------
class MSocket
{
public:
    MSocket(MINDEX inIndex)
        : Index(inIndex)
    {}

    virtual ~MSocket()
    {
    }

public:
    void SetNativeSocket(MNATIVE_SOCKET inNativeSocket) {
        NativeSocket = inNativeSocket;
    }

    virtual void SetUniqueID(MUINT64 inUniqueID) {
        UniqueID = inUniqueID;
    }

public:
    MNATIVE_SOCKET GetNativeSocket() const {
        return NativeSocket;
    }

    MINDEX GetIndex() const {
        return Index;
    }

    MUINT64 GetUniqueID() const {
        return UniqueID;
    }

    void SetState(EMSocketState inState)
    {
        if (State == inState) {
            return;
        }

        State = inState;
        StateStartTime = MDateTime::UtcNow();
    }

    EMSocketState GetState() const {
        return State;
    }

    const MDateTime& GetStateStartTime() const {
        return StateStartTime;
    }

    virtual void Clear()
    {
        NativeSocket = MSOCKET_INVALID;
        UniqueID = 0;
        State = EMSocketState::None;
    }

protected:
    // 배열상의 인덱스
    MINDEX Index = MINDEX_NONE;

    // 소켓
    MNATIVE_SOCKET NativeSocket = MSOCKET_INVALID;

    // 유니크 아이디
    MUINT64 UniqueID = 0;

    // 세션 상태    
    EMSocketState State = EMSocketState::None;

    // 상태 시작 시간
    MDateTime StateStartTime;
};



//---------------------------------------------------------
// 소켓 관리 클래스
// [네이티브 소켓]과 사용하는 IO모델에 따라 만든 [소켓 클래스]를 연결하고 관리
//---------------------------------------------------------
template <typename T>
class MSocketManager
{
public:
    ~MSocketManager() {
        Clear();
    }

public:
    // 소켓 매니저 초기화
    void InitSocketManager(MSIZE inCount)
    {
        // 정보 객체를 생성하고 풀에 등록
        SocketList.resize(inCount);
        for (MINDEX i = inCount - 1; 0 <= i; --i)
        {
            T* socket = new T(i);
            SocketList[i] = socket;
            AvailableSocketPool.push(socket);
        }
    }

    // 소켓 정보 추가
    T* Add(MNATIVE_SOCKET inNativeSocket)
    {
        if (true == AvailableSocketPool.empty()) {
            return nullptr;
        }

        // 활성화 소켓정보를 얻는다
        T* socket = AvailableSocketPool.top();
        AvailableSocketPool.pop();

        // 유니크 아이디 설정
        const MUINT64 uniqueID = GenerateUniqueID();
        socket->SetNativeSocket(inNativeSocket);
        socket->SetUniqueID(uniqueID);

        // 소켓 맵에 등록
        ActiveSocketMap[inNativeSocket] = socket;
        ActiveUniqueIDMap[uniqueID] = socket;

        return socket;
    }

    // 소켓 제거
    void RemoveByNativeSocket(MNATIVE_SOCKET inNativeSocket)
    {
        auto findIter = ActiveSocketMap.find(inNativeSocket);
        if (ActiveSocketMap.end() == findIter) {
            return;
        }

        T* socket = findIter->second;

        // 소켓 맵에서제거
        ActiveSocketMap.erase(findIter);

        // 유니크 아이디맵에서제거
        const MUINT64 uniqueID = socket->GetUniqueID();
        ActiveUniqueIDMap.erase(uniqueID);

        // 반납
        AvailableSocketPool.push(socket);
    }

    // 인덱스로 소켓정보를 얻는다
    T* FindByIndex(MINDEX inIndex) {
        return static_cast<T>(SocketList[inIndex]);
    }

    // 유니크 아이디로 얻는다
    T* FindByUniqueID(MUINT64 inUniqueID)
    {
        auto findIter = ActiveUniqueIDMap.find(inUniqueID);
        if (ActiveUniqueIDMap.end() != findIter) {
            return findIter->second;
        }

        return nullptr;
    }

    // 소켓으로 얻는다
    T* FindByNativeSocket(MNATIVE_SOCKET inNativeSocket)
    {
        auto findIter = ActiveSocketMap.find(inNativeSocket);
        if (ActiveSocketMap.end() != findIter) {
            return findIter->second;
        }

        return nullptr;
    }

    void Clear()
    {
        // 맵 정리
        ActiveSocketMap.clear();

        // 풀 정리
        while (false == AvailableSocketPool.empty()) {
            AvailableSocketPool.pop();
        }

        // 리스트 정리
        for (auto socket : SocketList) {
            delete socket;
        }
        SocketList.clear();
    }

    // 사용할수 있는 소켓 카운트
    MSIZE GetAvailableSocketCount()
    {
        return AvailableSocketPool.size();
    }

protected:
    MUINT64 GenerateUniqueID()
    {
        MUINT64 uniqueID = NextUniqueID;
        ++NextUniqueID;

        if (NextUniqueID >= 9223372036854775808) {
            NextUniqueID = 1;
        }
        return uniqueID;
    }

public:
    // 소켓 정보 리스트
    std::vector<T*> SocketList;

    // 활성화된 소켓 맵
    std::map<MNATIVE_SOCKET, T*> ActiveSocketMap;
    std::map<MUINT64, T*> ActiveUniqueIDMap;


protected:
    // 사용가능한 소켓 정보 풀
    std::stack<T*> AvailableSocketPool;

    // 유니크 아이디
    MUINT64 NextUniqueID = 1;
};





