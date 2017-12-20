#include "Camera.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>
#include <iomanip>

#include "App.h"
#include "TCPSender.h"

using namespace std;

extern mutex MainMutex;

BT::Camera::Camera(int CamId, unsigned int CameraPort)
	: CamId(CamId)
	, CameraPort(CameraPort)
	, k_fmt7Mode(MODE_0)
	, k_fmt7PixFmt(PIXEL_FORMAT_RGB8)
	, BytesPerColor(4)
{
	app = BT::App::GetApp();

	// Connect to a camera
	error = Cam.Connect(&app->guid);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Get the camera information
	error = Cam.GetCameraInfo(&camInfo);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	if (app->bIsDebug)
	{
		PrintCameraInfo(&camInfo);
	}

	// Query for available Format 7 modes
	Format7Info fmt7Info;
	fmt7Info.mode = k_fmt7Mode;
	error = Cam.GetFormat7Info(&fmt7Info, &bIsFormatSupported);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	if (app->bIsDebug)
	{
		PrintFormat7Capabilities(fmt7Info);
	}

	if ((k_fmt7PixFmt & fmt7Info.pixelFormatBitField) == 0)
	{
		// Pixel format not supported!
		BT::Print("Pixel format is not supported");
		app->ErrorExit(-1);
	}

	// Set format settings
	fmt7ImageSettings.mode = k_fmt7Mode;
	fmt7ImageSettings.offsetX = 0;
	fmt7ImageSettings.offsetY = 0;
	fmt7ImageSettings.width = fmt7Info.maxWidth;
	fmt7ImageSettings.height = fmt7Info.maxHeight;
	fmt7ImageSettings.pixelFormat = k_fmt7PixFmt;

	// Validate the settings to make sure that they are valid
	error = Cam.ValidateFormat7Settings(
		&fmt7ImageSettings, &bIsFormatSettingsValid, &fmt7PacketInfo);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	if (!bIsFormatSettingsValid)
	{
		// Settings are not valid
		BT::Print("Format7 settings are not valid");

		app->ErrorExit(-1);
	}

	// Set the settings to the camera
	error = Cam.SetFormat7Configuration(
		&fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Retrieve frame rate property
	frmRate.type = FRAME_RATE;
	error = Cam.GetProperty(&frmRate);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}


	// Init Udp server
	tcpSender = new TCPSender("127.0.0.1", "8889");
}

BT::Camera::~Camera()
{
	End();

	// release udp data
	delete tcpSender;
	tcpSender = nullptr;
}

int BT::Camera::Run()
{
	// Start capturing images
	error = Cam.StartCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}


	// Print framerate
	{
		ostringstream FrameRate;
		FrameRate 
		<< "Frame rate is " << fixed << std::setprecision(2) << frmRate.absValue << " fps" << endl;

		BT::Print(FrameRate.str().c_str());
	}

	while (true)
	{
		clock_t StartTime = clock();
		double Duration;

		// Retrieve an image
		error = Cam.RetrieveBuffer(&rawImage);
		if (error != PGRERROR_OK)
		{
			PrintError(error);
			continue;
		}

		// Get the raw image dimensions
		PixelFormat pixFormat;
		unsigned int rows, cols, stride;
		rawImage.GetDimensions(&rows, &cols, &stride, &pixFormat);

		// Convert the raw image
		error = rawImage.Convert(PIXEL_FORMAT_BGRU, &convertedImage);
		if (error != PGRERROR_OK)
		{
			PrintError(error);
			continue;
		}

		Duration = (clock() - StartTime) / (double)CLOCKS_PER_SEC;

		// if we need more delay
		//this_thread::sleep_for(chrono::milliseconds(200));
		Tick(Duration);
	}

	return 1;
}

void BT::Camera::PrintCameraInfo(FlyCapture2::CameraInfo * pCamInfo)
{

	ostringstream CameraInfo;

	CameraInfo << "*** CAMERA INFORMATION ***" << endl
	<< "Serial number - " << pCamInfo->serialNumber << endl
	<< "Camera model - " << pCamInfo->modelName << endl
	<< "Camera vendor - " << pCamInfo->vendorName << endl
	<< "Sensor - " << pCamInfo->sensorInfo << endl
	<< "Resolution - " << pCamInfo->sensorResolution << endl
	<< "Firmware version - " << pCamInfo->firmwareVersion << endl
	<< "Firmware build time - " << pCamInfo->firmwareBuildTime;
	BT::Print(CameraInfo.str().c_str());
}

void BT::Camera::PrintFormat7Capabilities(FlyCapture2::Format7Info fmt7Info)
{
	ostringstream Format7Capabilities;

	Format7Capabilities 
	<< "Max image pixels: (" << fmt7Info.maxWidth << ", "
	<< fmt7Info.maxHeight << ")" << endl
	<< "Image Unit size: (" << fmt7Info.imageHStepSize << ", "
	<< fmt7Info.imageVStepSize << ")" << endl
	<< "Offset Unit size: (" << fmt7Info.offsetHStepSize << ", "
	<< fmt7Info.offsetVStepSize << ")" << endl
	<< "Pixel format bitfield: 0x" << fmt7Info.pixelFormatBitField << endl;

	BT::Print(Format7Capabilities.str().c_str());
}

int BT::Camera::Tick(double Delta)
{
	unsigned char DataExample = convertedImage.GetData()[100];


	std::stringstream Log;
	Log
		<< "BT::Camera::Tick " << "Id: " << CamId << " Delta:" << Delta
		<< " convertedImage DataSize in bytes: " << convertedImage.GetDataSize()
		<< " Data[100] example: 0x" << std::hex << (int)DataExample;
	//BT::Print(Log.str().c_str());

	// Send image throuth udp package
	if (tcpSender != nullptr)
	{
		tcpSender->Broadcast((char*)convertedImage.GetData(), convertedImage.GetDataSize());
	}

	return 0;
}

int BT::Camera::End()
{
	// Stop capturing images
	error = Cam.StopCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Disconnect the camera
	error = Cam.Disconnect();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	return 0;
}
