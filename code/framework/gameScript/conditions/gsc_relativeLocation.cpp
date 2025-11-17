#include "gsc_relativeLocation.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\module\moduleAppearance.h"
#include "..\..\modulesOwner\modulesOwner.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\io\xml.h"

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

bool RelativeLocation::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	toObjectVar = _node->get_name_attribute(TXT("toObjectVar"), toObjectVar);
	toSocket = _node->get_name_attribute(TXT("toSocket"), toSocket);
	
	checkObjectVar = _node->get_name_attribute(TXT("checkObjectVar"), checkObjectVar);
	checkSocket = _node->get_name_attribute(TXT("checkSocket"), checkSocket);

	xLessThan.load_from_xml(_node, TXT("xLessThan"));
	xGreaterThan.load_from_xml(_node, TXT("xGreaterThan"));
	yLessThan.load_from_xml(_node, TXT("yLessThan"));
	yGreaterThan.load_from_xml(_node, TXT("yGreaterThan"));

	error_loading_xml_on_assert(xLessThan.is_set()
							 || xGreaterThan.is_set()
							 || yLessThan.is_set()
							 || yGreaterThan.is_set()
							 , result, _node, TXT("we require at least actual location condition to be provided, check code if condition implemented!"));

	return result;
}

bool RelativeLocation::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	bool noObject = true;

	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(toObjectVar))
	{
		if (auto* toIMO = exPtr->get())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(checkObjectVar))
			{
				if (auto* checkIMO = exPtr->get())
				{
					noObject = false;
					bool anyCondition = false;

					if (toIMO->get_presence()->get_in_room() == checkIMO->get_presence()->get_in_room())
					{
						Transform toPlacement = toIMO->get_presence()->get_placement();
						Transform checkPlacement = checkIMO->get_presence()->get_placement();
						if (toSocket.is_valid())
						{
							if (auto* a = toIMO->get_appearance())
							{
								toPlacement = toPlacement.to_world(a->calculate_socket_os(toSocket));
							}
						}
						if (checkSocket.is_valid())
						{
							if (auto* a = checkIMO->get_appearance())
							{
								checkPlacement = checkPlacement.to_world(a->calculate_socket_os(checkSocket));
							}
						}
						Vector3 relLoc = toPlacement.location_to_local(checkPlacement.get_translation());
						if (xLessThan.is_set())
						{
							anyCondition = true;
							if (relLoc.x < xLessThan.get())
							{
								anyOk = true;
							}
							else
							{
								anyFailed = true;
							}
						}
						if (xGreaterThan.is_set())
						{
							anyCondition = true;
							if (relLoc.x > xGreaterThan.get())
							{
								anyOk = true;
							}
							else
							{
								anyFailed = true;
							}
						}
						if (yLessThan.is_set())
						{
							anyCondition = true;
							if (relLoc.y < yLessThan.get())
							{
								anyOk = true;
							}
							else
							{
								anyFailed = true;
							}
						}
						if (yGreaterThan.is_set())
						{
							anyCondition = true;
							if (relLoc.y > yGreaterThan.get())
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
						warn(TXT("not in the same room!"));
					}
				}
			}
		}
	}

	if (noObject)
	{
		// what to do? allow or skip?
	}

	return anyOk && ! anyFailed;
}
