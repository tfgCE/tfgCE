#include "tweakingContext.h"

#include "..\..\core\debug\debug.h"

using namespace Framework;

TweakingContext* TweakingContext::s_tweakingContext = nullptr;

void TweakingContext::initialise_static()
{
	an_assert(s_tweakingContext == nullptr);
	s_tweakingContext = new TweakingContext();
}

void TweakingContext::close_static()
{
	an_assert(s_tweakingContext);
	delete_and_clear(s_tweakingContext);
}
