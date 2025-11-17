#include "wmp_hasDoor.h"

#include "..\meshGen\meshGenGenerationContext.h"
#include "..\meshGen\meshGenParamImpl.inl"

#include "..\library\library.h"

#include "..\world\door.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"

//

using namespace Framework;

//

bool HasDoor::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= tagCondition.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));

	return result;
}

bool HasDoor::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	bool anyOk = false;
	bool anyFailed = false;

	if (!tagCondition.is_empty())
	{
		bool thisOk = false;
		auto* owner = _context.get_owner();
		while (owner)
		{
			if (auto* r = fast_cast<Room>(owner))
			{
				for_every_ptr(dir, r->get_all_doors())
				{
					if (tagCondition.check(dir->get_tags()) ||
						(dir->get_door() && tagCondition.check(dir->get_door()->get_tags())))
					{
						thisOk = true;
					}
				}
			}
			owner = owner->get_wmp_owner();
		}
		anyOk |= thisOk;
		anyFailed |= ! thisOk;
	}

	bool queryResult = anyOk && !anyFailed;
	
	_value.set<bool>(queryResult);

	return result;
}
