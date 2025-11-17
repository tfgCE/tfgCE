#include "pilgrimGear.h"

#include "..\game\game.h"
#include "..\game\gameStateSensitive.h"
#include "..\library\library.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\gameplay\equipment\me_gun.h"

#include "..\..\core\tags\tag.h"

#include "..\..\framework\debug\previewGame.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\moduleCustom.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\object\item.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define INSPECT_GEAR_CREATION
#endif

REMOVE_AS_SOON_AS_POSSIBLE_
#ifndef INSPECT_GEAR_CREATION
#define INSPECT_GEAR_CREATION
#endif

//

using namespace TeaForGodEmperor;

//

// global references
DEFINE_STATIC_NAME_STR(grWeaponItemTypeToUseWithWeaponParts, TXT("weapon item type to use with weapon parts"));

// mesh generator parameters / variables
DEFINE_STATIC_NAME_STR(withLeftRestPoint, TXT("with left rest point"));
DEFINE_STATIC_NAME_STR(withRightRestPoint, TXT("with right rest point"));

//

void PilgrimGear::clear()
{
	permanentEXMs.clear();
	unlockedEXMs.clear();
	unlockAllEXMs = false;

	hands[Hand::Left].clear();
	hands[Hand::Right].clear();

	for_count(int, i, MAX_POCKETS)
	{
		pockets[i].clear();
	}
}

