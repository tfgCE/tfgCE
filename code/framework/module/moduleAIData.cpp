#include "moduleAIData.h"

#include "..\library\usedLibraryStored.inl"

#include "..\library\library.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ModuleAIData);

ModuleAIData::ModuleAIData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleAIData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("aiMind"))
	{
		bool result = true;
		result &= mind.load_from_xml(_node, TXT("name"), _lc);
		return result;
	}
	else if (_node->get_name() == TXT("aiMindControlledByPlayer"))
	{
		bool result = true;
		result &= mindControlledByPlayer.load_from_xml(_node, TXT("name"), _lc);
		return result;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool ModuleAIData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (mind.is_name_valid())
	{
		result &= mind.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (mindControlledByPlayer.is_name_valid())
	{
		result &= mindControlledByPlayer.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}
