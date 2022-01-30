#include "pch.h"
#include "ArUcoMarkerTracker.h"
#include "DetectedArUcoMarker.h"
#include <iostream>
#include "CvUtils.h"
#include <Trace.h>


namespace OpenCVRuntimeComponent
{
	namespace ArUcoTracking
	{
		/// <summary>
		/// Constructor for aruco marker tracking class.
		/// </summary>
		/// <param name="markerSize"></param>
		/// <param name="numMarkers"></param>
		/// <param name="dictId"></param>
		/// <param name="unitySpatialCoodinateSystem"></param>
		/// <param name="customObjectPoints"></param>
		ArUcoMarkerTracker::ArUcoMarkerTracker(
			float markerSize,
			int numMarkers,
			int dictId,
			Windows::Foundation::Collections::IVector<Windows::Foundation::Numerics::float3>^ customObjectPoints)
		{
			// Cache initial input parameters and custom aruco board configuration
			_markerSize = markerSize;
			_nMarkers = numMarkers;
			_dictId = dictId;
			_customObjectPoints = customObjectPoints;
		}

		/// <summary>
		/// Detect aruco markers in incoming frame using camera calib params to 
		/// return the position and rotation vector of detected markers
		/// </summary>
		/// <param name="softwareBitmap"></param>
		/// <param name="cameraCalibrationParameters"></param>
		/// <returns></returns>
		IVector<DetectedArUcoMarker^>^ ArUcoMarkerTracker::DetectArUcoMarkersInFrame(
			Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap,
			OpenCVRuntimeComponent::CameraCalibrationParams^ cameraCalibrationParameters)
		{
			// Clear the prior interface vector containing
			// detected aruco markers
			Windows::Foundation::Collections::IVector<ArUcoTracking::DetectedArUcoMarker^>^ detectedMarkers
				= ref new Platform::Collections::Vector<ArUcoTracking::DetectedArUcoMarker^>();

			// If null sensor frame, return zero detections
			if (softwareBitmap == nullptr)
			{
				DetectedArUcoMarker^ zeroMarker = ref new DetectedArUcoMarker(
					0,
					Windows::Foundation::Numerics::float3::zero(),
					Windows::Foundation::Numerics::float3::zero());
				detectedMarkers->Append(zeroMarker);
				return detectedMarkers;
			}

			// https://docs.opencv.org/4.1.1/d5/dae/tutorial_aruco_detection.html
			cv::Mat wrappedMat;
			std::vector<std::vector<cv::Point2f>> markers, rejectedCandidates;
			std::vector<int32_t> markerIds;

			// Create the aruco dictionary from id
			cv::Ptr<cv::aruco::Dictionary> dictionary =
				cv::aruco::getPredefinedDictionary(_dictId);

			// Create detector parameters
			cv::Ptr<cv::aruco::DetectorParameters> detectorParams
				= cv::aruco::DetectorParameters::create();

			// Use wrapper method to get cv::Mat from sensor frame
			// Can I directly stream gray frames from pv camera?
			OpenCVRuntimeComponent::ConversionUtils::WrapHoloLensSoftwareBitmapWithCvMat(softwareBitmap, wrappedMat);

			// Convert cv::Mat to grayscale for detection
			cv::Mat grayMat;
			cv::cvtColor(wrappedMat, grayMat, cv::COLOR_BGRA2GRAY);

			// Detect markers
			cv::aruco::detectMarkers(
				grayMat,
				dictionary,
				markers,
				markerIds,
				detectorParams,
				rejectedCandidates);

			dbg::trace(
				L"ArUcoMarkerTracker::DetectArUcoMarkersInFrame: %i markers found",
				markerIds.size());

			// If we have detected markers, compute the transform
			// to relate WinRT (right-handed row-vector) and Unity
			// (left-handed column-vector) representations for transforms
			// WinRT transfrom -> Unity transform by transpose and flip z values
			if (!markerIds.empty())
			{
				// Set camera intrinsic parameters for aruco based pose estimation
				cv::Mat cameraMatrix = FormatCameraMatrix(cameraCalibrationParameters);

				// Set distortion matrix for aruco based pose estimation
				cv::Mat distortionCoefficientsMatrix = FormatDistortionCoefficientsMatrix(cameraCalibrationParameters);

				// Vectors for pose (translation and rotation) estimation
				std::vector<cv::Vec3d> rVecs;
				std::vector<cv::Vec3d> tVecs;

				// Estimate pose of single markers
				cv::aruco::estimatePoseSingleMarkers(
					markers,
					_markerSize,
					cameraMatrix,
					distortionCoefficientsMatrix,
					rVecs,
					tVecs);

				// Iterate across the detected marker ids and cache information of 
				// pose of each marker as well as marker id
				for (size_t i = 0; i < markerIds.size(); i++)
				{
					//cv::Mat rMat;
					//cv::Rodrigues(rVecs[i], rMat);

					// Create marker WinRT marker class instance with current
					// detected marker parameters and view to unity transform
					DetectedArUcoMarker^ marker = ref new DetectedArUcoMarker(
						markerIds[i],
						Windows::Foundation::Numerics::float3((float)tVecs[i][0], (float)tVecs[i][1], (float)tVecs[i][2]),
						Windows::Foundation::Numerics::float3((float)rVecs[i][0], (float)rVecs[i][1], (float)rVecs[i][2]));

			// Add the marker to interface vector of markers
					detectedMarkers->Append(marker);
				}
			}

			return detectedMarkers;
		}

