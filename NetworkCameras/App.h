#pragma once


#include <mutex>
#include <thread>
#include <chrono>

#include "FlyCapture2.h"

using namespace FlyCapture2;

namespace BT
{
	class Camera;

	int Print(const char* Message);

	void PrintBuildInfo();
	void PrintError(Error error);

	class App
	{
		friend class Camera;

	public:
		App(bool bIsDebug);
		virtual ~App();

		void Run();

		static std::mutex MainMutex;
		inline static App* GetApp() { return GlobalApp; };

		inline Error GetError() { return error; }
		void ErrorExit(int Code);

	private:
		Error error;
		BusManager busMgr;
		PGRGuid guid;
		unsigned int numCameras;

		bool bIsDebug;

		void Release();

	private:
		// Global app access
		static App* GlobalApp;

	// Camera threads
	private:
		std::thread Cam1Tread;
		std::thread Cam2Tread;

		BT::Camera* Camera1;
		BT::Camera* Camera2;

		std::string Cam1Port = "8889";
		std::string Cam2Port = "8890";
		std::string CamsServer = "127.0.0.1";


	};
}


