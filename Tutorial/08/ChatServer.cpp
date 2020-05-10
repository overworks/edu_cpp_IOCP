#include "PacketManager.h"
#include "ChatServer.h"

void
ChatServer::Run(UINT32 maxClient)
{
	auto sendPacketFunc = [this](UINT32 clientIndex, UINT16 packetSize, const char* pSendPacket)
	{
		SendMsg(clientIndex, packetSize, pSendPacket);
	};

	m_pPacketManager = std::make_unique<PacketManager>(maxClient);
	m_pPacketManager->SendPacketFunc = sendPacketFunc;
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
ChatServer::OnConnect(UINT32 clientIndex)
{
	printf(u8"[OnConnect] 클라이언트: Index(%d)\n", clientIndex);

	auto packet = std::make_shared<PacketInfo>(clientIndex, (uint16_t)PACKET_ID::SYS_USER_CONNECT);
	m_pPacketManager->PushSystemPacket(packet);
}

void
ChatServer::OnClose(UINT32 clientIndex)
{
	printf(u8"[OnClose] 클라이언트: Index(%d)\n", clientIndex);

	auto packet = std::make_shared<PacketInfo>(clientIndex, (uint16_t)PACKET_ID::SYS_USER_DISCONNECT);
	m_pPacketManager->PushSystemPacket(packet);
}

void
ChatServer::OnReceive(UINT32 clientIndex, const UINT32 size, const char* pData)
{
	printf(u8"[OnReceive] 클라이언트: Index(%d), dataSize(%d)\n", clientIndex, size);

	m_pPacketManager->ReceivePacketData(clientIndex, size, pData);
}