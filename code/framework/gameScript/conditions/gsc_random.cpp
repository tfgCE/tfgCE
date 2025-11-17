#include "gsc_random.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::Random::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	chance.load_from_xml(_node, TXT("chance"));

	return result;
}

bool Conditions::Random::check(ScriptExecution const & _execution) const
{
	if (chance.is_set())
	{
		return ::Random::get_chance(chance.get());
	}

	// 50/50 then
	return ::Random::get_bool();
}
