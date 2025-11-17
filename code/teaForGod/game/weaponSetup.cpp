#include "weaponSetup.h"

#include "game.h"
#include "persistence.h"

#include "..\library\library.h"
#include "..\schematics\schematic.h"
#include "..\schematics\schematicLine.h"
#include "..\tutorials\tutorialHubId.h"
#include "..\utils\buildStatsInfo.h"

#include "..\..\framework\debug\previewGame.h"
#include "..\..\framework\library\usedLibraryStored.inl"

#include "..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// obsolete
DEFINE_STATIC_NAME(makesWeaponPartObsolete);
DEFINE_STATIC_NAME(makesWeaponPartObsolete0);
DEFINE_STATIC_NAME(makesWeaponPartObsolete1);
DEFINE_STATIC_NAME(makesWeaponPartObsolete2);

//

void purge_obsolete_weapon_parts(Array<WeaponPartType const*>& _parts)
{
	ArrayStatic<Name, 8> checkParams;
	checkParams.push_back(NAME(makesWeaponPartObsolete));
	checkParams.push_back(NAME(makesWeaponPartObsolete0));
	checkParams.push_back(NAME(makesWeaponPartObsolete1));
	checkParams.push_back(NAME(makesWeaponPartObsolete2));

	for (int i = 0; i < _parts.get_size(); ++i)
	{
		WeaponPartType const* wpi = _parts[i];

		for_every(cp, checkParams)
		{
			if (auto* makeObsoleteName = wpi->get_custom_parameters().get_existing<Framework::LibraryName>(*cp))
			{
				for (int j = 0; j < _parts.get_size(); ++j)
				{
					if (i == j) continue; // we modify i as well
					WeaponPartType const* wpj = _parts[j];
					if (wpj->get_name() == *makeObsoleteName)
					{
						_parts.remove_at(j);
						if (i > j) --i;
						--j;
					}
				}
			}
		}
	}
}

//

WeaponSetup::WeaponSetup(Persistence* _persistence)
: persistence(_persistence)
{

}

void WeaponSetup::setup_with_template(WeaponSetupTemplate const& _template, Persistence* _persistenceForParts)
{
	for_every(wp, _template.get_parts())
	{
		auto* wpi = wp->create_weapon_part(_persistenceForParts? _persistenceForParts : persistence.get());
		add_part(wp->at, wpi, wp->nonRandomised, wp->damaged, wp->defaultMeshParams);
	}
}

WeaponPart* WeaponSetup::get_part_at(WeaponPartAddress const& _address) const
{
	for_every(part, parts)
	{
		if (part->at == _address)
		{
			return part->part.get();
		}
	}
	return nullptr;
}

bool WeaponSetup::load_loose_parts_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (_node->get_bool_attribute_or_from_child_presence(TXT("looseParts"), looseParts.is_set()))
	{
		looseParts.set_if_not_set(0);
	}
	looseParts.load_from_xml(_node, TXT("looseParts"));
	
	return result;
}

bool WeaponSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	result &= load_loose_parts_from_xml(_node);

	for_every(node, _node->children_named(TXT("part")))
	{
		parts.set_size(parts.get_size() + 1);
		if (parts.get_last().load_from_xml(persistence.get(), node))
		{
			// ok
		}
		else
		{
			parts.pop_back();
			result = false;
		}
	}

	itemType.load_from_xml(_node, TXT("itemType"));

	resolve_library_links();

	return result;
}

bool WeaponSetup::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	for_every(part, parts)
	{
		auto* node = _node->add_node(TXT("part"));
		result &= part->save_to_xml(node);
	}

	if (itemType.is_name_valid())
	{
		_node->set_attribute(TXT("itemType"), itemType.get_name().to_string());
	}

	return result;
}

bool WeaponSetup::resolve_library_links()
{
	bool result = true;

	for_every(part, parts)
	{
		result &= part->resolve_library_links();
	}

	if (itemType.is_set())
	{
		itemType.find_may_fail(Library::get_current());
	}

	return result;
}

void WeaponSetup::clear()
{
	parts.clear();
}

void WeaponSetup::clear_for(Persistence* _persistence)
{
	clear();
	persistence = _persistence;
}

void WeaponSetup::copy_from(WeaponSetup const& _other)
{
	clear();
	looseParts = _other.looseParts;
	for_every(part, _other.parts)
	{
		add_part(part->at, part->part.get(), part->part->is_non_randomised(), part->part->get_damaged());
	}
}

