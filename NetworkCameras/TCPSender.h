#pragma once

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <chrono>
#include <mutex>


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


	// app access
	private:
		App* app;

	// Winsocket
	private:
		int iResult;
		struct addrinfo *result = NULL;
		struct addrinfo *ptr = NULL;
		struct addrinfo	hints;
		WSADATA wsaData;
		SOCKET ConnectSocket = INVALID_SOCKET;

		bool bIsConnected;

		std::mutex mtx;

		void Connect();
		void ReleaseSocket();
		void Run();
	};
}

