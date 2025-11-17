#include "aiManagerData.h"

#include "managerDatas\aiManagerData_backgroundMover.h"
#include "managerDatas\aiManagerData_spawnSet.h"

#include "..\..\core\io\xml.h"


using namespace TeaForGodEmperor;
using namespace AI;

//

REGISTER_FOR_FAST_CAST(ManagerData);

ManagerData::ManagerData()
{
}

ManagerData::~ManagerData()
{
}

bool ManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);

	return result;
}

bool ManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

void ManagerData::log(LogInfoContext& _log) const
{
	_log.log(TXT("ai manager data: %S"), name.to_char());
}

//

void ManagerDatas::initialise_static()
{
	base::initialise_static();

	ManagerDatasLib::BackgroundMover::register_itself();
	ManagerDatasLib::SpawnSet::register_itself();
}

void ManagerDatas::close_static()
{
	base::close_static();
}

