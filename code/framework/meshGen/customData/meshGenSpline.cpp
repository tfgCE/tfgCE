#include "meshGenSpline.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenSubStepDef.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace CustomData;

//

DEFINE_STATIC_NAME(zero);

//

#ifndef AN_CLANG
template class Spline<Vector2>;
template class Spline<Vector3>;
template struct SplineContext<Vector2>;
template struct SplineContext<Vector3>;
#endif

//

AN_CLANG_TEMPLATE_SPECIALIZATION REGISTER_FOR_FAST_CAST(Framework::MeshGeneration::CustomData::Spline<Vector2>);
AN_CLANG_TEMPLATE_SPECIALIZATION REGISTER_FOR_FAST_CAST(Framework::MeshGeneration::CustomData::Spline<Vector3>);

#include "meshGenSplineImpl.inl"

template <>
Matrix33 Framework::MeshGeneration::CustomData::Spline<Vector2>::build_transform_matrix_33(Array<RefPoint> const& _refPoints) const
{
	ArrayStatic<Vector2, 3> fromPoints; SET_EXTRA_DEBUG_INFO(fromPoints, TXT("MeshGeneration::CustomData::Spline.fromPoints"));
	ArrayStatic<Vector2, 3> toPoints; SET_EXTRA_DEBUG_INFO(toPoints, TXT("MeshGeneration::CustomData::Spline.toPoints"));

	for_every(refPoint, _refPoints)
	{
		if (has_ref_point(refPoint->name))
		{
			if (fromPoints.get_size() == 3)
			{
				todo_important(TXT("no support for more than 3 points yet!"));
				break;
			}
			fromPoints.push_back(*find_ref_point(refPoint->name));
			toPoints.push_back(refPoint->location);
		}
	}

	Matrix33 from = Matrix33::identity;
	Matrix33 to = Matrix33::identity;

	if (fromPoints.get_size() >= 1)
	{
		from.set_translation(fromPoints[0]);
		to.set_translation(toPoints[0]);
		if (fromPoints.get_size() >= 2)
		{
			from.set_y_axis(fromPoints[1] - fromPoints[0]);
			to.set_y_axis(toPoints[1] - toPoints[0]);
			if (fromPoints.get_size() >= 3)
			{
				from.set_x_axis(fromPoints[2] - fromPoints[0]);
				to.set_x_axis(toPoints[2] - toPoints[0]);
			}
			else
			{
				from.set_x_axis((fromPoints[1] - fromPoints[0]).rotated_right());
				to.set_x_axis((toPoints[1] - toPoints[0]).rotated_right());
			}
		}
	}

	return to.to_world(from.full_inverted());
}

template <>
Matrix33 Framework::MeshGeneration::CustomData::Spline<Vector3>::build_transform_matrix_33(Array<RefPoint> const& _refPoints) const
{
	an_assert(TXT("should not be used!"));
	return Matrix33::identity;
}

template <>
Matrix44 Framework::MeshGeneration::CustomData::Spline<Vector2>::build_transform_matrix_44(Array<RefPoint> const& _refPoints) const
{
	an_assert(TXT("should not be used!"));
	return Matrix44::identity;
}

template <>
Matrix44 Framework::MeshGeneration::CustomData::Spline<Vector3>::build_transform_matrix_44(Array<RefPoint> const& _refPoints) const
{
	an_assert(TXT("should not be used!"));
	return Matrix44::identity;
}