void WeaponSetup::copy_for_different_persistence(WeaponSetup const& _other, Optional<bool> const& _forStoring)
{
	clear();
	looseParts = _other.looseParts;
	for_every(part, _other.parts)
	{
		add_part(part->at, part->part->create_copy_for_different_persistence(persistence.get(),NP, NP, NP, _forStoring));
	}
}

void WeaponSetup::swap_with(WeaponSetup & _other)
{
	auto otherParts = _other.parts;
	auto thisParts = parts;
	clear();
	_other.clear();
	swap(looseParts, _other.looseParts);
	for_every(part, otherParts)
	{
		add_part(part->at, part->part.get());
	}
	for_every(part, thisParts)
	{
		_other.add_part(part->at, part->part.get());
	}
}

int WeaponSetup::find_free_loose_part_idx(Optional<int> const& _tryIdx) const
{
	if (_tryIdx.is_set())
	{
		bool exists = false;
		for_every(p, parts)
		{
			if (p->at.get_loose_part_idx() == _tryIdx.get())
			{
				exists = true;
				break;
			}
		}
		if (! exists)
		{
			return _tryIdx.get();
		}
	}

	int freeIdx = parts.get_size();
	for_every(p, parts) // hope we won't get someone who adds and removes two parts interchangebly
	{
		freeIdx = max(freeIdx, p->at.get_loose_part_idx() + 1);
	}

	return freeIdx;
}

bool WeaponSetup::does_contain(WeaponPart const * _part) const
{
	for_every(p, parts)
	{
		if (p->part == _part)
		{
			return true;
		}
	}
	return false;
}

void WeaponSetup::add_part(WeaponPartAddress const& _at, WeaponPart* _part, Optional<bool> const& _nonRandomised, Optional<EnergyCoef> const& _damaged, Optional<bool> const &_defaultMeshParams)
{
	if (!_part)
	{
		return;
	}
	WeaponPartAddress at = _at;
	if (is_loose_parts())
	{
		at.be_loose_part(find_free_loose_part_idx());
	}
	bool nonRandomised = _nonRandomised.get(_part->is_non_randomised());
	EnergyCoef damaged = _damaged.get(EnergyCoef::zero());
	bool defaultMeshParams = _defaultMeshParams.get(_part->should_use_default_mesh_params());
	if (_part->is_non_randomised() == nonRandomised &&
		_part->should_use_default_mesh_params() == defaultMeshParams &&
		persistence == _part->get_persistence())
	{
		parts.push_back(UsedWeaponPart(at, _part));
	}
	else
	{
		if (auto* wp = _part->create_copy_for_different_persistence(persistence.get(), nonRandomised, damaged, defaultMeshParams))
		{
			parts.push_back(UsedWeaponPart(at, wp));
		}
	}
}

void WeaponSetup::remove_part(WeaponPartAddress const& _at)
{
	for_every(part, parts)
	{
		if (part->at == _at)
		{
			parts.remove_at(for_everys_index(part));
			break;
		}
	}
}

void WeaponSetup::remove_part(WeaponPart* _wp)
{
	for (int i = 0; i < parts.get_size(); ++ i)
	{
		auto& part = parts[i];
		if (part.part == _wp)
		{
			parts.remove_at(i);
			--i;
		}
	}
}

void WeaponSetup::add_nonrandomised_part(WeaponPartAddress const & _at, Framework::LibraryName const& _name)
{
	if (! _name.is_valid())
	{
		return;
	}
	if (auto* wp = WeaponPart::create_instance_of(persistence.get(), _name, true))
	{
		WeaponPartAddress at = _at;
		if (is_loose_parts())
		{
			at.be_loose_part(find_free_loose_part_idx());
		}
		parts.push_back(UsedWeaponPart(at, wp));
	}
}

bool WeaponSetup::operator == (WeaponSetup const& _other) const
{
	if (parts.get_size() != _other.parts.get_size())
	{
		return false;
	}
	for_every(part, parts)
	{
		bool exists = false;
		for_every(opart, _other.parts)
		{
			if (part->at == opart->at)
			{
				if (WeaponPart::compare(*part->part.get(), *opart->part.get()))
				{
					exists = true;
					break;
				}
			}
		}
		if (!exists)
		{
			return false;
		}
	}

	return true;
}

bool WeaponSetup::is_valid() const
{
	bool chassisPresent = false;
	bool corePresent = false;
	for_every(part, parts)
	{
		if (auto* wp = part->part.get())
		{
			if (auto* wpt = wp->get_weapon_part_type())
			{
				if (wpt->get_type() == WeaponPartType::GunChassis)
				{
					chassisPresent = true;
				}
				if (wpt->get_type() == WeaponPartType::GunCore)
				{
					corePresent = true;
				}
			}
		}
	}

	return chassisPresent && corePresent;
}

