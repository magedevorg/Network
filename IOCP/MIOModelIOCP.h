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
// IOCP ó��
//---------------------------------------------------------------------
class MIOModelIOCP : public MIOModel
{
public:
	// �ʱ�ȭ
	virtual EMError Start(MIOModelEventListener* inListener) override;

	// ���� ó��
	virtual void Update() override;

	// ����
	virtual void Release() override;


	//-----------------------------------------------------
	// �������̽�
	//-----------------------------------------------------
	// ����
	virtual void Connect(const MIOModelEventConnect& inParam) override;

	// ���� ����
	virtual void Disconnect(const MIOModelEventDisconnect& inParam) override;

	// ����
	virtual void Listen(const MIOModelEventListen& inParam) override;

	// ��Ŷ ����
	virtual void SendPacket(MUINT64 inUniqueID, MUINT32 inPacketID, class MPacket& inPacket) override;
	
public:
	// ���� ����/����
	MNATIVE_SOCKET CreateNativeSocket();
	void CloseNativeSocket(MNATIVE_SOCKET inSocketID);

	//-----------------------------------------------------
	// ���� ����
	//-----------------------------------------------------
	virtual const MIOModelConfig* GetConfig() override {
		return &Config;
	}

	//-----------------------------------------------------
	// ���� �߰�
	//-----------------------------------------------------
	MIOCPSocket* AddSocket(MNATIVE_SOCKET inSocket);
	
	//-----------------------------------------------------
	// �̺�Ʈ ���� ó��
	//-----------------------------------------------------
	// work thread
	void PushEvent(void* inEvent, MSIZE inSize);

	// �̺�Ʈ ó��
	void ProcessEvent();

	//-----------------------------------------------------
	// IOCP �ڵ�
	//-----------------------------------------------------
	HANDLE GetIOCP() const {
		return IOCP;
	}

protected:
	// IOCP �ڵ�
	HANDLE IOCP = INVALID_HANDLE_VALUE;

	//-------------------------------------------------------
	// ������ ó��
	//-------------------------------------------------------
	std::vector<class MIOModelIOCPWorkerThread*> WorkThreadList;
	 
	//-------------------------------------------------------
	// ���� ���� �Ŵ���
	//-------------------------------------------------------
	MSocketManager<MIOCPSocket> SocketManager;
	
	//-------------------------------------------------------
	// work thread -> main thread�� ���޵Ǵ� �̺�Ʈ ����
	//-------------------------------------------------------
	MBuffer* EventPushBuffer = nullptr;
	MBuffer* EventProcessBuffer = nullptr;

	std::mutex Mutex_EventBuffer;


	//-------------------------------------------------------
	// Ǯ
	//-------------------------------------------------------
	// �޸� Ǯ
	MPool<MMemory> TempMemoryPool;

public:
	// ���� ����
	MIOModelIOCPConfig Config;
};


#endif
