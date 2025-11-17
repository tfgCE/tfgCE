#include "fbxUtils.h"

#ifdef USE_FBX
#include "..\..\core\math\math.h"

Matrix44 FBX::fbx_matrix_to_matrix_44(FbxAMatrix const & _matrix)
{
	double const * m = _matrix;

	Matrix44 result;

	result.m00 = (float)*m;		++m;
	result.m01 = (float)*m;		++m;
	result.m02 = (float)*m;		++m;
	result.m03 = (float)*m;		++m;

	result.m10 = (float)*m;		++m;
	result.m11 = (float)*m;		++m;
	result.m12 = (float)*m;		++m;
	result.m13 = (float)*m;		++m;

	result.m20 = (float)*m;		++m;
	result.m21 = (float)*m;		++m;
	result.m22 = (float)*m;		++m;
	result.m23 = (float)*m;		++m;

	result.m30 = (float)*m;		++m;
	result.m31 = (float)*m;		++m;
	result.m32 = (float)*m;		++m;
	result.m33 = (float)*m;		++m;

	if (Vector3::dot(Vector3::cross(result.get_x_axis(), result.get_y_axis()), result.get_z_axis()) < 0.0f)
	{
		// change to right handed system
		result.m20 = -result.m20;
		result.m21 = -result.m21;
		result.m22 = -result.m22;
		result.m23 = -result.m23;
	}

	return result;
}

Matrix44 FBX::fbx_matrix_to_matrix_44(FbxMatrix const & _matrix)
{
	double const * m = _matrix;

	Matrix44 result;

	result.m00 = (float)*m;		++m;
	result.m01 = (float)*m;		++m;
	result.m02 = (float)*m;		++m;
	result.m03 = (float)*m;		++m;

	result.m10 = (float)*m;		++m;
	result.m11 = (float)*m;		++m;
	result.m12 = (float)*m;		++m;
	result.m13 = (float)*m;		++m;

	result.m20 = (float)*m;		++m;
	result.m21 = (float)*m;		++m;
	result.m22 = (float)*m;		++m;
	result.m23 = (float)*m;		++m;

	result.m30 = (float)*m;		++m;
	result.m31 = (float)*m;		++m;
	result.m32 = (float)*m;		++m;
	result.m33 = (float)*m;		++m;

	if (Vector3::dot(Vector3::cross(result.get_x_axis(), result.get_y_axis()), result.get_z_axis()) < 0.0f)
	{
		// change to right handed system
		result.m20 = -result.m20;
		result.m21 = -result.m21;
		result.m22 = -result.m22;
		result.m23 = -result.m23;
	}

	return result;
}

#endif
