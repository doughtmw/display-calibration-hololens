#include "pch.h"
#include "DetectedArUcoBoard.h"

using namespace OpenCVRuntimeComponent;

ArUcoTracking::DetectedArUcoBoard::DetectedArUcoBoard(
	float3 position,
	float3 rotation,
	bool isDetected)
{
	// Set the position, rotation and cam to world transform
	// properties of current marker
	Position = position;
	Rotation = rotation;
	IsDetected = isDetected;
}
