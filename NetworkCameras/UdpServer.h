#pragma once

#include<stdio.h>
#include<winsock2.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

// Implement this examples
// http://www.binarytides.com/udp-socket-programming-in-winsock/
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms737889(v=vs.85).aspx

class UdpServer
{
public:
	UdpServer();
	~UdpServer();
};

