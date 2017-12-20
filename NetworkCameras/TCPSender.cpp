#include "TCPSender.h"

#include <sstream>

#include "App.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma warning( disable : 4996)


BT::TCPSender::TCPSender(std::string Server, std::string Port)
	: Server(Server)
	, Port(Port)
{
	// access to app
	app = BT::App::GetApp();


	//Initialise winsock
	BT::Print("Initialising Winsock...");
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		{
			std::ostringstream SockError;
			SockError << "WSAStartup failed with error : " << WSAGetLastError();
			BT::Print(SockError.str().c_str());
		}

		app->ErrorExit(EXIT_FAILURE);
	}
	BT::Print("Initialised.");

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(Server.c_str(), Port.c_str(), &hints, &result);
	if (iResult != 0) {

		{
			std::ostringstream SockError;
			SockError << "getaddrinfo failed with error : " << WSAGetLastError();
			BT::Print(SockError.str().c_str());
		}

		app->ErrorExit(EXIT_FAILURE);
	}


	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			{
				std::ostringstream SockError;
				SockError << "socket failed with errorr : " << WSAGetLastError();
				BT::Print(SockError.str().c_str());
			}

			app->ErrorExit(EXIT_FAILURE);
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		{
			std::ostringstream SockError;
			SockError << "Unable to connect to server! " << WSAGetLastError();
			BT::Print(SockError.str().c_str());
		}
		app->ErrorExit(EXIT_FAILURE);
	}

	BT::Print("Socket created");
}


BT::TCPSender::~TCPSender()
{
	// release socket
	closesocket(ConnectSocket);
	WSACleanup();
}

void BT::TCPSender::Broadcast(const char * Data, unsigned int Len)
{
	iResult = send(ConnectSocket, Data, Len, 0);
	if (iResult == SOCKET_ERROR) {
		{
			std::ostringstream SockError;
			SockError << "send failed with error: " << WSAGetLastError();
			BT::Print(SockError.str().c_str());
		}
	}
}