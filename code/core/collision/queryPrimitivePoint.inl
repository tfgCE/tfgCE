template <typename ShapeObject>
GradientQueryResult QueryPrimitivePoint::get_gradient(ShapeObject const & _shape, float _maxDistance, bool _square) const
{
	AN_CLANG_TYPENAME ShapeObject::QueryContext queryContext;
	return _shape.get_gradient(*this, REF_ queryContext, _maxDistance, _square);
}

