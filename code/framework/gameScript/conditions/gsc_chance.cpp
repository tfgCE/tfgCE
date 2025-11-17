#include "gsc_chance.h"

#include "..\..\..\core\io\xml.h"

#include "..\..\..\core\random\random.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool Chance::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	chance = _node->get_float_attribute_or_from_child(TXT("chance"), chance);

	return result;
}

bool Chance::check(ScriptExecution const & _execution) const
{
	bool result = false;

	if (Random::get_chance(chance))
	{
		result = true;
	}

	return result;
}
