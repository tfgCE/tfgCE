#include "regionGeneratorBase.h"

#include "..\utils.h"

using namespace TeaForGodEmperor;
using namespace RegionGenerators;

//

REGISTER_FOR_FAST_CAST(Base);

void Base::initialise_static()
{
	Framework::RegionGeneratorInfo::register_region_generator_info(String::empty(), []() { return new Base(); });
}

void Base::close_static()
{
}

Base::Base()
{
}

Base::~Base()
{
}

bool Base::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= aiManagers.load_from_xml(_node, _lc);

	return result;
}

bool Base::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= aiManagers.prepare_for_game(_library, _pfgContext);

	return result;
}

bool Base::post_generate(Framework::Region* _region) const
{
	bool result = true;

	scoped_call_stack_info(TXT("RegionGenerators::Base::post_generate"));

	result &= aiManagers.add_to_region(_region);

	return result;
}
