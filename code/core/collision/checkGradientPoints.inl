template <typename CollisionQueryPrimitive>
CheckGradientPoints<CollisionQueryPrimitive>::CheckGradientPoints(CollisionQueryPrimitive const & _queryPrimitiveRelativeToPlacement, Matrix44 const & _placement, float _epsilon)
: queryPrimitiveRelativeToPlacement(_queryPrimitiveRelativeToPlacement)
, queryPrimitiveInPlace(_queryPrimitiveRelativeToPlacement.transform_to_world_of(_placement))
, placement(_placement)
, epsilon(_epsilon)
{
}

template <typename CollisionQueryPrimitive>
CheckGradientPoints<CollisionQueryPrimitive> CheckGradientPoints<CollisionQueryPrimitive>::transformed_to_world_of(Matrix44 const & _transformToNewSpace) const
{
	return CheckGradientPoints(queryPrimitiveRelativeToPlacement, _transformToNewSpace.to_world(placement), epsilon / _transformToNewSpace.extract_scale());
}

template <typename CollisionQueryPrimitive>
CheckGradientPoints<CollisionQueryPrimitive> CheckGradientPoints<CollisionQueryPrimitive>::transformed_to_local_of(Matrix44 const & _transformToNewSpace) const
{
	return CheckGradientPoints(queryPrimitiveRelativeToPlacement, _transformToNewSpace.to_local(placement), epsilon * _transformToNewSpace.extract_scale());
}

template <typename CollisionQueryPrimitive>
Vector3 CheckGradientPoints<CollisionQueryPrimitive>::get_point_offset(CheckGradientPoint::Type _type) const
{
	static const Vector3 points[] = { Vector3(-1.0f,  0.0f,  0.0f),
									  Vector3( 1.0f,  0.0f,  0.0f),
									  Vector3( 0.0f, -1.0f,  0.0f),
									  Vector3( 0.0f,  1.0f,  0.0f),
									  Vector3( 0.0f,  0.0f, -1.0f),
									  Vector3( 0.0f,  0.0f,  1.0f) };
	an_assert(_type >= 0 && _type < 6);
	return placement.vector_to_world(points[_type] * epsilon);
}

