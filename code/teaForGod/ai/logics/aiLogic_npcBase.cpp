#include "aiLogic_npcBase.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\moduleTemporaryObjectsData.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// default values
DEFINE_STATIC_NAME(perception);
DEFINE_STATIC_NAME_STR(secondaryPerception, TXT("secondary perception"));

// variable names
DEFINE_STATIC_NAME(omniscient);
DEFINE_STATIC_NAME(perceptionSocket);
DEFINE_STATIC_NAME(secondaryPerceptionSocket);
DEFINE_STATIC_NAME(alwaysLookForNewEnemy);
DEFINE_STATIC_NAME(perceptionFOV);
DEFINE_STATIC_NAME(perceptionVerticalFOV);
DEFINE_STATIC_NAME(perceptionThinkingTime);
DEFINE_STATIC_NAME(perceptionThinkingTimeMul);
DEFINE_STATIC_NAME(relatedDeviceId);
DEFINE_STATIC_NAME(relatedDeviceIdVar);
DEFINE_STATIC_NAME(scanningFOV);
DEFINE_STATIC_NAME(scanningSpeed);
DEFINE_STATIC_NAME(maxAttackDistance);
DEFINE_STATIC_NAME(stayInRoom);
DEFINE_STATIC_NAME(aiAggressive);
DEFINE_STATIC_NAME(combatMusicIndicatePresenceDistance);
DEFINE_STATIC_NAME(shouldWanderForward);
DEFINE_STATIC_NAME(perceptionGlanceChance);

DEFINE_STATIC_NAME(storeExistenceInPilgrimage);
DEFINE_STATIC_NAME(storeExistenceInPilgrimageWorldAddress);
DEFINE_STATIC_NAME(storeExistenceInPilgrimagePOIIdx);

// ai messages
DEFINE_STATIC_NAME(enemyPlacement);
	DEFINE_STATIC_NAME(who);
	DEFINE_STATIC_NAME(enemy);
	DEFINE_STATIC_NAME(room);
	DEFINE_STATIC_NAME(placement);

// temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);

// temporary objects data tags
DEFINE_STATIC_NAME(projectileShootIdx);
DEFINE_STATIC_NAME(projectileMuzzleFlashIdx);
DEFINE_STATIC_NAME(projectileShootForNumberedSockets);
DEFINE_STATIC_NAME(projectileMuzzleFlashForNumberedSockets);

//

REGISTER_FOR_FAST_CAST(NPCBase);

NPCBase::NPCBase(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction))
: base(_mind, _logicData, _executeFunction)
{
	perceptionSocket = NAME(perception);
	secondaryPerceptionSocket = NAME(secondaryPerception);

	shotInfos.push_back(ShotInfo(NAME(shoot), NAME(muzzleFlash)));

	firstAdvance = true;
}

NPCBase::~NPCBase()
{
}

