#include "customUpgradeType.h"

#include "library.h"

#include "..\game\game.h"
#include "..\game\persistence.h"

#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\gameplay\moduleEXM.h"

#include "..\pilgrim\pilgrimOverlayInfo.h"

#include "..\tutorials\tutorialHubId.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\object\actor.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_a_armouryEnlarged, TXT("armoury; enlarged"));
DEFINE_STATIC_NAME_STR(aim_mgp_unlocked, TXT("mission general progress; unlocked"));
DEFINE_STATIC_NAME_STR(aim_a_weaponBought, TXT("armoury; weapon bought"));

//

REGISTER_FOR_FAST_CAST(CustomUpgradeType);
LIBRARY_STORED_DEFINE_TYPE(CustomUpgradeType, customUpgradeType);

CustomUpgradeType::CustomUpgradeType(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

CustomUpgradeType::~CustomUpgradeType()
{
}

bool CustomUpgradeType::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	uiFullName.load_from_xml_child(_node, TXT("uiFullName"), _lc, TXT("ui full name"));
	uiDesc.load_from_xml_child(_node, TXT("uiDesc"), _lc, TXT("ui desc"));

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("\"id\" is required for custom upgrade type"));

	order = _node->get_int_attribute(TXT("order"), order);

	if (auto* attr = _node->get_attribute(TXT("upgradeType")))
	{
		if (attr->get_as_string() == TXT("armoury enlargement")) upgradeType = UpgradeType::ArmouryEnlargement; else 
		if (attr->get_as_string() == TXT("mission general progress")) upgradeType = UpgradeType::MissionGeneralProgress; else
		if (attr->get_as_string() == TXT("weapon for armoury")) upgradeType = UpgradeType::WeaponForArmoury; else
		{
			error_loading_xml(_node, TXT("upgrade type \"%S\" no recognised"), attr->get_as_string().to_char());
			result = false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("no \"upgradeType\" provided"));
		result = false;
	}

	unlockLimit = _node->get_int_attribute_or_from_child(TXT("unlockLimit"), unlockLimit);

	forMissionsDefinitionTagged.load_from_xml_attribute_or_child_node(_node, TXT("forMissionsDefinitionTagged"));

	result &= lineModel.load_from_xml(_node, TXT("lineModel"), _lc);
	result &= lineModelForDisplay.load_from_xml(_node, TXT("lineModelForDisplay"), _lc);
	result &= outerLineModel.load_from_xml(_node, TXT("outerLineModel"), _lc);

	for_every(n, _node->children_named(TXT("costMeritPoints")))
	{
		costMeritPoints.flat = n->get_int_attribute(TXT("flat"), costMeritPoints.flat);
		costMeritPoints.linear = n->get_int_attribute(TXT("linear"), costMeritPoints.linear);
		costMeritPoints.square = n->get_int_attribute(TXT("square"), costMeritPoints.square);
		costMeritPoints.limit = n->get_int_attribute(TXT("limit"), costMeritPoints.limit);
	}
	
	result &= missionGeneralProgress.load_from_xml(_node, TXT("missionGeneralProgress"), _lc);
	
	for_every(containerNode, _node->children_named(TXT("weaponSetupTemplate")))
	{
		result &= weaponSetupTemplate.load_from_xml(containerNode);
	}

	useWeaponPartTypesTagged.load_from_xml_attribute_or_child_node(_node, TXT("useWeaponPartTypesTagged"));

	return result;
}

bool CustomUpgradeType::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	
	result &= lineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= lineModelForDisplay.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= outerLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= missionGeneralProgress.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= weaponSetupTemplate.resolve_library_links();

	return result;
}

