#pragma once

struct Matrix44;
struct Vector3;

namespace Collision
{
	class ModelInstance;
	class ModelInstanceSet;

	namespace CheckGradientPoint
	{
		enum Type
		{
			Centre	= -1,
			XM		=  0, // x minus
			XP		=  1, // x plus
			YM		=  2, // y minus
			YP		=  3, // y plus
			ZM		=  4, // z minus
			ZP		=  5, // z plus
		};

		inline Vector3 get_dir(Type _type)
		{
			if (_type == XM) return Vector3(-1.0f, 0.0f, 0.0f);
			if (_type == XP) return Vector3( 1.0f, 0.0f, 0.0f);
			if (_type == YM) return Vector3(0.0f, -1.0f, 0.0f);
			if (_type == YP) return Vector3(0.0f,  1.0f, 0.0f);
			if (_type == ZM) return Vector3(0.0f, 0.0f, -1.0f);
			if (_type == ZP) return Vector3(0.0f, 0.0f,  1.0f);
			an_assert(_type == Centre);
			return Vector3(0.0f, 0.0f, 0.0f);
		}
	};

	template <typename CollisionQueryPrimitive>
	struct CheckGradientPoints
	{
		CollisionQueryPrimitive queryPrimitiveRelativeToPlacement;
		CollisionQueryPrimitive queryPrimitiveInPlace;
		Matrix44 placement;
		float epsilon;

		CheckGradientPoints(CollisionQueryPrimitive const & _queryPrimitiveRelativeToPlacement, Matrix44 const & _placement, float _epsilon);

		CheckGradientPoints transformed_to_world_of(Matrix44 const & _transformToNewSpace) const;
		CheckGradientPoints transformed_to_world_of(Transform const & _transformToNewSpace) const;
		CheckGradientPoints transformed_to_local_of(Matrix44 const & _transformToNewSpace) const;
		CheckGradientPoints transformed_to_local_of(Transform const & _transformToNewSpace) const;

		Vector3 get_point_offset(CheckGradientPoint::Type _type) const;
	};

	#include "checkGradientPoints.inl"

};
