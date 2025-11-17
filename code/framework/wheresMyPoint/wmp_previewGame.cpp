#include "wmp_previewGame.h"

#include "..\framework.h"

//

using namespace Framework;

//

bool PreviewGameWMP::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<bool>(Framework::is_preview_game());
	
	return result;
}

//