bool PilgrimGear::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	clear();

	for_every(node, _node->children_named(TXT("permanentEXMs")))
	{
		for_every(subNode, node->children_named(TXT("permanent")))
		{
			Name exm = subNode->get_name_attribute(TXT("exm"));
			if (exm.is_valid())
			{
				permanentEXMs.push_back(exm);
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(containerNode, _node->children_named(TXT("unlockedEXMs")))
	{
		unlockAllEXMs = containerNode->get_bool_attribute_or_from_child_presence(TXT("all"), unlockAllEXMs);
		for_every(node, containerNode->children_named(TXT("unlocked")))
		{
			Name exm = node->get_name_attribute(TXT("exm"));
			if (exm.is_valid())
			{
				unlockedEXMs.push_back(exm);
			}
		}
	}

	for_every(ielNode, _node->children_named(TXT("energyLevels")))
	{
		result &= initialEnergyLevels.load_from_xml(ielNode);
	}
	
	{
		// load functions will auto clear
		result &= hands[Hand::Left].load_from_xml(_node->first_child_named(TXT("leftHand")));
		result &= hands[Hand::Right].load_from_xml(_node->first_child_named(TXT("rightHand")));
	}

	{
		int pocketIdx = 0;
		for_every(pocketsNode, _node->children_named(TXT("pockets")))
		{
			for_every(pocketNode, pocketsNode->children_named(TXT("pocket")))
			{
				if (pocketIdx < MAX_POCKETS)
				{
					result &= pockets[pocketIdx].load_from_xml(pocketNode);
					++pocketIdx;
				}
				else
				{
					error_loading_xml(pocketNode, TXT("increase MAX_POCKETS"));
					result = false;
				}
			}
		}
	}

	return result;
}

bool PilgrimGear::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	if (auto* contn = _node->add_node(TXT("permanentEXMs")))
	{
		for_every(exm, permanentEXMs)
		{
			if (auto* n = contn->add_node(TXT("permanent")))
			{
				n->set_attribute(TXT("exm"), exm->to_char());
			}
		}
	}

	if (auto* contn = _node->add_node(TXT("unlockedEXMs")))
	{
		if (unlockAllEXMs)
		{
			contn->add_node(TXT("all"));
		}
		for_every(exm, unlockedEXMs)
		{
			if (auto* n = contn->add_node(TXT("unlocked")))
			{
				n->set_attribute(TXT("exm"), exm->to_char());
			}
		}
	}

	if (!initialEnergyLevels.health.is_negative() &&
		!initialEnergyLevels.hand[Hand::Left].is_negative() &&
		!initialEnergyLevels.hand[Hand::Right].is_negative())
	{
		if (auto* n = _node->add_node(TXT("energyLevels")))
		{
			initialEnergyLevels.save_to_xml(n);
		}
	}
	
	{
		// load functions will auto clear
		result &= hands[Hand::Left].save_to_xml(_node->add_node(TXT("leftHand")));
		result &= hands[Hand::Right].save_to_xml(_node->add_node(TXT("rightHand")));
	}

	if(auto* pocketsNode = _node->add_node(TXT("pockets")))
	{
		for_count(int, pocketIdx, MAX_POCKETS)
		{
			if (auto* n = pocketsNode->add_node(TXT("pocket")))
			{
				result &= pockets[pocketIdx].save_to_xml(n);
			}
		}
	}

	return result;
}

bool PilgrimGear::resolve_library_links()
{
	bool result = true;

	for_count(int, hIdx, Hand::MAX)
	{
		result &= hands[hIdx].resolve_library_links();
	}

	for_count(int, i, MAX_POCKETS)
	{
		result &= pockets[i].resolve_library_links();
	}

	if (unlockAllEXMs)
	{
		Array<Name> allEXMs = EXMType::get_all();
		for_every(exm, allEXMs)
		{
			unlockedEXMs.push_back_unique(*exm);
		}
	}

	return result;
}

//

PilgrimGear::HandSetup::HandSetup()
: weaponSetup(nullptr)
{
	SET_EXTRA_DEBUG_INFO(passiveEXMs, TXT("PilgrimGear::HandSetup.passiveEXMs"));
}

void PilgrimGear::HandSetup::clear()
{
	passiveEXMs.clear();
	activeEXM.clear();
	inHand.clear();
	weaponSetup.clear();
	weaponSetupTemplate.clear();
}

bool PilgrimGear::HandSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	clear();

	if (!_node)
	{
		return result;
	}

	for_every(node, _node->children_named(TXT("exms")))
	{
		for_every(subNode, node->children_named(TXT("passive")))
		{
			PilgrimHandEXMSetup exm;
			if (exm.load_from_xml(subNode))
			{
				passiveEXMs.push_back(exm);
			}
			else
			{
				result = false;
			}
		}
		for_every(subNode, node->children_named(TXT("active")))
		{
			PilgrimHandEXMSetup exm;
			if (exm.load_from_xml(subNode))
			{
				activeEXM = exm;
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(containerNode, _node->children_named(TXT("weaponSetup")))
	{
		result &= weaponSetup.load_from_xml(containerNode);
	}

	for_every(containerNode, _node->children_named(TXT("weaponSetupTemplate")))
	{
		result &= weaponSetupTemplate.load_from_xml(containerNode);
	}

	for_every(containerNode, _node->children_named(TXT("inHand")))
	{
		result &= inHand.load_from_xml(containerNode);
	}

	return result;
}

bool PilgrimGear::HandSetup::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	if (auto* contn = _node->add_node(TXT("exms")))
	{
		if (activeEXM.is_set())
		{
			if (auto* n = contn->add_node(TXT("active")))
			{
				activeEXM.save_to_xml(n);
			}
		}
		for_every(pexm, passiveEXMs)
		{
			if (pexm->is_set())
			{
				if (auto* n = contn->add_node(TXT("passive")))
				{
					pexm->save_to_xml(n);
				}
			}
		}
	}

	if (!weaponSetup.is_empty())
	{
		if (auto* n = _node->add_node(TXT("weaponSetup")))
		{
			weaponSetup.save_to_xml(n);
		}
	}

	if (!weaponSetupTemplate.is_empty())
	{
		if (auto* n = _node->add_node(TXT("weaponSetupTemplate")))
		{
			weaponSetupTemplate.save_to_xml(n);
		}
	}

	if (inHand.is_used())
	{
		if (auto* n = _node->add_node(TXT("inHand")))
		{
			result &= inHand.save_to_xml(n);
		}
	}

	return result;
}

bool PilgrimGear::HandSetup::resolve_library_links()
{
	bool result = true;
	
	result &= weaponSetupTemplate.resolve_library_links();
	result &= inHand.resolve_library_links();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		result = true;
	}
#endif

	return result;
}

//

PilgrimGear::ItemSetup::ItemSetup()
: weaponSetup(nullptr)
{
	individualRandomGenerator.be_zero_seed();
}

void PilgrimGear::ItemSetup::clear()
{
	individualRandomGenerator.be_zero_seed();
	weaponSetup.clear();
	weaponSetupTemplate.clear();
	itemType.clear();
	actorType.clear();
	variables.clear();

}

bool PilgrimGear::ItemSetup::is_used() const
{
	return !weaponSetup.is_empty() || !weaponSetupTemplate.is_empty() || !itemType.get() || !actorType.get();
}

bool PilgrimGear::ItemSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	clear();

	for_every(node, _node->children_named(TXT("individualRandomGenerator")))
	{
		Optional<int> seedBase;
		Optional<int> seedOffset;
		seedBase.load_from_xml(node, TXT("base"));
		seedOffset.load_from_xml(node, TXT("offset"));
		if (seedBase.is_set())
		{
			individualRandomGenerator.set_seed(seedBase.get(), seedOffset.get(0));
		}
	}

	for_every(containerNode, _node->children_named(TXT("weaponSetup")))
	{
		result &= weaponSetup.load_from_xml(containerNode);
	}

	for_every(containerNode, _node->children_named(TXT("weaponSetupTemplate")))
	{
		result &= weaponSetupTemplate.load_from_xml(containerNode);
	}

	for_every(containerNode, _node->children_named(TXT("object")))
	{
		result &= itemType.load_from_xml(containerNode, TXT("itemType"));
		result &= actorType.load_from_xml(containerNode, TXT("actorType"));
		variables.load_from_xml_child_node(containerNode, TXT("variables"));
	}

	mainWeapon = _node->get_bool_attribute_or_from_child_presence(TXT("mainWeapon"), false);

	return result;
}

