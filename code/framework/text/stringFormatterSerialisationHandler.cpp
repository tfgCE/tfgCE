#include "stringFormatterSerialisationHandler.h"

#include "..\time\dateTime.h"
#include "..\time\time.h"

#include "..\..\core\serialisation\serialiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

bool StringFormatterSerialisationHandler::get_type_for_serialisation(Framework::ICustomStringFormatter const * _custom, REF_ Name & _type) const
{
	SERIALISATION_HANDLER_GET_TYPE(DateTime);
	SERIALISATION_HANDLER_GET_TYPE(Time);
	return false;
}

bool StringFormatterSerialisationHandler::does_serialise(Name const & _type) const
{
	SERIALISATION_HANDLER_DOES_SERIALISE(DateTime);
	SERIALISATION_HANDLER_DOES_SERIALISE(Time);
	return false;
}

bool StringFormatterSerialisationHandler::serialise(Serialiser& _serialiser, Framework::ICustomStringFormatterPtr & _custom, Name const & _type) const
{
	bool result = true;

	SERIALISATION_HANDLER_SERIALISE_VALUE(DateTime);
	SERIALISATION_HANDLER_SERIALISE_VALUE(Time);

	todo_important(TXT("I don't know how to serialise \"%S\""), _type.to_char());

	return result;
}
