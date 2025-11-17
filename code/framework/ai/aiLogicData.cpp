#include "aiLogicData.h"

using namespace Framework;
using namespace AI;

REGISTER_FOR_FAST_CAST(LogicData);

LogicData::LogicData()
{
}
	 
LogicData::~LogicData()
{
}

bool LogicData::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;
	autoRareAdvanceIfNotVisible.load_from_xml(_node, TXT("autoRareAdvanceIfNotVisible"));
	return result;
}