CustomUpgradeType const* CustomUpgradeType::find(Name const& _id)
{
	CustomUpgradeType const* best = nullptr;
	Library::get_current_as<Library>()->get_custom_upgrade_types().do_for_every(
		[&best, _id](Framework::LibraryStored* _libraryStored)
		{
			if (auto const* customUpgradeType = fast_cast<CustomUpgradeType>(_libraryStored))
			{
				if (customUpgradeType->id == _id)
				{
					// priority system was here
					if (!best)
					{
						best = customUpgradeType;
					}
				}
			}
		});
	return best;
}

void CustomUpgradeType::get_all(OUT_ Array< CustomUpgradeType const*>& _all)
{
	_all.clear();
	Library::get_current_as<Library>()->get_custom_upgrade_types().do_for_every(
		[&_all](Framework::LibraryStored* _libraryStored)
		{
			if (auto const* customUpgradeType = fast_cast<CustomUpgradeType>(_libraryStored))
			{
				_all.push_back(customUpgradeType);
			}
		});
	struct Utils
	{
		static int compare(void const* _a, void const* _b)
		{
			CustomUpgradeType const * a = *plain_cast<CustomUpgradeType*>(_a);
			CustomUpgradeType const * b = *plain_cast<CustomUpgradeType*>(_b);
			if (a->get_order() < b->get_order()) return B_BEFORE_A;
			if (a->get_order() > b->get_order()) return A_BEFORE_B;
			return String::compare_tchar_case_sort(a->get_id().to_char(), b->get_id().to_char());
		}
	};
	sort(_all, Utils::compare);
}

Energy CustomUpgradeType::calculate_unlock_xp_cost() const
{
	return Energy::zero();
}

int CustomUpgradeType::calculate_unlock_merit_points_cost() const
{
	if (upgradeType == MissionGeneralProgress)
	{
		if (auto* mgp = missionGeneralProgress.get())
		{
			return mgp->get_unlock_cost();
		}
	}

	int cost = costMeritPoints.flat;
	if (costMeritPoints.linear != 0 ||
		costMeritPoints.square != 0)
	{
		int unlocked = get_unlocked_count();
		if (cost == 0) unlocked += 1;
		cost += costMeritPoints.linear * unlocked;
		cost += costMeritPoints.square * unlocked * unlocked;
	}
	if (costMeritPoints.limit != 0)
	{
		cost = min(cost, costMeritPoints.limit);
	}
	return cost;
}

bool CustomUpgradeType::process_unlock() const
{
	if (!is_available_to_unlock())
	{
		return false;
	}
	if (upgradeType == UpgradeType::ArmouryEnlargement)
	{
		if (auto* p = Persistence::access_current_if_exists())
		{
			Concurrency::ScopedSpinLock lock(p->access_lock(), true);
			int unlocked = get_unlocked_count();
			++unlocked;
			p->set_weapons_in_armoury_columns(unlocked + p->get_base_weapons_in_armoury_columns());
			if (auto* w = Game::get()->get_world())
			{
				if (auto* aim = w->create_ai_message(NAME(aim_a_armouryEnlarged)))
				{
					// send it everywhere
					aim->to_world(w);
				}
			}
			return true;
		}
	}
	else if (upgradeType == UpgradeType::MissionGeneralProgress)
	{
		if (auto* p = Persistence::access_current_if_exists())
		{
			if (auto* mgp = missionGeneralProgress.get())
			{
				Concurrency::ScopedSpinLock lock(p->access_lock(), true);
				if (p->unlock_mission_general_progress(mgp))
				{
					if (auto* w = Game::get()->get_world())
					{
						if (auto* aim = w->create_ai_message(NAME(aim_mgp_unlocked)))
						{
							// send it everywhere
							aim->to_world(w);
						}
					}
					return true;
				}
			}
		}
	}
	else if (upgradeType == UpgradeType::WeaponForArmoury)
	{
		if (auto* p = Persistence::access_current_if_exists())
		{
			Concurrency::ScopedSpinLock lock(p->access_lock(), true, TXT("add weapon from unlock"));
			// no weapons available, add one, if not empty
			WeaponSetup addWeapon(p);
			addWeapon.setup_with_template(weaponSetupTemplate);
			Array<WeaponPartType const*> usingWeaponParts;
			WeaponPartType::get_parts_tagged(useWeaponPartTypesTagged, OUT_ usingWeaponParts);
			addWeapon.fill_with_random_parts_at(WeaponPartAddress(), true, &usingWeaponParts);
			addWeapon.resolve_library_links();
			p->add_weapon_to_armoury(addWeapon);

			if (auto* w = Game::get()->get_world())
			{
				if (auto* aim = w->create_ai_message(NAME(aim_a_weaponBought)))
				{
					// send it everywhere
					aim->to_world(w);
				}
			}

			return true;
		}
	}
	else
	{
		todo_implement
	}
	return false;
}

