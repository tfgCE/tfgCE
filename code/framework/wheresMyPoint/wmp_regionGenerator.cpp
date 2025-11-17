#include "wmp_regionGenerator.h"

#include "..\world\door.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool RegionGeneratorOuterConnectorsCount::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
	while (wmpOwner)
	{
		if (auto * co = fast_cast<RegionGeneratorContextOwner>(wmpOwner))
		{
			int ocCount = co->generator->get_outer_connector_count();
			if (ocCount == 0)
			{
				warn_processing_wmp_tool(this, TXT("no outer connectors, maybe this should be placed not in \"wheresMyPointOnSetupGenerator\" but in \"wheresMyPointOnGenerate\"?"));
			}
			_value.set(ocCount);
			return true;
		}
		wmpOwner = wmpOwner->get_wmp_owner();
	}
	if (!wmpOwner)
	{
		error_processing_wmp(TXT("region generator only!"));
		return false;
	}

	return result;
}

//

bool RegionGeneratorCheckGenerationTags::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	check.load_from_xml_attribute_or_node(_node, TXT("check"));

	error_loading_xml_on_assert(!check.is_empty(), result, _node, TXT("shouldn't be left empty"));

	return result;
}

bool RegionGeneratorCheckGenerationTags::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
	while (wmpOwner)
	{
		if (auto * co = fast_cast<RegionGeneratorContextOwner>(wmpOwner))
		{
			bool checkResult = check.check(co->generator->get_generation_context().generationTags);
			_value.set<bool>(checkResult);
			return true;
		}
		wmpOwner = wmpOwner->get_wmp_owner();
	}
	if (!wmpOwner)
	{
		error_processing_wmp(TXT("region generator only!"));
		return false;
	}

	return result;
}

//
