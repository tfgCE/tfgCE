#include "pico.h"

#ifdef AN_PICO
#endif

//

using namespace GameEnvironment;

//

void Pico::init()
{
	new Pico();
}

Pico::Pico()
{
}

Pico::~Pico()
{
}

Optional<bool> Pico::is_ok() const
{
	return true;
	return NP;
}

void Pico::set_achievement(Name const& _achievementName)
{
}