void WeaponSetup::increase_weapon_part_usage()
{
	for_every(part, parts)
	{
		part->part->mark_in_current_use();
	}
}

void WeaponSetup::on_weapon_part_discard(WeaponPart* _wp)
{
	for_every(part, parts)
	{
		if (part->part == _wp)
		{
			if (_wp->get_weapon_part_type() && _wp->get_weapon_part_type()->get_type() == WeaponPartType::GunChassis)
			{
				// no chassis? it should be all clear then
				parts.clear();
			}
			else
			{
				parts.remove_at(for_everys_index(part));
			}
		}
	}
}

void WeaponSetup::fill_with_random_parts_at(WeaponPartAddress const& _addr, Optional<bool> const & _useNonRandomisedParts, Array<WeaponPartType const*> const* _usingWeaponParts)
{
	if (_addr.is_empty())
	{
		if (looseParts.get(0) > 0)
		{
			Random::Generator rg;
			// add loose parts only
			for_count(int, i, looseParts.get(0))
			{
				Array<WeaponPartType const*> chooseFrom;
				if (_usingWeaponParts)
				{
					for_every_ptr(wpt, *_usingWeaponParts)
					{
						if (wpt->can_be_chosen_randomly())
						{
							chooseFrom.push_back(wpt);
						}
					}
				}
				else
				{
					for_every_ref(wpt, Library::get_current_as<Library>()->get_weapon_part_types().get_all_stored())
					{
						if (GameDefinition::check_loot_availability_against_progress_level(wpt->get_tags()))
						{
							if (wpt->can_be_chosen_randomly())
							{
								chooseFrom.push_back(wpt);
							}
						}
					}
				}
				if (!chooseFrom.is_empty())
				{
					purge_obsolete_weapon_parts(chooseFrom);
					int idx = RandomUtils::ChooseFromContainer<Array<WeaponPartType const*>, WeaponPartType const*>::choose(
						rg, chooseFrom, [](WeaponPartType const* const& _e) { return _e->get_prob_coef_from_tags(); });
					
					auto* nextPart = chooseFrom[idx];

					add_part(WeaponPartAddress(), WeaponPart::create_instance_of(Persistence::access_current_if_exists(), nextPart, _useNonRandomisedParts.get(false)));
				}
			}
		}
		else
		{
			// we need to find or add chassis
			WeaponPartAddress chassisAddress;
			for_every(part, get_parts())
			{
				if (part->part->get_weapon_part_type()->get_type() == WeaponPartType::GunChassis)
				{
					chassisAddress = part->at;
				}
			}
			if (chassisAddress.is_empty())
			{
				Array<WeaponPartType const*> chassises;
				if (_usingWeaponParts)
				{
					for_every_ptr(wpt, *_usingWeaponParts)
					{
						if (GameDefinition::check_loot_availability_against_progress_level(wpt->get_tags()))
						{
							if (wpt->can_be_chosen_randomly() &&
								wpt->get_type() == WeaponPartType::GunChassis)
							{
								chassises.push_back(wpt);
							}
						}
					}
				}
				if (chassises.is_empty())
				{
					for_every_ref(wpt, Library::get_current_as<Library>()->get_weapon_part_types().get_all_stored())
					{
						if (GameDefinition::check_loot_availability_against_progress_level(wpt->get_tags()))
						{
							if (wpt->can_be_chosen_randomly() &&
								wpt->get_type() == WeaponPartType::GunChassis)
							{
								chassises.push_back(wpt);
							}
						}
					}
				}
				if (!chassises.is_empty())
				{
					Random::Generator rg;
					purge_obsolete_weapon_parts(chassises);
					int idx = RandomUtils::ChooseFromContainer<Array<WeaponPartType const*>, WeaponPartType const*>::choose(
						rg, chassises, [](WeaponPartType const* const& _e) { return _e->get_prob_coef_from_tags(); });
					auto* chassis = chassises[idx];

					chassisAddress.add(chassis->get_plug().id);

					add_part(chassisAddress, WeaponPart::create_instance_of(Persistence::access_current_if_exists(), chassis, _useNonRandomisedParts.get(false)));
				}
			}
			if (!chassisAddress.is_empty())
			{
				fill_with_random_parts_at(chassisAddress, _useNonRandomisedParts, _usingWeaponParts);
			}
		}
	}
	else
	{
		// find element at this address
		Random::Generator rg;
		for_every(part, get_parts())
		{
			if (part->at == _addr)
			{
				for_every(slot, part->part->get_weapon_part_type()->get_slots())
				{
					WeaponPartAddress nextAddr = _addr;
					nextAddr.add(slot->id);
					bool isConnected = false;
					for_every(part, get_parts())
					{
						if (part->at == nextAddr)
						{
							isConnected = true;
							break;
						}
					}
					if (!isConnected)
					{
						Array<WeaponPartType const*> chooseFrom;
						if (_usingWeaponParts)
						{
							for_every_ptr(wpt, *_usingWeaponParts)
							{
								if (wpt->can_be_chosen_randomly() &&
									wpt->get_plug().connector.is_valid() &&
									wpt->get_plug().connector == slot->connector &&
									wpt->get_plug().connectType == slot->connectType)
								{
									chooseFrom.push_back(wpt);
								}
							}
						}
						if (chooseFrom.is_empty())
						{
							for_every_ref(wpt, Library::get_current_as<Library>()->get_weapon_part_types().get_all_stored())
							{
								if (GameDefinition::check_loot_availability_against_progress_level(wpt->get_tags()))
								{
									if (wpt->can_be_chosen_randomly() &&
										wpt->get_plug().connector.is_valid() &&
										wpt->get_plug().connector == slot->connector &&
										wpt->get_plug().connectType == slot->connectType)
									{
										chooseFrom.push_back(wpt);
									}
								}
							}
						}
						if (!chooseFrom.is_empty())
						{
							purge_obsolete_weapon_parts(chooseFrom);
							int idx = RandomUtils::ChooseFromContainer<Array<WeaponPartType const*>, WeaponPartType const*>::choose(
								rg, chooseFrom, [](WeaponPartType const* const& _e) { return _e->get_prob_coef_from_tags(); });

							auto* nextPart = chooseFrom[idx];

							add_part(nextAddr, WeaponPart::create_instance_of(Persistence::access_current_if_exists(), nextPart, _useNonRandomisedParts.get(false)));
						}
					}
					fill_with_random_parts_at(nextAddr, _useNonRandomisedParts, _usingWeaponParts);
				}
				break;
			}
		}
	}
}

