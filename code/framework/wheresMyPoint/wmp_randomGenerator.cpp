#include "wmp_randomGenerator.h"

#include "..\meshGen\meshGenGenerationContext.h"

#include "..\world\door.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool SpawnRandomGenerator::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<Random::Generator>(_context.access_random_generator().spawn());

	return result;
}

//

bool UseRandomGenerator::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	restoreFrom = _node->get_name_attribute(TXT("restoreFrom"), restoreFrom);

	return result;
}

bool UseRandomGenerator::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	Optional<Random::Generator> rg;
	if (restoreFrom.is_valid() && _context.get_owner())
	{
		WheresMyPoint::Value value;
		if (_context.get_owner()->restore_value_for_wheres_my_point(restoreFrom, value))
		{
			if (value.get_type() == type_id<Random::Generator>())
			{
				rg = value.get_as<Random::Generator>();
			}
		}
	}
	else if (_value.get_type() == type_id<Random::Generator>())
	{
		rg = _value.get_as<Random::Generator>();
	}
	
	if (rg.is_set())
	{
		_context.access_random_generator() = rg.get();
		if (auto* gc = fast_cast<Framework::MeshGeneration::GenerationContext>(_context.get_owner()))
		{
			gc->use_random_generator(rg.get());
		}
		if (auto* ei = fast_cast<Framework::MeshGeneration::ElementInstance>(_context.get_owner()))
		{
			ei->access_context().use_random_generator(rg.get());
		}
	}
	else
	{
		warn_processing_wmp_tool(this, TXT("can't restore random generator - no source"));
	}

	return result;
}

//
