#include "aiCommonVariables.h"

#include "logics\aiLogic_npcBase.h"

#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\presence\presencePath.h"
#include "..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\framework\world\inRoomPlacement.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;

//

// mobility types
DEFINE_STATIC_NAME(sedentary);
DEFINE_STATIC_NAME(ambushed);
DEFINE_STATIC_NAME(lurking);
DEFINE_STATIC_NAME(curious);
DEFINE_STATIC_NAME(hunter);

// room tags
DEFINE_STATIC_NAME(transportRoom);
DEFINE_STATIC_NAME(notWorthScanningRoom);
DEFINE_STATIC_NAME(worthScanningRoom);

//

#define VAR(type, var) \
	DEFINE_STATIC_NAME(var); \
	void TeaForGodEmperor::AI::GetCommonVariable::var(::Framework::AI::MindInstance* _mind, OUT_ type*& _var) \
	{ \
		if (auto* npcBase = fast_cast<Logics::NPCBase>(_mind->get_logic())) \
		{ \
			if (! npcBase->var##Ptr) \
			{ \
				SimpleVariableInfo tempVar = _mind->get_owner_as_modules_owner()->access_variables().find<type>(NAME(var)); \
				npcBase->var##Ptr = &tempVar.access<type>(); \
			} \
			_var = npcBase->var##Ptr; \
		} \
		else \
		{ \
			SimpleVariableInfo tempVar = _mind->get_owner_as_modules_owner()->access_variables().find<type>(NAME(var)); \
			_var = &tempVar.access<type>(); \
		} \
	}

#include "aiCommonVariablesList.h"

#undef VAR

//

#define VAR(type, var) \
	if (auto* existingVar = _mind->get_owner_as_modules_owner()->get_variables().get_existing<type>(NAME(var))) \
	{ \
		RegisteredType::log_value(_log, *existingVar, TXT(#var)); \
	}

void AI::log_common_variables(LogInfoContext & _log, ::Framework::AI::MindInstance* _mind)
{
	if (_mind->get_owner_as_modules_owner())
	{
		#include "aiCommonVariablesList.h"
	}
}

#undef VAR

#define ACCESS_VAR(type, var) \
	type * var##Ptr; \
	TeaForGodEmperor::AI::GetCommonVariable::var(_imo->get_ai()->get_mind(), var##Ptr); \
	an_assert(var##Ptr); \
	type & var = *var##Ptr;

//

bool Mobility::may_leave_room(Framework::IModulesOwner* _imo, Framework::Room* _room)
{
	ACCESS_VAR(bool, remainInRoom);
	if (remainInRoom)
	{
		return false;
	}

	ACCESS_VAR(Name, mobilityType);
	ACCESS_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	//ACCESS_VAR(Framework::InRoomPlacement, investigate);
	bool hasEnemy = enemy.get() != nullptr;
	//bool wantsToInvestigate = investigate.is_valid();

	if (mobilityType == NAME(sedentary))
	{
		return false;
	}
	if (mobilityType == NAME(lurking))
	{
		return hasEnemy;
	}
	if (mobilityType == NAME(curious))
	{
		return true;
	}
	if (mobilityType == NAME(hunter))
	{
		return true;
	}

	// default?
	return true;
}

bool Mobility::may_leave_room_to_wander(Framework::IModulesOwner* _imo, Framework::Room* _room)
{
	ACCESS_VAR(bool, remainInRoom);
	if (remainInRoom)
	{
		return false;
	}

	ACCESS_VAR(Name, mobilityType);
	ACCESS_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	bool hasEnemy = enemy.get() != nullptr;

	if (mobilityType == NAME(sedentary))
	{
		return false;
	}
	if (mobilityType == NAME(lurking))
	{
		return hasEnemy;
	}
	if (mobilityType == NAME(curious))
	{
		return true;
	}
	if (mobilityType == NAME(hunter))
	{
		return true;
	}

	// default?
	return true;
}

bool Mobility::may_leave_room_to_investigate(Framework::IModulesOwner* _imo, Framework::Room* _room)
{
	ACCESS_VAR(bool, remainInRoom);
	if (remainInRoom)
	{
		return false;
	}

	ACCESS_VAR(Name, mobilityType);
	ACCESS_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	bool hasEnemy = enemy.get() != nullptr;

	if (mobilityType == NAME(sedentary))
	{
		return false;
	}
	if (mobilityType == NAME(lurking))
	{
		return hasEnemy;
	}
	if (mobilityType == NAME(curious))
	{
		return true;
	}
	if (mobilityType == NAME(hunter))
	{
		return true;
	}

	// default?
	return true;
}

bool Mobility::may_leave_room_when_attacking(Framework::IModulesOwner* _imo, Framework::Room* _room)
{
	ACCESS_VAR(bool, remainInRoom);
	if (remainInRoom)
	{
		return false;
	}

	ACCESS_VAR(Name, mobilityType);
	bool transportRoom = _room->get_tags().get_tag(NAME(transportRoom)) != 0.0f;
	if (mobilityType == NAME(sedentary))
	{
		return false;
	}
	if (mobilityType == NAME(lurking))
	{
		return transportRoom;
	}
	if (mobilityType == NAME(curious))
	{
		return true;
	}
	if (mobilityType == NAME(hunter))
	{
		todo_note(TXT("add one more type"));
		return true;
	}

	// default?
	return true;
}

bool Mobility::should_remain_stationary(Framework::IModulesOwner* _imo)
{
	ACCESS_VAR(Name, mobilityType);
	ACCESS_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	bool hasEnemy = enemy.get() != nullptr;

	if (mobilityType == NAME(sedentary) || mobilityType == NAME(ambushed))
	{
		return !hasEnemy;
	}
	
	return false;
}

bool Mobility::is_in_transport_room(Framework::IModulesOwner* _imo)
{
	return is_transport_room(_imo->get_presence()->get_in_room());
}

bool Mobility::is_transport_room(Framework::Room* _room)
{
	return _room && _room->get_tags().get_tag(NAME(transportRoom)) != 0.0f;
}

//

bool Investigate::is_close_and_in_front(Framework::IModulesOwner* _imo, float _dist, float _inFrontAngle)
{
	ACCESS_VAR(Framework::InRoomPlacement, investigate);

	if (investigate.is_valid() && investigate.inRoom == _imo->get_presence()->get_in_room())
	{
		Vector3 toInvestigate = (investigate.placement.get_translation() - _imo->get_presence()->get_placement().get_translation());
		return toInvestigate.length_squared() < sqr(_dist) &&
			Vector3::dot(toInvestigate.normal(), _imo->get_presence()->get_placement().get_axis(Axis::Y)) >= cos_deg(_inFrontAngle);
	}

	todo_note(TXT("check adjacent room? or it may not make any sense as investigate should be in our room only?"));

	return false;
}

bool Investigate::is_in_worth_scanning(Framework::IModulesOwner* _imo)
{
	return is_worth_scanning(_imo->get_presence()->get_in_room());
}

bool Investigate::is_worth_scanning(Framework::Room* _room)
{
	return _room && (_room->get_tags().get_tag(NAME(notWorthScanningRoom)) == 0.0f || _room->get_tags().get_tag(NAME(worthScanningRoom)) != 0.0f);
}

//

Vector3 Targetting::get_enemy_targetting_in_owner_room(Framework::IModulesOwner* _imo)
{
	ACCESS_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	ACCESS_VAR(Vector3, enemyTargetingOffsetOS);

	if (!enemyPlacement.is_active())
	{
		warn(TXT("no enemy!"));
		return _imo->get_presence()->get_placement().get_translation();
	}
	Transform placement = enemyPlacement.get_placement_in_owner_room();

	if (enemyPlacement.get_target() && _imo->get_ai())
	{
		return placement.location_to_world(_imo->get_ai()->get_target_placement_os_for(enemyPlacement.get_target()).get_translation());
	}
	else
	{
		return placement.location_to_world(enemyTargetingOffsetOS);
	}
}

Vector3 Targetting::get_enemy_centre_of_presence_in_owner_room(Framework::IModulesOwner* _imo)
{
	ACCESS_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	ACCESS_VAR(Vector3, enemyCentreOffsetOS);

	if (!enemyPlacement.is_active())
	{
		warn(TXT("no enemy!"));
		return _imo->get_presence()->get_placement().get_translation();
	}
	Transform placement = enemyPlacement.get_placement_in_owner_room();

	if (enemyPlacement.get_target())
	{
		return placement.location_to_world(enemyPlacement.get_target_presence()->get_centre_of_presence_os().get_translation());
	}
	else
	{
		return placement.location_to_world(enemyCentreOffsetOS);
	}
}

Vector3 Targetting::get_enemy_targetting_offset_os(Framework::IModulesOwner* _imo)
{
	ACCESS_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	ACCESS_VAR(Vector3, enemyTargetingOffsetOS);

	if (!enemyPlacement.is_active())
	{
		warn(TXT("no enemy!"));
		return Vector3::zero;
	}

	if (enemyPlacement.get_target() && _imo->get_ai())
	{
		return _imo->get_ai()->get_target_placement_os_for(enemyPlacement.get_target()).get_translation();
	}
	else
	{
		return enemyTargetingOffsetOS;
	}
}

Vector3 Targetting::get_enemy_centre_of_presence_offset_os(Framework::IModulesOwner* _imo)
{
	ACCESS_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	ACCESS_VAR(Vector3, enemyCentreOffsetOS);

	if (!enemyPlacement.is_active())
	{
		warn(TXT("no enemy!"));
		return Vector3::zero;
	}

	if (enemyPlacement.get_target())
	{
		return enemyPlacement.get_target_presence()->get_centre_of_presence_os().get_translation();
	}
	else
	{
		return enemyCentreOffsetOS;
	}
}




