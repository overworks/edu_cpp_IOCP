#include "PacketManager.h"
#include "ChatServer.h"

ChatServer::ChatServer()
{
	mPacketManager = new PacketManager();
}

ChatServer::~ChatServer()
{
	delete mPacketManager;
}

void
ChatServer::Run(int maxClient)
{
	auto sendPacketFunc = [this](int clientIndex, uint16_t packetSize, const char* pSendPacket)
	{
		SendMsg(clientIndex, packetSize, pSendPacket);
	};

	mPacketManager->Init(maxClient, sendPacketFunc);
	mPacketManager->Run();

	StartServer(maxClient);
}

void
ChatServer::End()
{
	mPacketManager->End();

	DestroyThread();
}

void
ChatServer::OnConnect(uint32_t clientIndex)
{
	printf(u8"[OnConnect] 클라이언트: Index(%d)\n", clientIndex);

	PacketInfo packet{ clientIndex, (UINT16)PACKET_ID::SYS_USER_CONNECT, 0 };
	mPacketManager->PushSystemPacket(packet);
}

void
ChatServer::OnClose(uint32_t clientIndex)
{
	printf(u8"[OnClose] 클라이언트: Index(%d)\n", clientIndex);

	PacketInfo packet{ clientIndex, (UINT16)PACKET_ID::SYS_USER_DISCONNECT, 0 };
	mPacketManager->PushSystemPacket(packet);
}

void
ChatServer::OnReceive(uint32_t clientIndex, uint32_t dataSize, const char* pData)
{
	printf(u8"[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex, dataSize);

	mPacketManager->ReceivePacketData(clientIndex, dataSize, pData);
}