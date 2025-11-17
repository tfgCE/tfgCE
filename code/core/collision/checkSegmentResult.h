#pragma once

#include "..\math\math.h"

#include "..\globalDefinitions.h"

namespace Collision
{
	interface_class ICollidableShape;
	interface_class ICollidableObject;

	struct Flags;
	class PhysicalMaterial;
	class Model;

	struct CheckSegmentResult
	{
		bool hit;
		Vector3 hitLocation;
		Vector3 hitNormal;

#ifdef AN_DEVELOPMENT
		Model const * model;
#endif
		ICollidableShape const * shape;
		ICollidableObject const * object;
		PhysicalMaterial const * material;

		Optional<Vector3> gravityDir;
		Optional<float> gravityForce;

		Optional<bool> isWalkable;

		CheckSegmentResult();

		void to_local_of(Transform const & _placement);
		void to_world_of(Transform const & _placement);

		bool has_collision_flags(Flags const & _flagsToTest, bool _defaultResult = true) const;
	};

};
