#include "customLibraryStoredData.h"

#include "..\..\core\other\factoryOfNamed.h"

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(CustomLibraryData);

//

REGISTER_FOR_FAST_CAST(CustomLibraryStoredData);
LIBRARY_STORED_DEFINE_TYPE(CustomLibraryStoredData, customData);

typedef FactoryOfNamed<CustomLibraryData, Name> CustomLibraryStoredDataFactory;

CustomLibraryData* CustomLibraryStoredData::create_library_data(Name const & _type)
{
	if (CustomLibraryData* created = CustomLibraryStoredDataFactory::create(_type))
	{
		return created;
	}
	else
	{
		error(TXT("custom library stored data \"%S\" type not recognised"), _type.to_char());
		return nullptr;
	}
}

void CustomLibraryStoredData::register_custom_library_data(Name const & _name, ConstructCustomLibraryData _func)
{
	CustomLibraryStoredDataFactory::add(_name, _func);
}

void CustomLibraryStoredData::initialise_static()
{
	CustomLibraryStoredDataFactory::initialise_static();
}

void CustomLibraryStoredData::close_static()
{
	CustomLibraryStoredDataFactory::close_static();
}

CustomLibraryStoredData::CustomLibraryStoredData(Library * _library, LibraryName const & _name)
: base(_library, _name)
, data(nullptr)
{
}

CustomLibraryStoredData::~CustomLibraryStoredData()
{
}

#ifdef AN_DEVELOPMENT
void CustomLibraryStoredData::ready_for_reload()
{
	base::ready_for_reload();

	if (data.is_set())
	{
		data->ready_for_reload();
	}
}
#endif

bool CustomLibraryStoredData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

#ifdef AN_DEVELOPMENT
	if (data.is_set())
	{
		data->ready_for_reload();
		result &= data->load_from_xml(_node, _lc);
	}
#else
	if (data.is_set())
	{
		error_loading_xml(_node, TXT("data already exists for %S"), get_name().to_string().to_char());
		result = false;
	}
#endif
	else
	{
		type = _node->get_name_attribute(TXT("type"));
		if (type.is_valid())
		{
			data = create_library_data(type);
			if (data.is_set())
			{
				result &= data->load_from_xml(_node, _lc);
			}
			else
			{
				error_loading_xml(_node, TXT("no type found"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(_node, TXT("no type for custom library stored data provided"));
			result = false;
		}
	}

	return result;
}
		
bool CustomLibraryStoredData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (data.is_set())
	{
		result &= data->prepare_for_game(_library, _pfgContext);
	}
	else
	{
		result = false;
	}

	return result;
}

