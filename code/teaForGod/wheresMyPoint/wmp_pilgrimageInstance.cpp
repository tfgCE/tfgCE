#include "wmp_pilgrimageInstance.h"

#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageInstance.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

bool PilgrimageInstanceForWheresMyPoint::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	currentOrNext.load_from_xml(_node, TXT("currentOrNext"));

	return result;
}

bool PilgrimageInstanceForWheresMyPoint::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	bool matches = false;
	if (auto* pi = TeaForGodEmperor::PilgrimageInstance::get())
	{
		if (auto* p = pi->get_pilgrimage())
		{
			matches |= (currentOrNext.get_name() == p->get_name());
		}
		if (auto* p = pi->get_next_pilgrimage())
		{
			matches |= (currentOrNext.get_name() == p->get_name());
		}
		if (auto* npi = pi->get_next_pilgrimage_instance())
		{
			if (auto* p = npi->get_next_pilgrimage())
			{
				matches |= (currentOrNext.get_name() == p->get_name());
			}
		}
	}

	_value.set<bool>(matches);

	return result;
}
