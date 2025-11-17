#include "environmentType.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\types\names.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(EnvironmentType);
LIBRARY_STORED_DEFINE_TYPE(EnvironmentType, environmentType);

EnvironmentType::EnvironmentType(Library * _library, LibraryName const & _name)
: base(_library, _name)
, timeBeforeUsageReset(-1.0f)
{
}

bool EnvironmentType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= info.load_from_xml(_node, _lc);

	timeBeforeUsageReset = _node->get_float_attribute(TXT("timeBeforeUsageReset"), timeBeforeUsageReset);

	return result;
}

bool EnvironmentType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= info.prepare_for_game(_library, _pfgContext);

	return result;
}

#ifdef AN_DEVELOPMENT
void EnvironmentType::ready_for_reload()
{
	base::ready_for_reload();

	info.clear();
}
#endif