		/// Detect the ArUco board in frame given the sensor frame
		/// and marker dictionary. Hard coding in the layout of the 
		/// ArUco board for simplicity.
		DetectedArUcoBoard^ ArUcoMarkerTracker::DetectBoardInFrame(
			Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap,
			OpenCVRuntimeComponent::CameraCalibrationParams^ cameraCalibrationParameters)
		{
			auto detectedBoard = ref new DetectedArUcoBoard(
				Windows::Foundation::Numerics::float3::zero(),
				Windows::Foundation::Numerics::float3::zero(),
				false); // no board detected

			if (softwareBitmap == nullptr)
			{
				return detectedBoard;
			}

			// https://docs.opencv.org/4.1.1/d5/dae/tutorial_aruco_detection.html
			cv::Mat wrappedMat;
			std::vector<int32_t> markerIds;
			std::vector<std::vector<cv::Point2f>> markers, rejectedCandidates;
			std::vector<std::vector<cv::Point3f>> objPoints;
			std::vector<int32_t> boardIds;
			cv::Ptr<cv::aruco::Board> customBoard;

			// Set the custom object points for the board, force these
			// parameters
			SetCustomObjPoints(objPoints, boardIds);
			//std::pair<std::vector<std::vector<cv::Point3f>>, std::vector<int>> returnVals = SetCustomObjPoints();
			dbg::trace(L"Custom object points set.");

			//objPoints = returnVals.first;
			//boardIds = returnVals.second;

			// Create the aruco dictionary from id
			cv::Ptr<cv::aruco::Dictionary> dictionary =
				cv::aruco::getPredefinedDictionary(_dictId);

			// Create the custom board
			customBoard = cv::aruco::Board::create(
				objPoints,
				dictionary,
				boardIds);
			dbg::trace(L"Created aruco custom board object.");

			// Create detector parameters
			cv::Ptr<cv::aruco::DetectorParameters> detectorParams
				= cv::aruco::DetectorParameters::create();

			// Use wrapper method to get cv::Mat from sensor frame
			// Can I directly stream gray frames from pv camera?
			OpenCVRuntimeComponent::ConversionUtils::WrapHoloLensSoftwareBitmapWithCvMat(softwareBitmap, wrappedMat);

			// Convert cv::Mat to grayscale for detection
			cv::Mat grayMat;
			cv::cvtColor(wrappedMat, grayMat, cv::COLOR_BGRA2GRAY);

			// Detect markers
			cv::aruco::detectMarkers(
				grayMat,
				dictionary,
				markers,
				markerIds,
				detectorParams,
				rejectedCandidates);

			dbg::trace(
				L"ArUcoMarkerTracker::DetectBoardInFrame: %i markers found",
				markerIds.size());

			// If we have detected markers, compute the transform
			// to relate WinRT (right-handed row-vector) and Unity
			// (left-handed column-vector) representations for transforms
			// WinRT transfrom -> Unity transform by transpose and flip z values
			if (markerIds.size() > 1)
			{
				// Set camera intrinsic parameters for aruco based pose estimation
				cv::Mat cameraMatrix = FormatCameraMatrix(cameraCalibrationParameters);

				// Set distortion matrix for aruco based pose estimation
				cv::Mat distortionCoefficientsMatrix = FormatDistortionCoefficientsMatrix(cameraCalibrationParameters);

				// Vectors for pose (translation and rotation) estimation
				cv::Vec3d rVecs;
				cv::Vec3d tVecs;

				// Estimate pose of the custom board
				int valid = cv::aruco::estimatePoseBoard(
					markers, markerIds,
					customBoard,
					cameraMatrix,
					distortionCoefficientsMatrix,
					rVecs, tVecs);

				//cv::Mat rMat;
				//cv::Rodrigues(rVecs, rMat);

				// If one board marker detected
				if (valid > 0)
				{
					dbg::trace(
						L"ArUcoMarkerTracker::DetectBoardInFrame: detected an ArUco board object.");

					// Create marker WinRT marker class instance with current
					// detected board parameters and view to unity transform
					DetectedArUcoBoard^ board = ref new DetectedArUcoBoard(
						Windows::Foundation::Numerics::float3((float)tVecs[0], (float)tVecs[1], (float)tVecs[2]),
						Windows::Foundation::Numerics::float3((float)rVecs[0], (float)rVecs[1], (float)rVecs[2]),
						true); // board detected

					// Add the marker to interface vector of markers
					detectedBoard = board;
				}
			}

			return detectedBoard;
		}

