#pragma once



#include "MNetwork.h"
#include <thread>
#include <mutex>

#include "MBuffer.h"
#include "MMemory.h"

#if (MPLATFORM == MPLATFORM_WINDOWS)


//-------------------------------------------------------------
// IOCP 작업 쓰레드
//-------------------------------------------------------------
class MIOModelIOCPWorkerThread
{
public:
	MIOModelIOCPWorkerThread(class MIOModelIOCP* inOwner);
	~MIOModelIOCPWorkerThread();

public:
	void Run();
	void Stop();

protected:
	// 갱신 처리
	void Update();


	//-------------------------------------------------------------
	// 각 메시지 처리
	//-------------------------------------------------------------
	// accpet 완료 메시지
	void ProcessCompletionAcceptMessage(struct MIOCPCompletionAcceptMessage* inMessage);

	// recv 완료 메시지
	void ProcessCompletionRecvMessage(struct MIOCPCompletionRecvMessage* inMessage, DWORD inSize);

protected:
	// 소유자
	class MIOModelIOCP* Owner = nullptr;

	// IOCP 객체
	HANDLE IOCP;

	// 쓰레드 객체
	std::thread WorkerThread;

	// 임시 정보
	MBuffer TempBuffer;
	MMemory TempMemory;
};


#endif
