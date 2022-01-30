#pragma once

#include <opencv2/aruco.hpp>
#include <opencv2/core.hpp>
#include"CameraCalibrationParams.h"

using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Graphics::Imaging;

namespace OpenCVRuntimeComponent
{
	namespace ArUcoTracking
	{
		public ref class ArUcoMarkerTracker sealed 
		{
		public:
			ArUcoMarkerTracker::ArUcoMarkerTracker(
				float markerSize,
				int numMarkers,
				int dictId,
				IVector<float3>^ customObjectPoints);

			IVector<DetectedArUcoMarker^>^ DetectArUcoMarkersInFrame(
				SoftwareBitmap^ softwareBitmap,
				OpenCVRuntimeComponent::CameraCalibrationParams^ cameraCalibrationParameters);

			DetectedArUcoBoard^ DetectBoardInFrame(
				SoftwareBitmap^ softwareBitmap,
				OpenCVRuntimeComponent::CameraCalibrationParams^ cameraCalibrationParameters);

		private:
			// Cached parameters for aruco marker detection
			float _markerSize;
			int _dictId;
			int _nMarkers;
			IVector<float3>^ _customObjectPoints;

			// Set the custom object points from Slicer for the ArUco board.
			void SetCustomObjPoints(std::vector<std::vector<cv::Point3f>> &objPoints, std::vector<int> &boardPoints);
			//std::pair<std::vector<std::vector<cv::Point3f>>, std::vector<int>> SetCustomObjPoints();
			std::vector<cv::Point3f> FillPositions(float markerLen, cv::Point3f cornerCoords);
		};

		cv::Mat FormatCameraMatrix(OpenCVRuntimeComponent::CameraCalibrationParams^ p);
		cv::Mat FormatDistortionCoefficientsMatrix(OpenCVRuntimeComponent::CameraCalibrationParams^ p);

	}
}


