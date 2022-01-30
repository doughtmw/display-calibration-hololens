#include "pch.h"
#include "CvUtils.h"
#include "Trace.h"
#include "BufferHelpers.h"
#include "ArUcoMarkerTracker.h"
#include <DetectedArUcoMarker.h>
#include <ArUcoMarkerTracker.h>

using namespace OpenCVRuntimeComponent;
using namespace Platform;

CvUtils::CvUtils(
	float markerSize,
	int numMarkers,
	int dictId,
	Windows::Foundation::Collections::IVector<Windows::Foundation::Numerics::float3>^ customObjectPoints)
{
	_arUcoMarkerTracker = ref new ArUcoTracking::ArUcoMarkerTracker(
		markerSize,
		numMarkers,
		dictId,
		customObjectPoints);

	_pointCorrespondences = ref new HMDCalibration::PointCorrespondences();
}

IVector<ArUcoTracking::DetectedArUcoMarker^>^ 
OpenCVRuntimeComponent::CvUtils::DetectMarkers(
	SoftwareBitmap^ softwareBitmap,
	CameraCalibrationParams^ cameraCalibrationParams)
{
	return _arUcoMarkerTracker->DetectArUcoMarkersInFrame(
		softwareBitmap, 
		cameraCalibrationParams);
}

ArUcoTracking::DetectedArUcoBoard^ 
OpenCVRuntimeComponent::CvUtils::DetectBoard(
	SoftwareBitmap^ softwareBitmap,
	CameraCalibrationParams^ cameraCalibrationParams)
{
	return _arUcoMarkerTracker->DetectBoardInFrame(
		softwareBitmap,
		cameraCalibrationParams);
}

float4x4 OpenCVRuntimeComponent::CvUtils::RigidTransform3D3D(
	IVector<float3>^ headRelativeCameraPoint3D, 
	IVector<float3>^ headRelativeMarkerPoint3D)
{
	return _pointCorrespondences->ComputeRigidTransform3D3D(
		headRelativeCameraPoint3D,
		headRelativeMarkerPoint3D);
}

#pragma region FrameConversionUtils
// Taken directly from the OpenCVHelpers in HoloLensForCV repo.
// https://github.com/microsoft/HoloLensForCV
void ConversionUtils::WrapHoloLensSoftwareBitmapWithCvMat(
	SoftwareBitmap^ softwareBitmap,
	cv::Mat& wrappedImage)
{
	// Confirm that the sensor frame is not null
	if (softwareBitmap != nullptr)
	{
		BitmapBuffer^ bitmapBuffer =
			softwareBitmap->LockBuffer(BitmapBufferAccessMode::Read);

		uint32_t pixelBufferDataLength = 0;

		uint8_t* pixelBufferData =
			Io::GetTypedPointerToMemoryBuffer<uint8_t>(
				bitmapBuffer->CreateReference(),
				pixelBufferDataLength);

		int32_t wrappedImageType;

		switch (softwareBitmap->BitmapPixelFormat)
		{
		case BitmapPixelFormat::Bgra8:
			wrappedImageType = CV_8UC4;
			dbg::trace(
				L"WrapHoloLensSensorFrameWithCvMat: CV_8UC4 pixel format");
			break;

		case BitmapPixelFormat::Gray16:
			wrappedImageType = CV_16UC1;
			dbg::trace(
				L"WrapHoloLensSensorFrameWithCvMat: CV_16UC1 pixel format");
			break;

		case BitmapPixelFormat::Gray8:
			wrappedImageType = CV_8UC1;
			dbg::trace(
				L"WrapHoloLensSensorFrameWithCvMat: CV_8UC1 pixel format");
			break;

		default:
			dbg::trace(
				L"WrapHoloLensSensorFrameWithCvMat: unrecognized softwareBitmap pixel format, falling back to CV_8UC1");

			wrappedImageType = CV_8UC1;
			break;
		}

		wrappedImage = cv::Mat(
			softwareBitmap->PixelHeight,
			softwareBitmap->PixelWidth,
			wrappedImageType,
			pixelBufferData);

	}

	// Otherwise return an empty sensor frame
	else
	{
		uint8_t* pixelBufferData = new uint8_t();

		wrappedImage = cv::Mat(
			0,
			0,
			CV_8UC4,
			pixelBufferData);

		dbg::trace(
			L"WrapHoloLensSensorFrameWithCvMat: frame was null, returning empty matrix of CV_8UC4 pixel format.");
	}
}

// Wrap OpenCV Mat of type CV_8UC1 with SensorFrame.
SoftwareBitmap^ ConversionUtils::WrapCvMatWithHoloLensSoftwareBitmap(
	cv::Mat& from)
{
	int32_t pixelHeight = from.rows;
	int32_t pixelWidth = from.cols;

	BitmapPixelFormat bitmapPixelFormat;

	switch (from.type())
	{
	case CV_8UC4:
		bitmapPixelFormat = BitmapPixelFormat::Bgra8;
		dbg::trace(
		L"WrapCvMatWithHoloLensSoftwareBitmap: Bgra8 pixel format");
		break;
	case CV_8UC1:
		bitmapPixelFormat = BitmapPixelFormat::Gray8;
		dbg::trace(
			L"WrapCvMatWithHoloLensSoftwareBitmap: Gray8 pixel format");
		break;
	default:
		bitmapPixelFormat = BitmapPixelFormat::Gray8;
		dbg::trace(
			L"WrapCvMatWithHoloLensSoftwareBitmap: Gray8 pixel format");
		break;
	}

	SoftwareBitmap^ bitmap = ref new SoftwareBitmap(
		bitmapPixelFormat,
		pixelWidth, pixelHeight,
		BitmapAlphaMode::Ignore);

	BitmapBuffer^ bitmapBuffer = bitmap->LockBuffer(BitmapBufferAccessMode::ReadWrite);

	auto reference = bitmapBuffer->CreateReference();
	unsigned char* dstPixels = GetPointerToPixelData(reference);
	memcpy(dstPixels, from.data, from.step.buf[1] * from.cols * from.rows);
	
	return bitmap;
}

// https://github.com/microsoft/Windows-universal-samples/blob/master/Samples/CameraOpenCV/shared/OpenCVBridge/OpenCVHelper.cpp
// https://stackoverflow.com/questions/34198259/winrt-c-win10-opencv-hsv-color-space-image-display-artifacts/34198580#34198580
// Get pointer to memory buffer reference. 
unsigned char* ConversionUtils::GetPointerToPixelData(Windows::Foundation::IMemoryBufferReference^ reference)
{
	Microsoft::WRL::ComPtr<Windows::Foundation::IMemoryBufferByteAccess> bufferByteAccess;

	reinterpret_cast<IInspectable*>(reference)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));

	unsigned char* pixels = nullptr;
	unsigned int capacity = 0;
	bufferByteAccess->GetBuffer(&pixels, &capacity);

	return pixels;
}

#pragma endregion
