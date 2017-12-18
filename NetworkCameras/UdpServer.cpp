#include "UdpServer.h"

#include <sstream>

#include "App.h"

#pragma warning( disable : 4996)

BT::UdpServer::UdpServer(unsigned int Port, unsigned int SendBuflen)
	: Port(Port)
	, SendBuflen(SendBuflen)
	, RequestBuflen(512)
{
	// access to app
	app = BT::App::GetApp();


	// allacate send buffer
	SendBuffer = new char[SendBuflen] {0};
	RequestBuffer = new char[RequestBuflen] {0};

	slen = sizeof(si_other);

	//Initialise winsock
	BT::Print("Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		{
			std::ostringstream SockError;
			SockError << "Failed. Error Code : " << WSAGetLastError();
			BT::Print(SockError.str().c_str());
		}

		app->ErrorExit(EXIT_FAILURE);
	}
	BT::Print("Initialised.");


	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		{
			std::ostringstream SockError;
			SockError << "Could not create socket : " << WSAGetLastError();
			BT::Print(SockError.str().c_str());
		}
		app->ErrorExit(EXIT_FAILURE);
	}
	BT::Print("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(Port);


	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{

		{
			std::ostringstream SockError;
			SockError << "Bind failed with error code : " << WSAGetLastError();
			BT::Print(SockError.str().c_str());
		}
		app->ErrorExit(EXIT_FAILURE);
	}
	BT::Print("Bind done");
}


BT::UdpServer::~UdpServer()
{
	// release socket
	closesocket(s);
	WSACleanup();

	// release buffers
	delete[] SendBuffer;
	SendBuffer = nullptr;

	delete[] RequestBuffer;
	RequestBuffer = nullptr;
}

void BT::UdpServer::Broadcast()
{
	while (1)
	{
		//clear the buffers by filling null, it might have previously received data
		memset(RequestBuffer, '\0', RequestBuflen);

		//try to receiverequest
		if ((recv_len = recvfrom(s, RequestBuffer, RequestBuflen, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			{
				std::ostringstream SockError;
				SockError << "recvfrom() failed with error code  " << WSAGetLastError();
				BT::Print(SockError.str().c_str());
			}
		}

		//now reply with image
		if (SendBuffer[0] == 0)
		{
			continue;
		}
		if (sendto(s, SendBuffer, RequestBuflen, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			{
				std::ostringstream SockError;
				SockError << "sendto() failed with error code : " << WSAGetLastError();
				BT::Print(SockError.str().c_str());
			}
		}
	}
}

void BT::UdpServer::SetSendBuffer(const char * Data, unsigned int Len)
{
	memset(SendBuffer, '\0', SendBuflen);
	memcpy(SendBuffer, Data, RequestBuflen);
}
