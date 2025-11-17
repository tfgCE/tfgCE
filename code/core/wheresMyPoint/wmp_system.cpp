#include "wmp_system.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\system\core.h"

//

using namespace WheresMyPoint;

//

bool WheresMyPoint::System::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	systemTagRequired.load_from_xml_attribute(_node, TXT("systemTagRequired"));

	return result;
}


bool WheresMyPoint::System::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	bool anyOk = false;
	bool anyFailed = false;

	if (! systemTagRequired.is_empty())
	{
		if (systemTagRequired.check(::System::Core::get_system_tags()))
		{
			anyOk = true;
		}
		else
		{
			anyFailed = true;
		}
	}

	_value.set<bool>(anyOk && !anyFailed);

	return result;
}
