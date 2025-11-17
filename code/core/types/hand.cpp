#include "hand.h"

#include "..\vr\iVR.h"

//

int Hand::get_vr_hand_index(Hand::Type _hand)
{
	if (auto * vr = VR::IVR::get())
	{
		return _hand == Hand::Left ? vr->get_left_hand() : vr->get_right_hand();
	}
	return 0;
}

Hand::Type Hand::parse(String const & _text)
{
	if (_text == TXT("left"))
	{
		return Hand::Left;
	}
	if (_text == TXT("right"))
	{
		return Hand::Right;
	}
	error(TXT("not recognised hand \"%S\""), _text.to_char());
	return Hand::Left;
}

tchar const * Hand::to_char(Hand::Type _hand)
{
	return _hand == Hand::Left ? TXT("left") : TXT("right");
}
