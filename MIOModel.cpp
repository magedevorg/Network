#include "MIOModel.h"
#include "MMemory.h"
#include "MPacket.h"



EMError MIOModel::Start(MIOModelEventListener* inListener)
{
	// 리스너 설정
	EventListener = inListener;

#if (MPLATFORM == MPLATFORM_WINDOWS)
	{
		// 윈도우 소켓 초기화
		BYTE major = 2;
		BYTE minor = 2;
		WORD version = MAKEWORD(minor, major);

		WSADATA data;
		if (SOCKET_ERROR == ::WSAStartup(version, &data)) {
			return EMError::WinsockInitFailed;
		}
	}
#endif
	
	const MIOModelConfig* config = GetConfig();

	// 패킷에서 사용하는 메모리 풀 처리
	MPacket::MemoryPool.InitPool(MTRUE, 3, MTRUE,
		[]()->MMemory*
		{
			return new MMemory(128);
		});
		
	return EMError::None;
}

void MIOModel::Release()
{
#if (MPLATFORM == MPLATFORM_WINDOWS)
	{
		::WSACleanup();
	}
#endif
}





void MIOModel::SetSocketBlockingMode(MNATIVE_SOCKET inSocket, MBOOL inIsBlockingMode)
{
	// 블러킹 모드 설정
#if (MPLATFORM == MPLATFORM_WINDOWS)
	{
		u_long mode = 0;
		if (MFALSE == inIsBlockingMode) {
			mode = 1;
		}

		::ioctlsocket(inSocket, FIONBIO, &mode);
	}
#else
	{
		int mode = 0;
		if (MFALSE == inIsBlockingMode) {
			mode = 1;
		}

		ioctl(inSocket, FIONBIO, (char*)&mode);
	}
#endif
}


void MIOModel::SetAddress(sockaddr_in& outAddr, MINT16 inFamily, const MCHAR* inIP, MINT16 inPort)
{
	::memset(&outAddr, 0, sizeof(outAddr));

	outAddr.sin_family = inFamily;

	// inet_pton() 함수는 사람이 알아보기 쉬운 텍스트 형태의 IPv4와 IPv6주소를 binary 형태로 변환 하는 기능을 한다
	// IPv4 - AF_INET, IPv6 - AF_INET6
	// IPv4의 경우 주소가 ddd.ddd.ddd.ddd로 ddd자리에 0 ~ 255범위의 10진수가 온다
	// IPv6의 경우 cccc:cccc:cccc:cccc:cccc:cccc:cccc:cccc 16비트씩(c하나에 4bit) 콜론으로 구분하여 8개로 구성(총 128bit)
	// https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=nackji80&logNo=220575860980

#if(MPLATFORM == MPLATFORM_WINDOWS)
	{
		::inet_pton(AF_INET, inIP, &outAddr.sin_addr.S_un.S_addr);
	}
#else
	{
		::inet_pton(AF_INET, inIP, &outAddr.sin_addr);
	}
#endif

	outAddr.sin_port = htons(inPort);
}

EMError MIOModel::GetLastError()
{
	EMError error = EMError::None;

#if(MPLATFORM == MPLATFORM_WINDOWS)
	{
		const int errorNo = ::WSAGetLastError();
		switch (errorNo)
		{
		case WSAEWOULDBLOCK:
		{
			error = EMError::WouldBlock;
		} break;
		case WSAEADDRINUSE:
		{
			error = EMError::AlreadyUseAddress;
		} break;
		}
	}
#else
	{
		// 디버깅용으로 로컬에 저장
		int checkErrno = errno;

		switch (errno)
		{
		case EWOULDBLOCK:	// or EAGAIN
		{
			error = EMError::WouldBlock;
		} break;
		case EADDRINUSE:	// 이미 사용중인 주소
		{
			error = EMError::AlreadyUseAddress;
		} break;
		}
	}
#endif

	return error;
}


void MIOModel::OnPacket(MUINT64 inUniqueID, MUINT32 inPacketID, void* inData, MSIZE inSize)
{
	if (inPacketID == (MUINT32)EMPacket::ConnectResponse)
	{
		MByteReadStream readStream(inData);
		
		MPacketConnectResponse packet;
		packet.Serialize(readStream);

		// 접속에 대한 응답
		MIOModelEventOnConnect event(packet.Error, inUniqueID);
		EventListener->OnConnect(event);
		return;
	}


	EventListener->OnPacket(inUniqueID, inPacketID, inData, inSize);
}


void MIOModel::PushPacket(MBuffer& inBuffer, MUINT32 inPacketID, class MPacket& inPacket, MMemory& inPacketMemory)
{
	// 패킷을 시리얼라이즈
	MMemoryWriteStream writeStream(inPacketMemory);
	writeStream.Process(inPacket);

	MPacketHeader header;
	header.PacketID = inPacketID;
	header.PacketSize = writeStream.GetPos();
	
	// 헤더 푸시
	inBuffer.Push(&header, sizeof(header), MTRUE);

	// 데이터 push
	if (0 < header.PacketSize) {
		inBuffer.Push(inPacketMemory.GetPointer(), header.PacketSize, MTRUE);
	}
}


MBOOL MIOModel::PopPacket(MPacketHeader& inHeader, MMemory& inData, MBuffer& inSrcBuffer)
{
	const MSIZE dataSize = inSrcBuffer.GetReadableSize();
	if (dataSize < sizeof(inHeader)) {
		return MFALSE;
	}

	// 헤더를 읽는다
	inSrcBuffer.Read(&inHeader, sizeof(inHeader));

	// 데이터가 전부오지 않았다면 넘어간다
	if ((dataSize - sizeof(inHeader)) < inHeader.PacketSize) {
		return MFALSE;
	}

	// 헤더 팝
	inSrcBuffer.Read(&inHeader, sizeof(inHeader));
	inSrcBuffer.Pop(sizeof(inHeader));

	// 데이터 팝
	if (0 < inHeader.PacketSize) 
	{
		inSrcBuffer.Read(inData.GetPointer(), inHeader.PacketSize);
		inSrcBuffer.Pop(inHeader.PacketSize);
	}
	return MTRUE;
}