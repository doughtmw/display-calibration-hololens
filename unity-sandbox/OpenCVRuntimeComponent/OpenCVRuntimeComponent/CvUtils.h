#pragma once
#include"CameraCalibrationParams.h"
#include "PointCorrespondences.h"

using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Graphics::Imaging;

namespace OpenCVRuntimeComponent
{
    public ref class CvUtils sealed
    {
    public:
        CvUtils(
            float markerSize,
            int numMarkers,
            int dictId,
            IVector<Windows::Foundation::Numerics::float3>^ customObjectPoints);

        IVector<ArUcoTracking::DetectedArUcoMarker^>^ DetectMarkers(
            SoftwareBitmap^ softwareBitmap,
            CameraCalibrationParams^ cameraCalibrationParams);
        
        ArUcoTracking::DetectedArUcoBoard^ DetectBoard(
            SoftwareBitmap^ softwareBitmap,
            CameraCalibrationParams^ cameraCalibrationParams);

        float4x4 RigidTransform3D3D(
            IVector<float3>^ headRelativeCameraPoint3D,
            IVector<float3>^ headRelativeMarkerPoint3D);

    private:
        ArUcoTracking::ArUcoMarkerTracker^ _arUcoMarkerTracker;
        HMDCalibration::PointCorrespondences^ _pointCorrespondences;
    };

    private class ConversionUtils
    {
    public:
        static void WrapHoloLensSoftwareBitmapWithCvMat(SoftwareBitmap^ softwareBitmap, cv::Mat& openCVImage);
        static SoftwareBitmap^ WrapCvMatWithHoloLensSoftwareBitmap(cv::Mat& from);
        static unsigned char* GetPointerToPixelData(Windows::Foundation::IMemoryBufferReference^ reference);
    };
}
