#include "oculus_ge.h"

#ifdef AN_WITH_OCULUS
#endif

#ifdef AN_WITH_OCULUS_MOBILE
#endif

//

using namespace GameEnvironment;

//

void Oculus::init()
{
	new Oculus();
}

Oculus::Oculus()
{
}

Oculus::~Oculus()
{
}

Optional<bool> Oculus::is_ok() const
{
	return true;
#ifdef AN_WITH_OCULUS
	return true;
#endif
#ifdef AN_WITH_OCULUS_MOBILE
	return true;
#endif
	return NP;
}

void Oculus::set_achievement(Name const& _achievementName)
{
#ifdef AN_WITH_OCULUS
#endif
#ifdef AN_WITH_OCULUS_MOBILE
#endif
}
