#pragma once

using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;

namespace OpenCVRuntimeComponent
{
    public ref class CameraCalibrationParams sealed
    {
    public:
        CameraCalibrationParams(
            _In_ float2 focalLength,
            _In_ float2 principalPoint,
            _In_ float3 radialDistortion,
            _In_ float2 tangentialDistortion,
            _In_ int imageWidth,
            _In_ int imageHeight);

        property float2 FocalLength;
        property float2 PrincipalPoint;
        property float3 RadialDistortion;
        property float2 TangentialDistortion;
        property int ImageWidth;
        property int ImageHeight;
    };
}

