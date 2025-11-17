#include "wmp_slidingLocomotion.h"

#include "..\debug\previewGame.h"
#include "..\game\game.h"

//

using namespace Framework;

//

bool SlidingLocomotion::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;


	bool slidingLocomotion = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::is_preview_game())
	{
		// to allow seeing in the preview tool
		slidingLocomotion = true;
	}
#endif
	slidingLocomotion |= Game::is_using_sliding_locomotion();

	_value.set(slidingLocomotion);

	return result;
}

//
