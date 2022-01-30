#pragma once

using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;

namespace OpenCVRuntimeComponent
{
	namespace ArUcoTracking
	{
		public ref class DetectedArUcoBoard sealed
		{
		public:
			DetectedArUcoBoard(
				_In_ float3 position,
				_In_ float3 rotation,
				_In_ bool isDetected);

			property float3 Position;
			property float3 Rotation;
			property bool IsDetected;
		};
	}

}
