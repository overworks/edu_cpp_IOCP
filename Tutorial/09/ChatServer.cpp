#include "ChatServer.h"

void
ChatServer::Run(uint32_t maxClient)
{
	auto sendPacketFunc = [this](UINT32 clientIndex, UINT16 packetSize, const char* pSendPacket)
	{
		SendMsg(clientIndex, packetSize, pSendPacket);
	};

	m_pPacketManager = std::make_unique<PacketManager>(maxClient, sendPacketFunc);
	m_pPacketManager->Run();

	StartServer(maxClient);
}

void
ChatServer::End()
{
	m_pPacketManager->End();

	DestroyThread();
}

void
ChatServer::OnConnect(uint32_t clientIndex)
{
	printf(u8"[OnConnect] 클라이언트: Index(%d)\n", clientIndex);

	PacketInfo packet{ clientIndex, (UINT16)PACKET_ID::SYS_USER_CONNECT, 0 };
	m_pPacketManager->PushSystemPacket(packet);
}

void
ChatServer::OnClose(uint32_t clientIndex)
{
	printf(u8"[OnClose] 클라이언트: Index(%d)\n", clientIndex);

	PacketInfo packet{ clientIndex, (UINT16)PACKET_ID::SYS_USER_DISCONNECT, 0 };
	m_pPacketManager->PushSystemPacket(packet);
}

void
ChatServer::OnReceive(uint32_t clientIndex, uint32_t size, const char* pData)
{
	printf(u8"[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex, size);

	m_pPacketManager->ReceivePacketData(clientIndex, size, pData);
}