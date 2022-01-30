#include "pch.h"
#include "PointCorrespondences.h"

using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;

using namespace OpenCVRuntimeComponent;

using PointSet = Eigen::Matrix<float, 3, Eigen::Dynamic>;

HMDCalibration::PointCorrespondences::PointCorrespondences(){}

/// <summary>
/// Ported/Adapted from: https://github.com/nghiaho12/rigid_transform_3D/blob/master/rigid_transform_3D.py
/// Based on article: http://nghiaho.com/?page_id=671
/// https://github.com/korejan/rigid_transform_3D_cpp/blob/master/rigid_transform_3D.cpp
/// </summary>
/// <param name="headRelativeCameraPoint3D"></param>
/// <param name="headRelativeMarkerPoint3D"></param>
/// <returns></returns>
float4x4 HMDCalibration::PointCorrespondences::ComputeRigidTransform3D3D(
	IVector<float3>^ headRelativeCameraPoint3D, 
	IVector<float3>^ headRelativeMarkerPoint3D)
{
	// Format the incoming vector3 into an eigen matrix
	const PointSet& A = FormatVector3ForEigen(headRelativeCameraPoint3D);
	const PointSet& B = FormatVector3ForEigen(headRelativeMarkerPoint3D);

	// Iterate across vector and debug point correspondences 
	for (int i = 0; i < (int)headRelativeCameraPoint3D->Size; i++)
	{
		dbg::trace(L"PointCorrespondences::HeadRelativeCameraPoint3D: %f, %f, %f \n",
			A(0, i), A(1, i), A(2, i));
	}

	for (int i = 0; i < (int)headRelativeMarkerPoint3D->Size; i++)
	{
		dbg::trace(L"PointCorrespondences::headRelativeMarkerPoint3D: %f, %f, %f \n",
			B(0, i), B(1, i), B(2, i));
	}

	assert(A.cols() == B.cols());

	// find mean column wise, size 3 x 1
	const Eigen::Vector3f centroid_A = A.rowwise().mean();
	const Eigen::Vector3f centroid_B = B.rowwise().mean();

	// subtract mean
	PointSet Am = A.colwise() - centroid_A;
	PointSet Bm = B.colwise() - centroid_B;

	PointSet H = Am * Bm.transpose();

	//
	//# sanity check
	//#if linalg.matrix_rank(H) < 3:
	//	#    raise ValueError("rank of H = {}, expecting 3".format(linalg.matrix_rank(H)))
	//

	// find rotation
	Eigen::JacobiSVD<Eigen::Matrix3Xf> svd = H.jacobiSvd(Eigen::DecompositionOptions::ComputeFullU | Eigen::DecompositionOptions::ComputeFullV);
	const Eigen::Matrix3f& U = svd.matrixU();
	Eigen::MatrixXf V = svd.matrixV();
	Eigen::Matrix3f R = V * U.transpose();

	// special reflection case
	if (R.determinant() < 0.0f)
	{
		V.col(2) *= -1.0f;
		R = V * U.transpose();
	}

	const Eigen::Vector3f t = -R * centroid_A + centroid_B;

	// Combine into a 4x4 transformation matrix and return
	//	R R R T
	//	R R R T
	//	R R R T
	//	0 0 0 1

	// Fill the transform to be sent to Unity
	float4x4 mpc2tmpfloat4x4 = float4x4(
		R(0, 0), R(0, 1), R(0, 2), t(0),
		R(1, 0), R(1, 1), R(1, 2), t(1),
		R(2, 0), R(2, 1), R(2, 2), t(2),
		0, 0, 0, 1);

	DebugFloat4x4(
		mpc2tmpfloat4x4,
		"PointCorrespondences::RigidTransform3D3D");

	return mpc2tmpfloat4x4;
}

Eigen::MatrixXf HMDCalibration::PointCorrespondences::FormatVector3ForEigen
(IVector<float3>^ v)
{
	// Allocate the matrix (3 x N)
	//Eigen::MatrixXf m(v->Size, 3);
	Eigen::MatrixXf m(3, v->Size);

	// Iterate across the incoming vector and fill the eigen matrix
	for (auto i = 0; i < (int)v->Size; i++)
	{
		m.col(i) << v->GetAt(i).x, v->GetAt(i).y, v->GetAt(i).z;
	/*	m(i, 0) = (float)v->GetAt(i).x;
		m(i, 1) = (float)v->GetAt(i).y;
		m(i, 2) = (float)v->GetAt(i).z;*/
	}

	return m;
}

void HMDCalibration::PointCorrespondences::DebugFloat4x4(float4x4 f, std::string s)
{
	dbg::trace(
		L"%s: %f, %f, %f, %f \n %f, %f, %f, %f \n %f, %f, %f, %f \n %f, %f, %f, %f \n",
		s.c_str(),
		f.m11, f.m12, f.m13, f.m14,
		f.m21, f.m22, f.m23, f.m24,
		f.m31, f.m32, f.m33, f.m34,
		f.m41, f.m42, f.m43, f.m44);
}
