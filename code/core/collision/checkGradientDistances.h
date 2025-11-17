#pragma once

#include "..\globalDefinitions.h"

struct Vector3;

namespace Collision
{
	interface_class ICollidableObject;
	interface_class ICollidableShape;

	struct CheckGradientDistances
	{
		struct Dir
		{
			float distance;
			ICollidableObject const * object;
			ICollidableShape const * shape;
		};
		Dir dirs[6];
		float maxDistance; // max current distance
		float maxInitialDistance;

		CheckGradientDistances(float _maxDistance, float _epsilon);

		void update_max_distance(float _epsilon); // this is max distance + epsilon * 2

		Vector3 get_gradient(float _epsilon) const;
	};

};
