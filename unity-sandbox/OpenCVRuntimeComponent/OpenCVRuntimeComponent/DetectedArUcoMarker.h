#pragma once

using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;

namespace OpenCVRuntimeComponent
{
	namespace ArUcoTracking
	{
		public ref class DetectedArUcoMarker sealed
		{
		public:
			DetectedArUcoMarker(
				_In_ int id,
				_In_ float3 position,
				_In_ float3 rotation);
		
			property int Id;
			property float3 Position;
			property float3 Rotation;
		};
	}
}



