#include "wmp_teaBoxSeed.h"

#include "..\game\playerSetup.h"

#include "..\..\framework\framework.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

bool TeaBoxSeed::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	return result;
}

bool TeaBoxSeed::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	bool provided = false;
	if (auto* ps = PlayerSetup::get_current_if_exists())
	{
		if (auto* gs = ps->get_active_game_slot())
		{
			_value.set<int>(gs->teaBoxSeed);
			provided = true;
		}
	}

	if (!provided)
	{
		_value.set<int>(Random::next_int());
	}

	return result;
}
