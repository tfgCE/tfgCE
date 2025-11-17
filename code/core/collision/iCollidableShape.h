#pragma once

#include "..\globalDefinitions.h"
#include "..\fastCast.h"

struct Name;

namespace Meshes
{
	class Skeleton;
};

namespace Collision
{
	class PhysicalMaterial;

	/*
	 *	Collidable shape (might be object or something else)
	 */
	interface_class ICollidableShape
	{
		FAST_CAST_DECLARE(ICollidableShape);
		FAST_CAST_END();
	public:
		ICollidableShape();
		virtual ~ICollidableShape() {}

		virtual PhysicalMaterial const * get_material() const = 0;
		virtual void set_collidable_shape_bone(Name const & _boneName) = 0;
		virtual Name const & get_collidable_shape_bone() const = 0;
		virtual int get_collidable_shape_bone_index(Meshes::Skeleton const * _skeleton) const = 0;
	};
};