//

void WeaponSetupTemplate::clear()
{
	parts.clear();
}

bool WeaponSetupTemplate::load_loose_parts_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (_node->get_bool_attribute_or_from_child_presence(TXT("looseParts"), looseParts.is_set()))
	{
		looseParts.set_if_not_set(0);
	}
	looseParts.load_from_xml(_node, TXT("looseParts"));

	return result;
}

bool WeaponSetupTemplate::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("part")))
	{
		WeaponSetupTemplate::UsedWeaponPart wp;
		wp.at.set(node->get_string_attribute(TXT("at")));
		result &= wp.weaponPartType.load_from_xml(node, TXT("weaponPartType"));
		wp.nonRandomised = node->get_bool_attribute(TXT("nonRandomised"), wp.nonRandomised);
		wp.damaged = EnergyCoef::load_from_attribute_or_from_child(node, TXT("damaged"), EnergyCoef::zero());
		wp.defaultMeshParams = node->get_bool_attribute(TXT("defaultMeshParams"), wp.defaultMeshParams);
		parts.push_back(wp);
	}

	return result;
}

bool WeaponSetupTemplate::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	for_every(part, parts)
	{
		if (part->weaponPartType.get())
		{
			auto* node = _node->add_node(TXT("part"));
			node->set_attribute(TXT("at"), part->at.to_string().to_char());
			node->set_attribute(TXT("weaponPartType"), part->weaponPartType->get_name().to_string().to_char());
			if (part->nonRandomised) node->set_bool_attribute(TXT("nonRandomised"), part->nonRandomised);
			if (! part->damaged.is_zero()) node->set_attribute(TXT("damaged"), part->damaged.as_string_percentage());
			if (part->defaultMeshParams) node->set_bool_attribute(TXT("defaultMeshParams"), part->defaultMeshParams);
		}
	}

	return result;
}

bool WeaponSetupTemplate::resolve_library_links()
{
	bool result = true;

	for_every(wp, parts)
	{
		result &= wp->weaponPartType.find(Library::get_current());
	}

	return result;
}

//

TeaForGodEmperor::WeaponPart* WeaponSetupTemplate::WeaponPart::create_weapon_part(Persistence* _persistence, Optional<int> const& _rgIndex) const
{
	return TeaForGodEmperor::WeaponPart::create_instance_of(_persistence, weaponPartType.get(), nonRandomised, damaged, defaultMeshParams, _rgIndex);
}
