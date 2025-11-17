#include "wmp_uniqueID.h"

#include "..\game\game.h"

using namespace Framework;

//

bool UniqueID::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (auto * game = Game::get())
	{
		_value.set(game->get_unique_id());
	}
	else
	{
		error(TXT("no game object available"));
		result = false;
	}

	return result;
}

//