		// Fill object points structure with corner positions 
		// in the board reference system. Corners are stored 
		// in standard clockwise order starting with the top left. 
		void ArUcoMarkerTracker::SetCustomObjPoints(std::vector<std::vector<cv::Point3f>>& objPoints, std::vector<int>& markerIds)
			//std::pair<std::vector<std::vector<cv::Point3f>>, std::vector<int>> ArUcoMarkerTracker::SetCustomObjPoints()
		{
			//std::vector<int> markerIds;
			//std::vector<std::vector<cv::Point3f>> objPoints;

			// Fill board ids vector with marker ids from custom dictionary
			// Assuming we start at index zero
			for (int i = 0; i < _nMarkers; i++)
			{
				markerIds.push_back(i);
				dbg::trace(L"Added marker id: %i", i);
			}

			std::vector<cv::Point3f> markerLocations;

			// Iterate across the cached vector of input points and fill a vector
			for (Windows::Foundation::Collections::IIterator<Windows::Foundation::Numerics::float3>^ point
				= _customObjectPoints->First(); point->HasCurrent; point->MoveNext())
			{
				markerLocations.push_back(cv::Point3f(point->Current.x, point->Current.y, point->Current.z));
			}

			// Call FillPositions to calculate Board object points.
			for (int i = 0; i < _nMarkers; i++)
			{
				std::vector<cv::Point3f> corners;
				corners = FillPositions(
					_markerSize,
					markerLocations[i]);

				// Push corners to object points vector.
				objPoints.push_back(corners);
				dbg::trace(L"Added corner: %f %f %f; %f %f %f; %f %f %f; %f %f %f",
					corners[0].x, corners[0].y, corners[0].z,
					corners[1].x, corners[1].y, corners[1].z,
					corners[2].x, corners[2].y, corners[2].z,
					corners[3].x, corners[3].y, corners[3].z);
			}
		}

		// Calculate positions for ArUco markers on board.
		// Assuming markers are all in same orientation, 
		// otherwise require 2 points to set corner positions.
		//	0______1
		//	|      |
		//	|  in  | 
		//	|______|
		//  3      2
		// corner_coordinates are input.
		// https://docs.google.com/document/d/1QU9KoBtjSM2kF6ITOjQ76xqL7H0TEtXriJX5kwi9Kgc/edit
		std::vector<cv::Point3f> ArUcoMarkerTracker::FillPositions(float markerLen, cv::Point3f cornerCoords)
		{
			// Create 3 point vector of 4 corner locations
			std::vector<cv::Point3f> corners(4);

			// y
			// ^
			// |
			// |
			//  _______> x
			float s = markerLen;
			// Assuming we select top left corner of marker as fiducial point
			corners[0] = cv::Point3f(
				cornerCoords.x,
				cornerCoords.y,
				cornerCoords.z);
			corners[1] = cv::Point3f(
				cornerCoords.x + s,
				cornerCoords.y,
				cornerCoords.z);
			corners[2] = cv::Point3f(
				cornerCoords.x + s,
				cornerCoords.y - s,
				cornerCoords.z);
			corners[3] = cv::Point3f(
				cornerCoords.x,
				cornerCoords.y - s,
				cornerCoords.z);

			return corners;
		}

		cv::Mat FormatCameraMatrix(OpenCVRuntimeComponent::CameraCalibrationParams^ p)
		{
			cv::Mat cM(3, 3, CV_64F, cv::Scalar(0));
			cM.at<double>(0, 0) = p->FocalLength.x;
			cM.at<double>(0, 2) = p->PrincipalPoint.x;
			cM.at<double>(1, 1) = p->FocalLength.y;
			cM.at<double>(1, 2) = p->PrincipalPoint.y;
			cM.at<double>(2, 2) = 1.0;

			return cM;
		}

		cv::Mat FormatDistortionCoefficientsMatrix(OpenCVRuntimeComponent::CameraCalibrationParams^ p)
		{
			cv::Mat dCM(1, 5, CV_64F);
			dCM.at<double>(0, 0) = p->RadialDistortion.x;
			dCM.at<double>(0, 1) = p->RadialDistortion.y;
			dCM.at<double>(0, 2) = p->TangentialDistortion.x;
			dCM.at<double>(0, 3) = p->TangentialDistortion.y;
			dCM.at<double>(0, 4) = p->RadialDistortion.z;

			return dCM;
		}
	}


}
