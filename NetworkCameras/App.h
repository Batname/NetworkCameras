#pragma once


#include <mutex>
#include <vector>

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
		//std::thread CamerasThread;


		std::vector<BT::Camera*> Cameras;
		std::vector<std::string> CamPorts = { "8889", "8890" };
		std::string CamsServer = "127.0.0.1";

	};
}


