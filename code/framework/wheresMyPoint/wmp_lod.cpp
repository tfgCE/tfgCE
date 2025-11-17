#include "wmp_lod.h"

#include "..\meshGen\meshGenGenerationContext.h"

//

using namespace Framework;

//

bool LOD::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	range.load_from_xml(_node, TXT("range"));

	return result;
}

bool LOD::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	Optional<int> lodValue;
	WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
	{
		while (wmpOwner)
		{
			if (auto* context = fast_cast<MeshGeneration::GenerationContext>(wmpOwner))
			{
				lodValue = context->get_for_lod();
				break;
			}
			wmpOwner = wmpOwner->get_wmp_owner();
		}
	}
	if (lodValue.is_set())
	{
		if (range.is_empty())
		{
			_value.set<int>(lodValue.get());
			return true;
		}
		else
		{
			_value.set<bool>(range.does_contain(lodValue.get()));
			return true;
		}
	}

	if (!wmpOwner)
	{
		error_processing_wmp(TXT("mesh gen only!"));
		return false;
	}
	else
	{
		error_processing_wmp(TXT("didn't find mesh gen context!"));
		return false;
	}
}