void NPCBase::learn_from(SimpleVariableStorage& _parameters)
{
	an_assert(get_mind()->get_owner_as_modules_owner()->get_appearance());
	omniscient = _parameters.get_value<bool>(NAME(omniscient), omniscient);
	alwaysLookForNewEnemy = _parameters.get_value<bool>(NAME(alwaysLookForNewEnemy), alwaysLookForNewEnemy);
	perceptionSocket = _parameters.get_value<Name>(NAME(perceptionSocket), perceptionSocket);
	secondaryPerceptionSocket = _parameters.get_value<Name>(NAME(secondaryPerceptionSocket), secondaryPerceptionSocket);
	perceptionSocketIdx = get_mind()->get_owner_as_modules_owner()->get_appearance()->find_socket_index(perceptionSocket);
	secondaryPerceptionSocketIdx = get_mind()->get_owner_as_modules_owner()->get_appearance()->find_socket_index(secondaryPerceptionSocket);

	if (perceptionSocket.is_valid() && perceptionSocketIdx == NONE)
	{
		warn(TXT("perception socket defined as \"%S\" but not found"), perceptionSocket.to_char());
	}

	shouldWanderForward = _parameters.get_value<bool>(NAME(shouldWanderForward), shouldWanderForward);
	glanceChance = _parameters.get_value<float>(NAME(perceptionGlanceChance), glanceChance);

	// check if exists and override then
	if (auto* v = _parameters.get_existing<float>(NAME(perceptionFOV)))
	{
		perceptionFOV = Range(-*v * 0.5f, *v * 0.5f);
	}
	if (auto* v = _parameters.get_existing<Range>(NAME(perceptionFOV)))
	{
		perceptionFOV = Range(*v);
	}
	if (auto* v = _parameters.get_existing<float>(NAME(perceptionVerticalFOV)))
	{
		perceptionVerticalFOV = Range(-*v * 0.5f, *v * 0.5f);
	}
	if (auto* v = _parameters.get_existing<Range>(NAME(perceptionVerticalFOV)))
	{
		perceptionVerticalFOV = Range(*v);
	}

	perceptionThinkingTime = _parameters.get_value<Range>(NAME(perceptionThinkingTime), perceptionThinkingTime);
	{
		float perceptionThinkingTimeMul = 1.0f;
		perceptionThinkingTimeMul = _parameters.get_value<float>(NAME(perceptionThinkingTimeMul), perceptionThinkingTimeMul);
		perceptionThinkingTime *= perceptionThinkingTimeMul;
	}

	scanningFOV = _parameters.get_value<float>(NAME(scanningFOV), scanningFOV);
	scanningSpeed = _parameters.get_value<float>(NAME(scanningSpeed), scanningSpeed);
	maxAttackDistance = _parameters.get_value<float>(NAME(maxAttackDistance), maxAttackDistance.get(0.0f));
	if (maxAttackDistance.get() == 0.0f)
	{
		maxAttackDistance.clear();
	}

	relatedDeviceId = _parameters.get_value<int>(NAME(relatedDeviceId), relatedDeviceId);
	relatedDeviceIdVar = _parameters.get_value<Name>(NAME(relatedDeviceIdVar), relatedDeviceIdVar);

	stayInRoom = _parameters.get_value<bool>(NAME(stayInRoom), stayInRoom);
	aiAggressive = _parameters.get_value<bool>(NAME(aiAggressive), aiAggressive);

	combatMusicIndicatePresenceDistance = _parameters.get_value<float>(NAME(combatMusicIndicatePresenceDistance), combatMusicIndicatePresenceDistance);
}

void NPCBase::auto_fill_shot_infos()
{
	shotInfos.clear();
	if (auto* to = get_mind()->get_owner_as_modules_owner()->get_temporary_objects())
	{
		if (auto* tod = fast_cast<Framework::ModuleTemporaryObjectsData>(to->get_module_data()))
		{
			Name projectileShootForNumberedSockets;
			Name projectileMuzzleFlashForNumberedSockets;
			for_every_ref(tosd, tod->get_temporary_objects())
			{
				if (tosd->tags.has_tag(NAME(projectileShootIdx)))
				{
					int idx = tosd->tags.get_tag_as_int(NAME(projectileShootIdx));
					shotInfos.set_size(max(shotInfos.get_size(), idx + 1));
					shotInfos[idx].projectile = tosd->id;
				}
				if (tosd->tags.has_tag(NAME(projectileMuzzleFlashIdx)))
				{
					int idx = tosd->tags.get_tag_as_int(NAME(projectileMuzzleFlashIdx));
					shotInfos.set_size(max(shotInfos.get_size(), idx + 1));
					shotInfos[idx].muzzleFlash = tosd->id;
				}
				if (tosd->tags.has_tag(NAME(projectileShootForNumberedSockets)))
				{
					projectileShootForNumberedSockets = tosd->id;
				}
				if (tosd->tags.has_tag(NAME(projectileMuzzleFlashForNumberedSockets)))
				{
					projectileMuzzleFlashForNumberedSockets = tosd->id;
				}
			}
			if (projectileShootForNumberedSockets.is_valid())
			{
				if (auto* imo = get_mind()->get_owner_as_modules_owner())
				{
					if (auto* a = imo->get_appearance())
					{
						for (int idx = 0;; ++idx)
						{
							Name muzzleSocket = Name(String::printf(TXT("muzzle_%i"), idx));
							if (a->has_socket(muzzleSocket))
							{
								shotInfos.push_back();
								auto& si = shotInfos.get_last();
								si.projectile = projectileShootForNumberedSockets;
								si.muzzleFlash = projectileMuzzleFlashForNumberedSockets;
								si.socket = muzzleSocket;
							}
							else
							{
								break;
							}
						}
					}
				}
			}
			for_every(si, shotInfos)
			{
				if (!si->sound.is_valid())
				{
					si->sound = NAME(shoot);
				}
			}
		}
	}
}

