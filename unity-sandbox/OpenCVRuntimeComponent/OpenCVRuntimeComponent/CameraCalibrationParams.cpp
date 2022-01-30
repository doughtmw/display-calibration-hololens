#include "pch.h"
#include "CameraCalibrationParams.h"

OpenCVRuntimeComponent::CameraCalibrationParams::CameraCalibrationParams(
    float2 focalLength, 
    float2 principalPoint, 
    float3 radialDistortion, 
    float2 tangentialDistortion, 
    int imageWidth, int imageHeight)
{
    FocalLength = focalLength;
    PrincipalPoint = principalPoint;
    RadialDistortion = radialDistortion;
    TangentialDistortion = tangentialDistortion;
    ImageWidth = imageWidth;
    ImageHeight = imageHeight;
}

