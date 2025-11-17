#include "iGameEnvironment.h"

#include "..\debug\debug.h"

//

using namespace GameEnvironment;

//

IGameEnvironment* IGameEnvironment::s_gameEnvironment = nullptr;

IGameEnvironment::IGameEnvironment()
{
	an_assert(! s_gameEnvironment);
	s_gameEnvironment = this;
}

IGameEnvironment::~IGameEnvironment()
{
	an_assert(s_gameEnvironment == this);
	s_gameEnvironment = nullptr;
}

void IGameEnvironment::close_static()
{
	if (s_gameEnvironment)
	{
		delete s_gameEnvironment;
	}
	an_assert(!s_gameEnvironment);
}