int CustomUpgradeType::get_unlocked_count() const
{
	if (upgradeType == UpgradeType::ArmouryEnlargement)
	{
		if (auto* p = Persistence::access_current_if_exists())
		{
			Concurrency::ScopedSpinLock lock(p->access_lock(), true);
			return max(0, (p->get_weapons_in_armoury_columns() - p->get_base_weapons_in_armoury_columns()));
		}
	}
	else if (upgradeType == UpgradeType::MissionGeneralProgress)
	{
		if (auto* p = Persistence::access_current_if_exists())
		{
			if (p->is_mission_general_progress_unlocked(missionGeneralProgress.get()))
			{
				return 1;
			}
			return 0;
		}
	}
	else if (upgradeType == UpgradeType::WeaponForArmoury)
	{
		if (auto* p = Persistence::access_current_if_exists())
		{
			int existing = 0;
			WeaponPartType* lookForWeaponPartType = nullptr;
			// find chassis in template
			{
				for_every(part, weaponSetupTemplate.get_parts())
				{
					if (auto* wpt = part->weaponPartType.get())
					{
						if (wpt->get_type() == WeaponPartType::GunChassis)
						{
							lookForWeaponPartType = wpt;
							break;
						}
					}
				}
			}
			if (lookForWeaponPartType)
			{
				Concurrency::ScopedSpinLock lock(p->access_lock(), true, TXT("check existing weapons"));
				for_every(wia, p->get_weapons_in_armoury())
				{
					for_every(part, wia->setup.get_parts())
					{
						if (auto* wp = part->part.get())
						{
							if (wp->get_weapon_part_type() == lookForWeaponPartType)
							{
								++existing;
								break; // to next weapon
							}
						}
					}
				}
			}
			return existing;
		}
	}
	else
	{
		todo_implement
	}
	
	return 0;
}

bool CustomUpgradeType::is_available_to_unlock(OPTIONAL_ UnlockableCustomUpgradesContext const* _context) const
{
	if (!forMissionsDefinitionTagged.is_empty())
	{
		bool ok = false;
		if (auto* gs = PlayerSetup::get_current().get_active_game_slot())
		{
			if (auto* md = gs->get_missions_definition())
			{
				ok = forMissionsDefinitionTagged.check(md->get_tags());
			}
		}
		if (!ok)
		{
			return false;
		}
	}
	if (unlockLimit > 0 && get_unlocked_count() >= unlockLimit)
	{
		return false;
	}
	{
		if (upgradeType == UpgradeType::ArmouryEnlargement)
		{
			return true;
		}
		else if (upgradeType == UpgradeType::MissionGeneralProgress)
		{
			if (auto* mgp = missionGeneralProgress.get())
			{
				return mgp->is_available_to_unlock(_context);
			}
			return false;
		}
		else if (upgradeType == UpgradeType::WeaponForArmoury)
		{
			// check if we have space to add a weapon
			if (auto* p = Persistence::access_current_if_exists())
			{
				if (p->get_weapons_in_armoury().get_size() >= p->get_max_weapons_in_armoury())
				{
					return false;
				}
			}
			return true;
		}
		else
		{
			todo_implement
		}
	}
	return false;
}
