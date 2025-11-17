#include "..\math\math.h"
#include "checkGradientDistances.h"
#include "checkGradientPoints.h"

using namespace Collision;

CheckGradientDistances::CheckGradientDistances(float _maxDistance, float _epsilon)
: maxInitialDistance( _maxDistance )
{
	Dir* dir = dirs;
	for (int idx = 0; idx < 6; ++idx, ++dir)
	{
		dir->distance = _maxDistance + _epsilon;
		dir->object = nullptr;
		dir->shape = nullptr;
	}
	update_max_distance(_epsilon);
}

void CheckGradientDistances::update_max_distance(float _epsilon)
{
	maxDistance = max( max(dirs[0].distance, dirs[1].distance),
				  max( max(dirs[2].distance, dirs[3].distance),
					   max(dirs[4].distance, dirs[5].distance) ) );
	maxDistance += _epsilon * 1.5f;
}

Vector3 CheckGradientDistances::get_gradient(float _epsilon) const
{
	//	note:	I am not entirely convinced about clamping results here, although it makes kind of sense
	//			we're interested if something is at top distance from us (because we probe points moved by epsilon, we reduce max distance by epsilon)
	//			we're not interested what happens beyond that range
	float maxRadius = maxInitialDistance;
	float distCorr[] = { min(maxRadius, dirs[0].distance),
						 min(maxRadius, dirs[1].distance),
						 min(maxRadius, dirs[2].distance),
						 min(maxRadius, dirs[3].distance),
						 min(maxRadius, dirs[4].distance),
						 min(maxRadius, dirs[5].distance) };

	float distance = (distCorr[0] + distCorr[1] + distCorr[2] + distCorr[3] + distCorr[4] + distCorr[5]) / 6.0f;

	// gradient should point where distance grows, but value should be dependant on actual distance (negative distance should be treated just as it would be on surface)
	return max(maxRadius - distance, 0.0f) *
		Vector3((dirs[CheckGradientPoint::XP].distance - dirs[CheckGradientPoint::XM].distance),
				(dirs[CheckGradientPoint::YP].distance - dirs[CheckGradientPoint::YM].distance),
				(dirs[CheckGradientPoint::ZP].distance - dirs[CheckGradientPoint::ZM].distance)).normal();
}