bool PilgrimGear::ItemSetup::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;

	if (!individualRandomGenerator.is_zero_seed())
	{
		if (auto* n = _node->add_node(TXT("individualRandomGenerator")))
		{
			n->set_int_attribute(TXT("base"), individualRandomGenerator.get_seed_hi_number());
			if (individualRandomGenerator.get_seed_lo_number() != 0)
			{
				n->set_int_attribute(TXT("offset"), individualRandomGenerator.get_seed_lo_number());
			}
		}
	}

	if (mainWeapon)
	{
		_node->add_node(TXT("mainWeapon"));
	}

	if (!weaponSetup.is_empty())
	{
		if (auto* n = _node->add_node(TXT("weaponSetup")))
		{
			weaponSetup.save_to_xml(n);
		}
	}

	if (!weaponSetupTemplate.is_empty())
	{
		if (auto* n = _node->add_node(TXT("weaponSetupTemplate")))
		{
			weaponSetupTemplate.save_to_xml(n);
		}
	}

	if (itemType.is_set() || actorType.is_set())
	{
		if (auto* n = _node->add_node(TXT("object")))
		{
			if (itemType.get())
			{
				n->set_attribute(TXT("itemType"), itemType->get_name().to_string().to_char());
			}
			if (actorType.get())
			{
				n->set_attribute(TXT("actorType"), actorType->get_name().to_string().to_char());
			}
			variables.save_to_xml_child_node(n, TXT("variables"));
		}
	}

	return result;
}

bool PilgrimGear::ItemSetup::resolve_library_links()
{
	bool result = true;

	result &= weaponSetupTemplate.resolve_library_links();
	result &= itemType.find(Library::get_current());
	result &= actorType.find(Library::get_current());

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		result = true;
	}
#endif

	return result;
}

void PilgrimGear::ItemSetup::store_object(Framework::IModulesOwner* _object)
{
	clear();

	if (!_object)
	{
		return;
	}

	MODULE_OWNER_LOCK_FOR_IMO(_object, TXT("store_object"));

	individualRandomGenerator = _object->get_individual_random_generator();

	if (auto* g = _object->get_gameplay_as<ModuleEquipments::Gun>())
	{
		weaponSetup.copy_for_different_persistence(g->get_weapon_setup(), true);
	}
	else
	{
		if (auto* a = _object->get_as_actor())
		{
			actorType = a->get_actor_type();
		}
		if (auto* i = _object->get_as_item())
		{
			itemType = i->get_item_type();
		}
	}

	{
		for_every_ptr(m, _object->get_customs())
		{
			if (auto* gss = fast_cast<IGameStateSensitive>(m))
			{
				gss->store_for_game_state(variables);
			}
		}
	}
}

