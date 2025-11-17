#include "wmp_allowMoreDetails.h"

#include "..\game\game.h"

using namespace Framework;

//

bool AllowMoreDetails::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	if (auto* g = Game::get())
	{
		_value.set<bool>(g->should_generate_more_details());
	}
	else
	{
		_value.set<bool>(false);
	}

	return true;
}
