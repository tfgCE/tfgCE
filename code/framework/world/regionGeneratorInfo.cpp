#include "regionGeneratorInfo.h"

#include "..\..\core\other\factoryOfNamed.h"

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(RegionGeneratorInfo);

typedef FactoryOfNamed<RegionGeneratorInfo, String> RegionGeneratorInfoFactory;

void RegionGeneratorInfo::register_region_generator_info(String const & _name, RegionGeneratorInfo::Construct _func)
{
	RegionGeneratorInfoFactory::add(_name, _func);
}

void RegionGeneratorInfo::initialise_static()
{
	RegionGeneratorInfoFactory::initialise_static();
}

void RegionGeneratorInfo::close_static()
{
	RegionGeneratorInfoFactory::close_static();
}

RegionGeneratorInfo* RegionGeneratorInfo::create(String const & _name)
{
	return RegionGeneratorInfoFactory::create(_name);
}

bool RegionGeneratorInfo::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	return result;
}

bool RegionGeneratorInfo::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}
