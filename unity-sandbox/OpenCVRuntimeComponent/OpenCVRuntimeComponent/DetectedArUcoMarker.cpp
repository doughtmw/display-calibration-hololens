#include "pch.h"
#include "DetectedArUcoMarker.h"

namespace OpenCVRuntimeComponent
{
	namespace ArUcoTracking
	{
		DetectedArUcoMarker::DetectedArUcoMarker(
			_In_ int id,
			_In_ float3 position,
			_In_ float3 rotation)
		{
			// Set the position, rotation and cam to world transform
			// properties of current marker
			Id = id;
			Position = position;
			Rotation = rotation;
		}
	}
}
