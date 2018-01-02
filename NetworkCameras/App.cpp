#include "App.h"
#include "Camera.h"

// Standart libs
#include <stdio.h>
#include <sstream>
#include <iostream>

std::mutex BT::App::MainMutex;
BT::App* BT::App::GlobalApp = nullptr;

using namespace FlyCapture2;
using namespace std;

int BT::Print(const char * Message)
{
	BT::App::MainMutex.lock();
	printf("[BT log] %s\n", Message);
	BT::App::MainMutex.unlock();

	return 0;
}

void BT::PrintBuildInfo()
{
	FC2Version fc2Version;
	Utilities::GetLibraryVersion(&fc2Version);

	ostringstream version;
	version << "FlyCapture2 library version: " << fc2Version.major << "."
		<< fc2Version.minor << "." << fc2Version.type << "."
		<< fc2Version.build;

	BT::Print(version.str().c_str());

	ostringstream timeStamp;
	timeStamp << "Application build date: " << __DATE__ << " " << __TIME__;
	BT::Print(timeStamp.str().c_str());
}

void BT::PrintError(Error error)
{
	BT::App::MainMutex.lock();
	error.PrintErrorTrace();
	BT::App::MainMutex.unlock();
}

BT::App::App(bool bIsDebug)
	: bIsDebug(bIsDebug)
{
	// declare app global pointer
	GlobalApp = this;


	if (bIsDebug)
	{
		BT::PrintBuildInfo();
	}

	// check if cameras connected
	error = busMgr.GetNumOfCameras(&numCameras);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		ErrorExit(-1);
	}

	// Print num of cameras
	{
		ostringstream CamNumMessage;
		CamNumMessage << "Number of cameras detected: " << numCameras;

		BT::Print(CamNumMessage.str().c_str());
	}

	// Exit if less than 2 cameras
	if (numCameras < 2)
	{
		BT::Print("Insufficient number of cameras... exiting");
		ErrorExit(-1);
	}
}

BT::App::~App()
{
	Release();

}

void BT::App::Run()
{
	for (unsigned int i = 0; i < numCameras; i++)
	{
		Cameras.push_back(new BT::Camera(i, CamsServer, CamPorts[i]));
	}

	// Run camera threads
	while (true)
	{
		for (auto Cam : Cameras)
		{
			Cam->CaptureFrame();
		}
	}
}

void BT::App::ErrorExit(int Code)
{
	// Free resources
	Release();

	// Exit
	exit(Code);
}

void BT::App::Release()
{
	for (auto Cam : Cameras)
	{
		delete Cam;
	}
}
