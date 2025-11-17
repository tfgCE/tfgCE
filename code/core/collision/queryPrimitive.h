#pragma once

namespace Collision
{
	// this is just template for query primitive class
	struct QueryPrimitive
	{
	public:
		/*
		QueryPrimitive transform_to_world_of(Matrix44 const & _matrix) const;
		QueryPrimitive transform_to_local_of(Matrix44 const & _matrix) const;
		Range3 calculate_bounding_box(float _radius) const;
		*/
		virtual void never_derive_from_this_class() = 0;
	};

};
