#pragma once

#include "..\..\..\core\math\math.h"

namespace Framework
{
	namespace IKUtils
	{
		inline Vector2 ik2_calculate_joint_loc(float _length0, float _length1, float _dist);
		inline Matrix44 ik2_bone(Vector3 const & _loc, Vector3 const & _at, Vector3 const _right);

		#include "ikUtils.inl"
	};
};
