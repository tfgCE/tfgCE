#pragma once

#include "..\math\math.h"

namespace Collision
{
	/**
	 *	This structure is used to calculate gradient
	 *	For convex shapes we are able to tell exactly how gradient is pointed (normal) and it's distance from surface.
	 *	And for most of the shapes (except convex hull, but that's still kind of ok, althouth result is approximated)
	 *	we use negative distance.
	 *
	 *	Negative distance tells how much inside the shape we are. Zero distance is at the surface. Positive is outside the shape.
	 *	This is used to allow pushing away from the collision shape in case we get embedded in it (when for example collision shape is turned on).
	 *	Negative distance allows to use concept of equilibrium shape. This could be point, line, plane or surface or mix of those.
	 *	We do not have any kind of implementation of equilibrium shapes - they happen to be at place of lowest (negative) distance.
	 *	Concept of equilibrium shapes allows to control where object should be pushed away in case it gets embedded.
	 *
	 *	Example of use:
	 *		For doors that are exactly between two spaces we					For two doors that belong to each shape we should use greater
	 *		may use normal shape for them. equilibrium plane					collision to have equilibrium plane between spaces.
	 *		will be exactly between two spaces.
	 *																		   actual	    with extended
	 *																			door		  collision
	 *				+---+														+-+				+-+~*
	 *				|   |														| |				| |	!
	 *				| : |		side view										| |				| : !			and same for second door
	 *				| : |														| |				| : !				  in space b
	 *				| : |		: equilibrium plane								| |				| : !
	 *				| : |														| |				| : !
	 *				|   |														| |				| | !
	 *				+---+														+-+				+-+~*
	 *		----------+----------											 -----+-----	 -----+-----
	 *		  space a | space b													a | b			a | b
	 */
	struct GradientQueryResult
	{
		float distance;
		NORMAL_ Vector3 normal; // collision normal

		GradientQueryResult(float _distance, Vector3 const & _normal);

	};
};
