#pragma once

#include "..\globalDefinitions.h"

struct String;

namespace Hand
{
	enum Type
	{
		Left = 0,
		Right = 1,
		MAX = 2,
	};

	inline Hand::Type other_hand(Hand::Type _hand) { return _hand == Left ? Right : (_hand == Right ? Left : MAX); }

	inline bool is_valid(Hand::Type _hand) { return _hand == Left || _hand == Right; }
	int get_vr_hand_index(Hand::Type);

	Type parse(String const & _text);

	tchar const * to_char(Type _hand);
};
