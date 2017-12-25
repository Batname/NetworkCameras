#pragma once

#include "FlyCapture2.h"

#include <mutex>
#include <thread>
#include <chrono>

namespace BT
{
	class App;
	class TCPSender;

	class Camera
	{
	public:
		Camera(std::string CamServer, std::string CamPort);
		virtual ~Camera();

	public:
		virtual int Run();
		virtual int End();
	private:
		const FlyCapture2::Mode k_fmt7Mode;
		const FlyCapture2::PixelFormat k_fmt7PixFmt;
		FlyCapture2::Format7Info fmt7Info;
		FlyCapture2::Format7ImageSettings fmt7ImageSettings;
		FlyCapture2::Format7PacketInfo fmt7PacketInfo;


		bool bIsFormatSupported;
		bool bIsFormatSettingsValid;
		std::string CamPort;
		std::string CamServer;

		unsigned int BytesPerColor;

		// access to the app
		App* app;


		void PrintCameraInfo(FlyCapture2::CameraInfo *pCamInfo);
		void PrintFormat7Capabilities(FlyCapture2::Format7Info fmt7Info);

	// Frame Data
	private:
		FlyCapture2::PixelFormat pixFormat;
		FlyCapture2::Property frmRate;
		FlyCapture2::Image rawImage;
		FlyCapture2::Image convertedImage;

		unsigned int rows, cols, stride;

	protected:
		FlyCapture2::Error error;
		FlyCapture2::CameraInfo camInfo;
		FlyCapture2::Camera Cam;

		virtual int Tick(double Delta);

	// UDP server communication
	protected:
		TCPSender* tcpSender;
		std::thread tcpSenderThread;

		std::mutex mtx;
	};
}