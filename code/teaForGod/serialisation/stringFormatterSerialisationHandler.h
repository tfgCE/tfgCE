#pragma once

#include "..\..\framework\text\stringFormatterSerialisationHandlers.h"

namespace TeaForGodEmperor
{
	class StringFormatterSerialisationHandler
	: public Framework::ICustomStringFormatterParamSerialisationHandler
	{
	public:
		implement_ bool get_type_for_serialisation(Framework::ICustomStringFormatter const * _custom, REF_ Name & _type) const;
		implement_ bool does_serialise(Name const & _type) const;
		implement_ bool serialise(Serialiser& _serialiser, Framework::ICustomStringFormatterPtr & _custom, Name const & _type) const;
	};

};