Framework::IModulesOwner* PilgrimGear::ItemSetup::async_create_object(Framework::IModulesOwner* ownerAsObject) const
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	an_assert(ownerAsObject);

	Framework::Object* gearObject = nullptr;
	if (is_used())
	{
		Framework::ActorType* useActorType = actorType.get();
		Framework::ItemType* useItemType = itemType.get();
		bool isWeapon = false;
		if (!useItemType && (! weaponSetup.is_empty() || ! weaponSetupTemplate.is_empty()))
		{
			isWeapon = true;
			useItemType = Library::get_current()->get_global_references().get<Framework::ItemType>(NAME(grWeaponItemTypeToUseWithWeaponParts));
		}

		if (useItemType || useActorType)
		{
#ifdef INSPECT_GEAR_CREATION
			output(TXT("create gear object \"%S\""), useItemType? useItemType->get_name().to_string().to_char() : useActorType->get_name().to_string().to_char());
#endif

			an_assert(Persistence::access_current_if_exists());
			auto* persistence = Persistence::access_current_if_exists();
			WeaponSetup useWeaponSetup(persistence);
			if (! weaponSetup.is_empty())
			{
				isWeapon = true;
				useWeaponSetup.copy_for_different_persistence(weaponSetup);
			}
			else if (! weaponSetupTemplate.is_empty())
			{
				Random::Generator weaponPartRG = ownerAsObject->get_individual_random_generator();
				weaponPartRG.advance_seed(975379, 297234);
				isWeapon = true;
				for_every(wp, weaponSetupTemplate.get_parts())
				{
					auto* wpi = wp->create_weapon_part(persistence, weaponPartRG.get_int());
					useWeaponSetup.add_part(wp->at, wpi, wp->nonRandomised, wp->damaged, wp->defaultMeshParams);
				}
			}

			{
				for_every(part, useWeaponSetup.get_parts())
				{
					if (auto* p = part->part.get())
					{
						p->make_sure_schematic_mesh_exists();
					}
				}
			}

#ifdef INSPECT_GEAR_CREATION
			output(TXT("create actual gear object"));
#endif
			if (useItemType) useItemType->load_on_demand_if_required();
			if (useActorType) useActorType->load_on_demand_if_required();

			Game::get_as<Game>()->perform_sync_world_job(TXT("create gear object"), [&gearObject, useItemType, useActorType, ownerAsObject]()
				{
					if (useItemType)
					{
						gearObject = useItemType->create(String(TXT("pilgrim's gear")));
					}
					else if (useActorType)
					{
						gearObject = useActorType->create(String(TXT("pilgrim's gear")));
					}
					gearObject->init(ownerAsObject->get_in_sub_world());
				});

			{
				Random::Generator rg = ownerAsObject->get_individual_random_generator();
				if (!individualRandomGenerator.is_zero_seed())
				{
					rg = individualRandomGenerator;
				}
				else
				{
					rg = Random::Generator(); // spawn random one
				}
				gearObject->access_individual_random_generator() = rg;
			}

			if (isWeapon)
			{
				// rest points are also set via gun module
				//equipment->access_variables().access<bool>(_hand == Hand::Left ? NAME(withLeftRestPoint) : NAME(withRightRestPoint)) = true;
				gearObject->access_variables().access<bool>(NAME(withLeftRestPoint)) = true;
				gearObject->access_variables().access<bool>(NAME(withRightRestPoint)) = true;
			}

			{
				if (auto* pi = PilgrimageInstance::get())
				{
					if (auto* p = pi->get_pilgrimage())
					{
						gearObject->access_variables().set_from(p->get_equipment_parameters());
					}
				}

				if (auto* p = ownerAsObject->get_gameplay_as<ModulePilgrim>())
				{
					if (auto* pd = p->get_pilgrim_data())
					{
						gearObject->access_variables().set_from(pd->get_equipment_parameters());
					}
				}
			}

#ifdef INSPECT_GEAR_CREATION
			output(TXT("initialise gear object"));
#endif

			gearObject->initialise_modules([isWeapon, &useWeaponSetup](Framework::Module* _module)
				{
					if (auto* g = fast_cast<ModuleEquipments::Gun>(_module))
					{
						// the stats differ then a bit as we have different storages
						//g->be_pilgrim_weapon();

						if (isWeapon)
						{
							g->setup_with_weapon_setup(useWeaponSetup);
						}
					}
				});

			for_every_ptr(m, gearObject->get_customs())
			{
				if (auto* gss = fast_cast<IGameStateSensitive>(m))
				{
					gss->restore_from_game_state(variables);
				}
			}

#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("gear object created!"));
#endif
		}
	}

	return gearObject;
}

//



