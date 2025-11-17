#pragma once

#include "fbxLib.h"

#ifdef USE_FBX
struct Matrix44;

namespace FBX
{
	Matrix44 fbx_matrix_to_matrix_44(FbxAMatrix const & _matrix);
	Matrix44 fbx_matrix_to_matrix_44(FbxMatrix const & _matrix);
};
#endif