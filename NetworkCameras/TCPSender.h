#pragma once

#include <string.h>

#include <iostream>
#include <chrono>
#include <mutex>

#ifdef __linux__ 
//https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/tcpclient.c
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>    //socket
	#include <arpa/inet.h> //inet_addr
	#include <netdb.h> // gethostbyname
	#include <netinet/in.h>
#elif _WIN32
	#include <winsock2.h>
	#include <Ws2tcpip.h>
#else
#endif

// Implement this examples
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms737889(v=vs.85).aspx


namespace BT
{
	class App;

	class TCPSender
	{
	public:
		TCPSender(std::string Server, std::string Port);
		~TCPSender();

		void Broadcast(const char * Data, unsigned int Len);


	private:
		std::string Port;
		std::string Server;

		char * Buffer = nullptr;


	// app access
	private:
		App* app;

	#ifdef __linux__
	// linux socket
	private:
    	int sockfd;
    	struct hostent *server;
    	struct sockaddr_in serveraddr;

	#elif _WIN32
	// Winsocket
	private:
		int iResult;
		struct addrinfo *result = NULL;
		struct addrinfo *ptr = NULL;
		struct addrinfo	hints;
		WSADATA wsaData;
		SOCKET ConnectSocket = INVALID_SOCKET;
	#else
	#endif


	private:
		bool bIsConnected;

		std::mutex mtx;

		void Connect();
		void ReleaseSocket();
		void Run();
	};
}

