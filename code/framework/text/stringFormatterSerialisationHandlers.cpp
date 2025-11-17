#include "stringFormatterSerialisationHandlers.h"

#include "stringFormatter.h"

#include "..\..\core\serialisation\serialiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

StringFormatterSerialisationHandlers* StringFormatterSerialisationHandlers::stringFormatterSerialisationHandlers = nullptr;

void StringFormatterSerialisationHandlers::initialise_static()
{
	an_assert(stringFormatterSerialisationHandlers == nullptr);
	stringFormatterSerialisationHandlers = new StringFormatterSerialisationHandlers();
}

void StringFormatterSerialisationHandlers::close_static()
{
	an_assert(stringFormatterSerialisationHandlers);
	delete_and_clear(stringFormatterSerialisationHandlers);
}

void StringFormatterSerialisationHandlers::add_serialisation_handler(ICustomStringFormatterParamSerialisationHandler* _handler)
{
	an_assert(stringFormatterSerialisationHandlers);
	stringFormatterSerialisationHandlers->handlers.push_back(RefCountObjectPtr<ICustomStringFormatterParamSerialisationHandler>(_handler));
}

bool StringFormatterSerialisationHandlers::serialise(Serialiser& _serialiser, ICustomStringFormatterPtr & _custom)
{
	bool result = true;
	
	if (_serialiser.is_writing())
	{
		bool serialised = false;
		Name type;
		for_every_ref(h, get()->handlers)
		{
			if (h->get_type_for_serialisation(_custom.get()->get_actual(), REF_ type))
			{
				an_assert(type.is_valid());
				
				result &= serialise_data(_serialiser, type);
				result &= h->serialise(_serialiser, _custom, type);
				
				serialised = true;
				break;
			}
		}
		
		an_assert(serialised, TXT("no handler to serialise?"));

		result &= serialised;
	}
	
	if (_serialiser.is_reading())
	{
		bool serialised = false;
		Name type;
		result &= serialise_data(_serialiser, type);
		if (! type.is_valid())
		{
			result = false;
		}
		if (result)
		{
			for_every_ref(h, get()->handlers)
			{
				if (h->does_serialise(type))
				{
					result &= h->serialise(_serialiser, _custom, type);
					
					serialised = true;
					break;
				}
			}
		}
		
		an_assert(serialised, TXT("no handler to serialise?"));

		result &= serialised;
	}

	return result;
}
