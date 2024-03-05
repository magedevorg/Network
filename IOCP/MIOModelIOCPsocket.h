#pragma once

#include "MNetwork.h"
#include "MBuffer.h"
#include "MIOModelIOCPMessage.h"

#include <atomic>
#include <mutex>

#if (MPLATFORM == MPLATFORM_WINDOWS)

//-------------------------------------------------------------
// IOCP 소켓정보
//-------------------------------------------------------------
class MIOCPSocket : public MSocket
{
public:
	MIOCPSocket(MINDEX inIndex);
	virtual ~MIOCPSocket();

public:
	// IOCP 핸들 설정
	void SetIOCP(HANDLE inIOCP) {
		IOCP = inIOCP;
	}

	// 갱신
	void Update();

	//-----------------------------------------------------
	// 인터페이스
	//-----------------------------------------------------
	// Accept 설정
	void Accept();

	// recv설정
	void Recv();

	// 전송
	void Send(const MBYTE* inData, MSIZE inSize);

	
	//-----------------------------------------------------
	// 버퍼정보 
	//-----------------------------------------------------
	// 전송 패킷 추가
	void PushSendPacket(MUINT32 inPacketID, class MPacket& inPacket, MMemory* inTempMemory);

	// 전송 처리
	void SendBufferData();

	
	// 
	void PushRecvPacket(void* inData, MSIZE inSize);


	MBuffer* GetRecvBuffer() const {
		return RecvBuffer;
	}

protected:
	HANDLE IOCP = NULL;

	// 송신 버퍼
	MBuffer* SendPushBuffer = nullptr;
	MBuffer* SendBuffer = nullptr;

	// 전송 플래그
	std::atomic<bool> SendFlag = false;

	//---------------------------------------------------------
	// 수신처리
	//---------------------------------------------------------
	MBuffer* RecvPushBuffer = nullptr;
	MBuffer* RecvBuffer = nullptr;

	std::atomic<bool> RecvFlag = false;

	
	//-----------------------------------------------------------
	// 요청 메시지
	//-----------------------------------------------------------
	// 패킷 전송 메시지
	MIOCPRequestSendMessage RequestSendMessage;

	//-----------------------------------------------------------
	// 완료 메시지
	//-----------------------------------------------------------
	// Accept 완료 메시지
	MIOCPCompletionAcceptMessage CompletionAcceptMessage;

	// Recv 완료 메시지
	MIOCPCompletionRecvMessage CompletionRecvMessage;

	// Send 완료 메시지
	MIOCPCompletionSendMessage CompletionSendMessage;
};


#endif
