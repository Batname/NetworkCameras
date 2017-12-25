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

BT::Camera::Camera(unsigned int CamIndex, std::string CamServer, std::string CamPort)
	: CamIndex(CamIndex)
	, CamServer(CamServer)
	, CamPort(CamPort)
	, k_fmt7Mode(MODE_0)
	, k_fmt7PixFmt(PIXEL_FORMAT_RGB8)
	, BytesPerColor(4)
	, bIsTCPThreadRunning(false)
{
	app = BT::App::GetApp();

	// Get camera guit
	error = busMgr.GetCameraFromIndex(CamIndex, &CamGuid);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Connect to a camera
	error = pCamera.Connect(&CamGuid);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Get the camera information
	error = pCamera.GetCameraInfo(&camInfo);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	if (app->bIsDebug)
	{
		PrintCameraInfo(&camInfo);
	}


	// Turn trigger mode off
	FlyCapture2::TriggerMode trigMode;
	trigMode.onOff = false;
	error = pCamera.SetTriggerMode(&trigMode);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Turn Timestamp on
	FlyCapture2::EmbeddedImageInfo imageInfo;
	imageInfo.timestamp.onOff = true;
	error = pCamera.SetEmbeddedImageInfo(&imageInfo);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Query for available Format 7 modes
	Format7Info fmt7Info;
	fmt7Info.mode = k_fmt7Mode;
	error = pCamera.GetFormat7Info(&fmt7Info, &bIsFormatSupported);
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
	error = pCamera.ValidateFormat7Settings(
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
	error = pCamera.SetFormat7Configuration(
		&fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Retrieve frame rate property
	frmRate.type = FRAME_RATE;
	error = pCamera.GetProperty(&frmRate);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Start capturing images
	error = pCamera.StartCapture();
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


	// Init Udp server thread
	tcpSenderThread = std::thread([=]
	{
		bIsTCPThreadRunning = true;
		tcpSender = new TCPSender(CamServer, CamPort);
	});
}

BT::Camera::~Camera()
{
	Disconnect();

	// release udp data
	if (bIsTCPThreadRunning)
	{
		bIsTCPThreadRunning = false;

		tcpSenderThread.join();
		delete tcpSender;
		tcpSender = nullptr;
	}
}

int BT::Camera::CaptureFrame()
{
	clock_t StartTime = clock();
	double Duration;

	// Retrieve an image
	error = pCamera.RetrieveBuffer(&rawImage);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
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
	}

	Duration = (clock() - StartTime) / (double)CLOCKS_PER_SEC;

	Tick(Duration);

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
		<< "BT::Camera::Tick " << " Delta:" << Delta
		<< " convertedImage DataSize in bytes: " << convertedImage.GetDataSize()
		<< " Server: " << CamServer << " Port " << CamPort
		<< " Data[100] example: 0x" << std::hex << (int)DataExample;
	//BT::Print(Log.str().c_str());

	// Send image throuth udp package
	if (tcpSender != nullptr)
	{
		tcpSender->Broadcast((char*)convertedImage.GetData(), convertedImage.GetDataSize());
	}

	return 0;
}

int BT::Camera::Disconnect()
{
	// Stop capturing images
	error = pCamera.StopCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	// Disconnect the camera
	error = pCamera.Disconnect();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		app->ErrorExit(-1);
	}

	return 0;
}
