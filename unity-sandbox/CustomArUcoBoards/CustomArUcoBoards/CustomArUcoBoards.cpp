// CustomArUcoBoards.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include <opencv2/aruco.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/calib3d/calib3d.hpp>

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
std::vector<cv::Point3f> FillPositions(
	int markerId,
	float markerLen,
	cv::Point3f cornerCoords)
{
	// Create 3 point vector of 4 corner locations
	std::vector<cv::Point3f> corners(4);

	//// Assuming we select center of marker as registration point
	//float cornerShift = markerLen / 2.0f;
	//corners[0] = cornerCoords + cv::Point3f(-cornerShift, cornerShift, 0);
	//corners[1] = cornerCoords + cv::Point3f(cornerShift, cornerShift, 0);
	//corners[2] = cornerCoords + cv::Point3f(cornerShift, -cornerShift, 0);
	//corners[3] = cornerCoords + cv::Point3f(-cornerShift, -cornerShift, 0);

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

// Fill object points structure with corner positions 
// in the board reference system. Corners are stored 
// in standard clockwise order starting with the top left. 
std::pair<std::vector<std::vector<cv::Point3f>>, std::vector<int>> SetCustomObjPoints(
	int nMarkers,
	float markerLength)
{
	std::vector<int> markerIds;
	std::vector<std::vector<cv::Point3f>> objPoints;

	// Fill board ids vector with 
	// marker ids from custom dictionary
	// Assuming we start at index zero
	for (int i = 0; i < nMarkers; i++)
	{
		markerIds.push_back(i);
		std::cout << "Added marker id: " + std::to_string(i) << std::endl;
	}

	/*
	Centered Markup
	9.56385,8.93296
	-5.74237,8.93345
	-5.68413,-6.27982
	9.52103,-6.31107
	*/

	const float H = 279.4;
	const float W = 215.9;

	// y
	// ^
	// |
	// |
	//  _______> x

	// Scale points for meters in Unity
	// Centered in Slicer
	// Multiply components by 10, multiply X values by -1
	std::vector<cv::Point3f> markerLocations;
	markerLocations.push_back(cv::Point3f(-95.6385, 89.3296, 0) / 1000.0); // convert from mm to m for unity
	markerLocations.push_back(cv::Point3f(57.4237, 89.3345, 0) / 1000.0);
	markerLocations.push_back(cv::Point3f(56.8413, -62.7982, 0) / 1000.0);
	markerLocations.push_back(cv::Point3f(-95.2103, -63.1107, 0) / 1000.0);

	// Call FillPositions to calculate Board object points.
	for (int i = 0; i < nMarkers; i++)
	{
		std::vector<cv::Point3f> corners;
		corners = FillPositions(
			markerIds[i],
			markerLength,
			markerLocations[i]);

		// Push corners to object points vector.
		objPoints.push_back(corners);
		std::cout << "Added corner: " +
			std::to_string(i)
			<< std::endl <<
			std::to_string(corners[0].x) + " , " +
			std::to_string(corners[0].y) + " , " +
			std::to_string(corners[0].z) + " , "
			<< std::endl <<
			std::to_string(corners[1].x) + " , " +
			std::to_string(corners[1].y) + " , " +
			std::to_string(corners[1].z) + " , "
			<< std::endl <<
			std::to_string(corners[2].x) + " , " +
			std::to_string(corners[2].y) + " , " +
			std::to_string(corners[2].z) + " , "
			<< std::endl <<
			std::to_string(corners[3].x) + " , " +
			std::to_string(corners[3].y) + " , " +
			std::to_string(corners[3].z) << std::endl;
	}

	//std::cout << 
	//	std::to_string(objPoints[0][0].x) + 
	//	std::to_string(objPoints[0][0].y) + 
	//	std::to_string(objPoints[0][0].z) << std::endl;

	std::pair<std::vector<std::vector<cv::Point3f>>, std::vector<int>> returnVals;
	returnVals.first = objPoints;
	returnVals.second = markerIds;

	return returnVals;
}

void DrawMarkers(
	int nMarkers,
	int startId,
	int nPixels,
	cv::Ptr<cv::aruco::Dictionary> customDict)
{
	cv::Mat marker_image;
	bool is_view_marker = false;

	for (int i = 0; i < nMarkers; i++)
	{
		cv::aruco::drawMarker(customDict,
			startId, nPixels, marker_image);

		while (!is_view_marker)
		{
			// Show the current marker image.
			imshow(std::to_string(startId), marker_image);

			// 1 ms wait key. Exit when escape is pressed.
			char c = cv::waitKey(1);
			if (c == 27)
			{
				is_view_marker = true;

				// Save the created marker.
				cv::String filename = "customMarker_" + std::to_string(startId) + ".png";
				cv::imwrite(filename, marker_image);
				std::cout << "Custom marker image " + std::to_string(startId) + " saved successfully." << std::endl;
			}
		}

		// Loop.
		startId += 1;
		is_view_marker = false;
	}
}

// Fill camera intrinsics information from calibration
// Calibrates webcam
cv::Mat SetCameraIntrinsics()
{
	cv::Mat cIntr = cv::Mat::zeros(cv::Size(3, 3), CV_32F);

	// Currently populating by hand. TODO: automate calibration procedure.
	// TODO: try out the ArUco chessboard calibration.
	// [[658.9556826920646, 0.0, 324.9187535049604], [0.0, 656.5978474059713, 196.8665186210343], [0.0, 0.0, 1.0]]
	// Populate the matrix.
	cIntr.at<float>(0, 0) = 658.9556826920646;
	cIntr.at<float>(0, 1) = 0.0;
	cIntr.at<float>(0, 2) = 324.9187535049604;

	cIntr.at<float>(1, 0) = 0.0;
	cIntr.at<float>(1, 1) = 656.5978474059713;
	cIntr.at<float>(1, 2) = 196.8665186210343;

	cIntr.at<float>(2, 0) = 0.0;
	cIntr.at<float>(2, 1) = 0.0;
	cIntr.at<float>(2, 2) = 1.0;

	return cIntr;
}

cv::Mat SetDistortionParams()
{
	cv::Mat dCoeff = cv::Mat::zeros(cv::Size(5, 1), CV_32F);

	// [[-0.0433980985969365, 0.010627348064559127, -0.011219846365467556, 0.018566783062551585, 0.11905086405769405]]
	dCoeff.at<float>(0, 0) = -0.0433980985969365;
	dCoeff.at<float>(0, 1) = 0.010627348064559127;
	dCoeff.at<float>(0, 2) = -0.011219846365467556;
	dCoeff.at<float>(0, 3) = 0.018566783062551585;
	dCoeff.at<float>(0, 4) = 0.11905086405769405;

	return dCoeff;
}

int main()
{
	// Not using custom markers, but using a custom configuration of them
	// as in the data/GroundTruths folder
	bool isCustomMarkers = false;
	bool isCustomBoard = true;

	// https://docs.opencv.org/4.0.1/db/da9/tutorial_aruco_board_detection.html
	cv::Mat cameraIntrinsics = cv::Mat::zeros(cv::Size(3, 3), CV_32F);
	cv::Mat distortionCoeffs = cv::Mat::zeros(cv::Size(5, 1), CV_32F);
	std::vector<std::vector<cv::Point3f>> objPoints;
	cv::Ptr<cv::aruco::Dictionary> customDict;
	cv::Ptr<cv::aruco::Dictionary> dict;
	std::vector<int> boardIds;

	int nMarkers = 4;
	int nBits = 4;

	// 72 DPI = 2.8346 pixels/mm
	// 300 DPI = 11.811024 pixels/mm
	// Want 40 mm markers * 11.811024 pixels / mm = 472.44096
	int nPixels = 472;
	float markerLength = 0.04;  // 40 mm
	int startId = 0;

	// Using the 6X5_250 dictionary for ArUco markers
	if (!isCustomMarkers)
	{
		dict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
		std::cout << "Generated ArUco dictionary" << std::endl;
	}
	if (isCustomMarkers)
	{
		// Custom dictionary
		customDict = cv::aruco::generateCustomDictionary(
			nMarkers, nBits);
		std::cout << "Generated custom ArUco dictionary" << std::endl;

	}

	// Draw the custom markers and save
	//DrawMarkers(nMarkers, 0, nPixels, customDict);
	//DrawMarkers(nMarkers, 0, nPixels, dict);
	cv::Ptr<cv::aruco::Board> customBoard;
	cv::Ptr<cv::aruco::Board> board;

	if (!isCustomBoard)
	{
		// 5 x 6 markers, 0.03 m size, 0.01 m spacing
		// 4 cm = 1.5748 in
		int nMarkersX = 5;
		int nMarkersY = 6;
		float markerSize = 30 / 1000.0f;
		float markerSpacing = 10 / 1000.0f;

		if (!isCustomMarkers)
		{
			board =
				cv::aruco::GridBoard::create(
					nMarkersX, nMarkersY,
					markerSize, markerSpacing,
					dict);

			DrawMarkers(
				nMarkers,
				startId,
				nPixels,
				dict);

			std::cout << "Generated ArUco board" << std::endl;

		}

		if (isCustomMarkers)
		{
			board =
				cv::aruco::GridBoard::create(
					nMarkersX, nMarkersY,
					markerSize, markerSpacing,
					customDict);

			DrawMarkers(
				nMarkers,
				startId,
				nPixels,
				customDict);

			std::cout << "Generated ArUco board with custom markers" << std::endl;

		}
		// Draw the grid board (US letter) (72 DPI)
		// Scaled sheet created in Gimp with 612 x 792 pixels (72 DPI)
		// 72 DPI * 1.5748 in * 5
		// 72 DPI * 1.5748 in * 6
		// Then paste into sheet in Gimp which is also
		// scaled to 72 DPI
		// 567 / 72 dpi = 7.875 in = 20 cm
		// 680 / 72 dpi = 9.444 in = 24 cm
		//cv::Mat grid_board_image;
		//gridBoard->draw(cv::Size(567, 680), grid_board_image, 10, 1);
	}
	if (isCustomBoard)
	{
		// Set custom marker locations from Slicer
		std::pair<
			std::vector<std::vector<cv::Point3f>>,
			std::vector<int>>
			returnVals = SetCustomObjPoints(
				nMarkers,
				markerLength);

		objPoints = returnVals.first;
		boardIds = returnVals.second;

		if (!isCustomMarkers)
		{
			customBoard = cv::aruco::Board::create(
				objPoints,
				dict,
				boardIds);

			DrawMarkers(
				nMarkers,
				startId,
				nPixels,
				dict);

			std::cout << "Generated custom ArUco board with dict" << std::endl;

		}
		if (isCustomMarkers)
		{

			customBoard = cv::aruco::Board::create(
				objPoints,
				customDict,
				boardIds);

			DrawMarkers(
				nMarkers,
				startId,
				nPixels,
				customDict);
			std::cout << "Generated custom ArUco board with custom markers" << std::endl;

		}
	}

	// Create the camera intrinsic and 
	// distortion parameters matrices
	cameraIntrinsics = SetCameraIntrinsics();
	distortionCoeffs = SetDistortionParams();

	// Get input video frames
	cv::VideoCapture inputVideo;
	inputVideo.open(0);

	while (inputVideo.grab())
	{
		cv::Mat image, imageCopy;
		inputVideo.retrieve(image);
		image.copyTo(imageCopy);

		std::vector<int> ids;
		std::vector<std::vector<cv::Point2f>> corners;
		// Estimate pose of the custom board
		cv::Vec3d rvec, tvec;

		if (!isCustomMarkers)
		{
			cv::aruco::detectMarkers(
				image,
				dict,
				corners,
				ids);
			std::cout << "detected " +
				std::to_string(ids.size()) +
				" markers in frame" << std::endl;
		}
		if (isCustomMarkers)
		{
			cv::aruco::detectMarkers(
				image,
				customDict,
				corners,
				ids);
			std::cout << "detected " +
				std::to_string(ids.size()) +
				" custom markers in frame" << std::endl;
		}

		// If we detect a marker then enter
		if (ids.size() > 1)
		{
			// Draw markers
			cv::aruco::drawDetectedMarkers(
				imageCopy,
				corners,
				ids);


			if (!isCustomBoard)
			{
				int valid = cv::aruco::estimatePoseBoard(
					corners, ids,
					board,
					cameraIntrinsics, distortionCoeffs,
					rvec, tvec);
				// If one board marker detected, draw the axis
				if (valid > 0)
				{
					cv::aruco::drawAxis(
						imageCopy,
						cameraIntrinsics, distortionCoeffs,
						rvec, tvec, 0.1);
				}
			}

			//std::cout << std::to_string(corners.size()) << std::endl;
			//std::cout << std::to_string(ids.size()) << std::endl;

			if (isCustomBoard)
			{
				int valid = cv::aruco::estimatePoseBoard(
					corners, ids,
					customBoard,
					cameraIntrinsics, distortionCoeffs,
					rvec, tvec);
				// If one board marker detected, draw the axis
				if (valid > 0)
				{
					cv::aruco::drawAxis(
						imageCopy,
						cameraIntrinsics, distortionCoeffs,
						rvec, tvec, 0.1);
				}
			}

		}

		// Show the output
		cv::imshow("out", imageCopy);
		char key = (char)cv::waitKey(1);
		if (key == 27)
			break;
	}
}
