#include "mggc_slidingLocomotion.h"

#include "..\..\game\game.h"

#include "..\..\framework.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool SlidingLocomotion::check(ElementInstance & _instance) const
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::is_preview_game())
	{
		// to allow seeing in the preview tool
		return true;
	}
#endif
	return Game::is_using_sliding_locomotion();
}
