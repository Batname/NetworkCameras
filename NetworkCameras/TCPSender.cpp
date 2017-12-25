#include "TCPSender.h"

#include <sstream>

#include "App.h"

#ifdef __linux__ 

#elif _WIN32
	// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
	#pragma comment (lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")
	
	#pragma warning( disable : 4996)
#else
#endif

#define BUFSIZE 64

BT::TCPSender::TCPSender(std::string Server, std::string Port)
	: Server(Server)
	, Port(Port)
	, bIsConnected(false)
{
	// access to app
	app = BT::App::GetApp();

	{
		std::ostringstream Log;
		Log << "Run camera TCP sender: Server: " << Server
		<< " Port: " << Port;
		BT::Print(Log.str().c_str());
	}

	Buffer = new char[1264896];

	Run();
}

BT::TCPSender::~TCPSender()
{
	// release socket
	ReleaseSocket();

	delete[] Buffer;
	Buffer = nullptr;
}

#ifdef __linux__
void BT::TCPSender::Run()
{
	Connect();
}



void BT::TCPSender::Broadcast(const char * Data, unsigned int Len)
{
	if (!bIsConnected)
	{
		return;
	}

	mtx.lock();
	memcpy ( Buffer, Data, Len );
	mtx.unlock();

	/* send the message line to the server */
    int n = write(sockfd, Buffer, Len);
    if (n < 0)
    {
      	BT::Print("ERROR writing to socket");
    }
    else
    {
    	{
    		std::ostringstream Log;
    		Log << "convertedImage.GetDataSize: " << Len << " Data[100]: 0x" << std::hex << (int)Data[100];
    		BT::Print(Log.str().c_str());
    	}
    }


    // wait for server answer
    char buf[BUFSIZE];
    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
    {
     	BT::Print("ERROR reading from socket");
     	ReleaseSocket();
     	Connect();
    }

}

void BT::TCPSender::Connect()
{
	/* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        BT::Print("ERROR opening socket");
    }
    else
    {
    	BT::Print("Socket created");
    }

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(Server.c_str());
    if (server == NULL)
    {
        {
			std::ostringstream SockError;
			SockError << "ERROR, no such host as : " << Server.c_str();
			BT::Print(SockError.str().c_str());
		}
    }
    else
    {
    	BT::Print("Host exists");
    }


    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(std::stoi(Port));

     /* connect: create a connection with the server */
    if (connect(sockfd,(struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
    	BT::Print("Error socket connection");
    }
    else
    {
    	BT::Print("Socket connection success");
    	bIsConnected = true;
    }
}

void BT::TCPSender::ReleaseSocket()
{
	bIsConnected = false;
	close(sockfd);
}

#elif _WIN32
void BT::TCPSender::Run()
{
	Connect();

	while (1)
	{
		if (ConnectSocket == INVALID_SOCKET) {
			{
				std::ostringstream SockError;
				SockError << "Unable to connect to server! " << WSAGetLastError()
				<< " Server: " << Server << " Port " << Port;
				BT::Print(SockError.str().c_str());
			}

			// if there is no connection wait 1 sec and try reconnect
			BT::Print("Reconnect in 1 sec");

			// release socket
			closesocket(ConnectSocket);
			WSACleanup();

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			Connect();
		}
		else
		{
			// Now we connected
			bIsConnected = true;
			break;
		}
	}


	BT::Print("Socket created");
}

void BT::TCPSender::Broadcast(const char * Data, unsigned int Len)
{
	if (!bIsConnected)
	{
		return;
	}

	iResult = send(ConnectSocket, Data, Len, 0);
	if (iResult == SOCKET_ERROR) {
		{
			std::ostringstream SockError;
			SockError << "send failed with error: " << WSAGetLastError()
			<< " Server: " << Server << " Port " << Port;
			BT::Print(SockError.str().c_str());
		}

		{
			// Try reconnect now
			BT::Print("Try reconnect now");

			// reconnect now
			ReleaseSocket();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			Run();
		}
	}
	else
	{
		{
			std::ostringstream Log;
			Log << "convertedImage.GetDataSize: " << Len << " Data[100]: 0x" << std::hex << (int)Data[100];
			BT::Print(Log.str().c_str());
		}
	}
}

void BT::TCPSender::Connect()
{

	//Initialise winsock
	BT::Print("Initialising Winsock...");
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		{
			std::ostringstream SockError;
			SockError << "WSAStartup failed with error : " << WSAGetLastError()
			<< " Server: " << Server << " Port " << Port;
			BT::Print(SockError.str().c_str());
		}
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
			SockError << "getaddrinfo failed with error : " << WSAGetLastError()
			<< " Server: " << Server << " Port " << Port;
			BT::Print(SockError.str().c_str());
		}
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			{
				std::ostringstream SockError;
				SockError << "socket failed with errorr : " << WSAGetLastError()
				<< " Server: " << Server << " Port " << Port;
				BT::Print(SockError.str().c_str());
			}
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
}

void BT::TCPSender::ReleaseSocket()
{
	bIsConnected = false;
	closesocket(ConnectSocket);
	WSACleanup();
}
#endif