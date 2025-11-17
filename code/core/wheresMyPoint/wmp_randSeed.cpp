#include "wmp_randSeed.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool WheresMyPoint::RandSeed::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	seedBase.load_from_xml(_node, TXT("base"));
	seedOffset.load_from_xml(_node, TXT("offset"));

	return result;
}

bool WheresMyPoint::RandSeed::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	auto& rg = _context.access_random_generator();
	output(TXT("<randSeed base=\"%i\" offset=\"%i\"/>"), rg.get_seed_hi_number(), rg.get_seed_lo_number());
	if (seedBase.is_set())
	{
		rg.set_seed(seedBase.get(), seedOffset.get(0));
		output(TXT("changed to base=\"%i\" offset=\"%i\""), rg.get_seed_hi_number(), rg.get_seed_lo_number());
	}

	return result;
}
