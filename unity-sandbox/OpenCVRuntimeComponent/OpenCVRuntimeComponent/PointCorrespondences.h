#pragma once

using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;

namespace OpenCVRuntimeComponent
{
	namespace HMDCalibration
	{
		public ref class PointCorrespondences sealed
		{
		public:
			PointCorrespondences();

			// Requires the eigen package
			float4x4 ComputeRigidTransform3D3D(
				IVector<float3>^ headRelativeCameraPoint3D,
				IVector<float3>^ headRelativeMarkerPoint3D);

		private:
			Eigen::MatrixXf FormatVector3ForEigen(IVector<float3>^ v);
			void DebugFloat4x4(float4x4 f, std::string s);
		};

	}
}


