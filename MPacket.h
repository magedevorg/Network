#pragma once


#include "MStream.h"
#include "MBuffer.h"
#include "MError.h"
#include "MPool.h"
#include "MMemory.h"

#include <vector>
#include <queue>




//----------------------------------------------------------
// 패킷 헤더
// 따로 관리됨
//----------------------------------------------------------
struct MPacketHeader
{
public:
    // 패킷 식별자
    MUINT32 PacketID = 0;

    // 패킷 사이즈(패킷 사이즈만)
    MSIZE PacketSize = 0;
};



//----------------------------------------------------------
// 패킷 클래스
//----------------------------------------------------------
class MPacket : public MSerializable
{
    friend class MIOModel;
public:
    //------------------------------------------------
    // 패킷에 필요한 자료형 선언
    //------------------------------------------------
    // 패킷용 가변 배열
    template<typename T>
    class MVariableArray : public MSerializable
    {
    public:
        MVariableArray()
        {
            // 패킷 메모리 풀에서 얻어옴
            Memory = MPacket::MemoryPool.Pop();
        }

        ~MVariableArray()
        {
            // 패킷 메모리 풀로 반납
            MPacket::MemoryPool.Push(Memory);
        }
    public:
        // 할당
        void SetCount(MSIZE inCount)
        {
            Count = inCount;
            Memory->Alloc(sizeof(T) * inCount, MTRUE);
        }

        MSIZE GetCount() {
            return Count;
        }

        // 용량
        MSIZE GetCapacity() {
            return Memory->GetSize();
        }

        T& operator[](MINDEX inIndex)
        {
            return (T&)(*(Memory->GetPointer() + (sizeof(T) * inIndex)));
        }

        // 직렬화
        virtual void Serialize(MStream& inStream) override
        {
            inStream.Process(&Count, sizeof(Count));

            const MSIZE size = sizeof(T) * Count;
            if (MTRUE == inStream.IsRead())
            {
                // 읽기 일경우 메모리 할당
                Memory->Alloc(size, MTRUE);    
            }
            
            inStream.Process(Memory->GetPointer(), size);
        }
  
    protected:
        // 사이즈
        MSIZE Count = 0;

        // 할당 받은 메모리
        MMemory* Memory = nullptr;
    };
    

public:
    virtual ~MPacket() {}

protected:
    // 직렬화
    virtual void Serialize(MStream& inStream) = 0;
    
protected:
    // 패킷에서 사용하는 메모리 풀
    static MPool<MMemory> MemoryPool;
};






//----------------------------------------------------------
// 예약된 패킷
// 서버에서 클라이언트로 전달해줘야하는 정보
//----------------------------------------------------------
enum class EMPacket : MUINT32
{
    None                    = 0,
    ConnectResponse         = 1,            // 서버에서 접속 요청에 대한 응답을 넘겨줌

    UserPacket              = 100 + 1,      // 유저 패키
};




//----------------------------------------------------------
// 서버에서 전달하는 접속 응답 패킷
//----------------------------------------------------------
class MPacketConnectResponse : public MPacket
{
public:
    // 직렬화
    virtual void Serialize(MStream& inStream) override;

public:
    // 결과
    EMError Error;
};
