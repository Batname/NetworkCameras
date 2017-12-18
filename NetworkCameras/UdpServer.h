#pragma once

#include<stdio.h>
#include<winsock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

// Implement this examples
// http://www.binarytides.com/udp-socket-programming-in-winsock/
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms737889(v=vs.85).aspx

// UDP data structure example
// https://wiki.unrealengine.com/UDP_Socket_Sender_Receiver_From_One_UE4_Instance_To_Another

namespace BT
{
	class App;

	class UdpServer
	{
	public:
		UdpServer(unsigned int Port, unsigned int SendBuflen);
		~UdpServer();

		void Broadcast();

		void SetSendBuffer(const char* Data, unsigned int Len);


	private:
		unsigned int Port;
		unsigned int SendBuflen;
		unsigned int RequestBuflen;


		char* SendBuffer;
		char* RequestBuffer;

	// app access
	private:
		App* app;

	// Winsocket
	private:
		SOCKET s;
		struct sockaddr_in server, si_other;
		int slen, recv_len;
		WSADATA wsa;
	};
}