void NPCBase::advance(float _deltaTime)
{
	if (firstAdvance)
	{
		firstAdvance = false;
		if (auto* imo = get_mind()->get_owner_as_modules_owner())
		{
			if (imo->get_variables().get_value<bool>(NAME(storeExistenceInPilgrimage), false))
			{
				if (auto* piow = PilgrimageInstance::get())
				{
					auto* wa = imo->get_variables().get_existing<Framework::WorldAddress>(NAME(storeExistenceInPilgrimageWorldAddress));
					auto* waPOIIdx = imo->get_variables().get_existing<int>(NAME(storeExistenceInPilgrimagePOIIdx));
					if (wa && waPOIIdx)
					{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
						output(TXT("npc base: check address=\"%S\" poiIdx=\"%i\""), wa->to_string().to_char(), *waPOIIdx);
						output(TXT("check in pilgrimage : %S"), piow->has_been_killed(*wa, *waPOIIdx) ? TXT("killed") : TXT("ok"));
#endif
						if (piow->has_been_killed(*wa, *waPOIIdx))
						{
							imo->cease_to_exist(false);
							return;
						}
					}
				}
			}
		}
	}

	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	if ((stayInRoom && ! toStayInRoom.is_active()) ||
		(!stayInRoom && toStayInRoom.is_active()))
	{
		reset_to_stay_in_room();
	}

	base::advance(_deltaTime);
}

void NPCBase::reset_to_stay_in_room()
{
	if (stayInRoom)
	{
		auto * imo = get_mind()->get_owner_as_modules_owner();
		auto * presence = imo->get_presence();
		toStayInRoom.find_path(imo, presence->get_in_room(), presence->get_placement());
	}
	else
	{
		toStayInRoom.clear_target();
	}
}

void NPCBase::give_out_enemy_location(Framework::RelativeToPresencePlacement const& _pathToEnemy)
{
	give_out_enemy_location(_pathToEnemy.get_target(), _pathToEnemy.get_in_final_room(), _pathToEnemy.get_placement_in_final_room());
}

void NPCBase::give_out_enemy_location(Framework::IModulesOwner* enemy, Framework::Room* inRoom, Transform const & _placement)
{
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(enemyPlacement)))
			{
				ai_log(mind->get_logic(), TXT("inform about our plans"));
				message->to_room(imo->get_presence()->get_in_room());
				message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = imo;
				message->access_param(NAME(enemy)).access_as<SafePtr<Framework::IModulesOwner>>() = enemy;
				message->access_param(NAME(room)).access_as<SafePtr<Framework::Room>>() = inRoom;
				message->access_param(NAME(placement)).access_as<Transform>() = _placement;
			}
		}
	}
}

bool NPCBase::is_ok_to_play_combat_music(Framework::RelativeToPresencePlacement const& _rtpp) const
{
	bool okForCombatMusic = true;
	{
		float cmidDistance = get_combat_music_indicate_presence_distance();
		if (cmidDistance > 0.0f)
		{
			if (_rtpp.calculate_string_pulled_distance() > cmidDistance)
			{
				okForCombatMusic = false;
			}
		}
	}
	return okForCombatMusic;
}

bool NPCBase::is_ok_to_play_combat_music(Framework::PresencePath const& _pp) const
{
	bool okForCombatMusic = true;
	{
		float cmidDistance = get_combat_music_indicate_presence_distance();
		if (cmidDistance > 0.0f)
		{
			if (_pp.calculate_string_pulled_distance() > cmidDistance)
			{
				okForCombatMusic = false;
			}
		}
	}
	return okForCombatMusic;
}
