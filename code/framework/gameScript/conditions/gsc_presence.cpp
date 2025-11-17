#include "gsc_presence.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\module\modulePresence.h"
#include "..\..\world\doorInRoom.h"
#include "..\..\world\region.h"
#include "..\..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

DEFINE_STATIC_NAME(self);

//

bool Conditions::Presence::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	objectVarBased = _node->get_name_attribute(TXT("objectVarBased"), objectVarBased);
	basedOnObjectVar = _node->get_name_attribute(TXT("basedOnObjectVar"), basedOnObjectVar);
	sameRoomAsObjectVar = _node->get_name_attribute(TXT("sameRoomAsObjectVar"), sameRoomAsObjectVar);
	sameOriginRoomAsObjectVar = _node->get_name_attribute(TXT("sameOriginRoomAsObjectVar"), sameOriginRoomAsObjectVar);
	
	inRoomVar = _node->get_name_attribute(TXT("inRoomVar"), inRoomVar);
	inRoomAtLeast.load_from_xml(_node, TXT("inRoomAtLeast"));
	belowZ.load_from_xml(_node, TXT("belowZ"));
	xInRange.load_from_xml(_node, TXT("xInRange"));
	yInRange.load_from_xml(_node, TXT("yInRange"));
	zInRange.load_from_xml(_node, TXT("zInRange"));
	maxSpeed.load_from_xml(_node, TXT("maxSpeed"));

	inRegionTagged.load_from_xml_attribute_or_child_node(_node, TXT("inRegionTagged"));
	inRoomTagged.load_from_xml_attribute_or_child_node(_node, TXT("inRoomTagged"));

	return result;
}

bool Conditions::Presence::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				if (auto* p = imo->get_presence())
				{
					if (objectVarBased.is_valid())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVarBased))
						{
							if (auto* wimo = exPtr->get())
							{
								if (auto* wp = wimo->get_presence())
								{
									if (wp->get_based_on() == imo)
									{
										anyOk = true;
									}
									else
									{
										anyFailed = true;
									}
								}
							}
						}
						else
						{
							error(TXT("no objectVarBased"));
						}
					}
					if (basedOnObjectVar.is_valid())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(basedOnObjectVar))
						{
							if (auto* wimo = exPtr->get())
							{
								if (p->get_based_on() == wimo)
								{
									anyOk = true;
								}
								else
								{
									anyFailed = true;
								}
							}
						}
						else
						{
							error(TXT("no basedOnObjectVar"));
						}
					}
					if (sameRoomAsObjectVar.is_valid())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(sameRoomAsObjectVar))
						{
							if (auto* wimo = exPtr->get())
							{
								if (auto* wp = wimo->get_presence())
								{
									if (wp->get_in_room() == p->get_in_room())
									{
										anyOk = true;
									}
									else
									{
										anyFailed = true;
									}
								}
							}
						}
						else
						{
							error(TXT("no sameRoomAsObjectVar"));
						}
					}
					if (sameOriginRoomAsObjectVar.is_valid())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(sameOriginRoomAsObjectVar))
						{
							if (auto* wimo = exPtr->get())
							{
								if (auto* wp = wimo->get_presence())
								{
									if (wp->get_in_room() == p->get_in_room() ||
										(wp->get_in_room() && p->get_in_room() && wp->get_in_room()->get_origin_room() == p->get_in_room()->get_origin_room()))
									{
										anyOk = true;
									}
									else
									{
										anyFailed = true;
									}
								}
							}
						}
						else
						{
							error(TXT("no sameOriginRoomAsObjectVar"));
						}
					}
					if (inRoomVar.is_valid())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar))
						{
							if (auto* room = exPtr->get())
							{
								if (room == p->get_in_room())
								{
									anyOk = true;
								}
								else
								{
									anyFailed = true;
								}
							}
						}
						else
						{
							error(TXT("no inRoomVar"));
						}
					}
					if (! inRegionTagged.is_empty())
					{
						bool itIs = false;
						auto* room = p->get_in_room();
						auto* r = room->get_in_region();
						while (r)
						{
							if (inRegionTagged.check(r->get_tags()))
							{
								itIs = true;
								break;
							}
							r = r->get_in_region();
						}
						if (itIs)
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
					if (!inRoomTagged.is_empty())
					{
						auto* room = p->get_in_room();
						if (inRoomTagged.check(room->get_tags()))
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
					if (inRoomAtLeast.is_set())
					{
						if (auto* r = p->get_in_room())
						{
							float minDist = inRoomAtLeast.get() + 1.0f;
							//FOR_EVERY_DOOR_IN_ROOM(dir, r)
							for_every_ptr(dir, r->get_doors())
							{
								if (!dir->is_visible()) continue;
								float dist = dir->calculate_dist_of(p->get_centre_of_presence_WS());
								minDist = min(dist, minDist);
							}
							if (minDist >= inRoomAtLeast.get())
							{
								anyOk = true;
							}
							else
							{
								anyFailed = true;
							}
						}
					}
					if (belowZ.is_set())
					{
						if (p->get_placement().get_translation().z < belowZ.get())
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
					if (xInRange.is_set())
					{
						if (xInRange.get().does_contain(p->get_placement().get_translation().x))
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
					if (yInRange.is_set())
					{
						if (yInRange.get().does_contain(p->get_placement().get_translation().y))
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
					if (zInRange.is_set())
					{
						if (zInRange.get().does_contain(p->get_placement().get_translation().z))
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
					if (maxSpeed.is_set())
					{
						if (p->get_velocity_linear().length() <= maxSpeed.get())
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
				}
			}
		}
	}

	return anyOk && ! anyFailed;
}
