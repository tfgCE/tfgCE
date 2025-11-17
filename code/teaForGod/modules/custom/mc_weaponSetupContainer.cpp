#include "mc_weaponSetupContainer.h"

#include "..\..\game\persistence.h"
#include "..\..\library\library.h"

#include "..\..\..\core\random\randomUtils.h"

#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// tags
DEFINE_STATIC_NAME(nonLoot);

// module params
DEFINE_STATIC_NAME(id);

//

REGISTER_FOR_FAST_CAST(WeaponSetupContainer);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new WeaponSetupContainer(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new WeaponSetupContainerData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & WeaponSetupContainer::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("weaponSetupContainer")), create_module, create_module_data);
}

WeaponSetupContainer::WeaponSetupContainer(Framework::IModulesOwner* _owner)
: base(_owner)
, weaponSetup(Persistence::access_current_if_exists())
{
}

WeaponSetupContainer::~WeaponSetupContainer()
{
}

void WeaponSetupContainer::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	auto* wscData = fast_cast<WeaponSetupContainerData>(_moduleData);

	id = _moduleData->get_parameter<Name>(this, NAME(id), id);

	if (wscData)
	{
		weaponSetup.set_loose_parts(wscData->get_weapon_setup().get_loose_parts());
	}
	if (wscData && !wscData->get_weapon_setup().get_parts().is_empty())
	{
		Random::Generator weaponPartRG = get_owner()->get_individual_random_generator();
		weaponPartRG.advance_seed(97856, 97234);
		// build parts if not already provided
		for_every(dp, wscData->get_weapon_setup().get_parts())
		{
			if (auto* wp = dp->create_weapon_part(&Persistence::access_current(), weaponPartRG.get_int()))
			{
				weaponSetup.add_part(dp->at, wp, wscData->should_use_non_randomised_parts());
			}
		}
	}
	if (wscData && wscData->should_fill_with_random_parts())
	{
		bool addAsUsual = true;
		if (weaponSetup.get_loose_parts().is_set())
		{
			// use limits
			struct UseRandomWeaponParts
			{
				Optional<int> left;
				WeaponSetupContainerData::RandomWeaponParts const* rwp;
			};
			Array<UseRandomWeaponParts> urwps;
			for_every(rwp, wscData->get_random_parts())
			{
				UseRandomWeaponParts urwp;
				urwp.left = rwp->limit;
				urwp.rwp = rwp;
				urwps.push_back(urwp);
				if (urwp.left.is_set())
				{
					addAsUsual = false;
				}
			}
			if (!addAsUsual)
			{
				int loosePartsLeft = weaponSetup.get_loose_parts().get();
				Array<WeaponPartType const*> wpts;
				while (loosePartsLeft > 0 && !urwps.is_empty())
				{
					int idx = RandomUtils::ChooseFromContainer<Array<UseRandomWeaponParts>, UseRandomWeaponParts>::choose(Random::next_int(), urwps, [](UseRandomWeaponParts const& urwp) { return urwp.rwp->probCoef; });
					auto& urwp = urwps[idx];
					wpts.clear();
					if (auto* wpt = urwp.rwp->weaponPartType.get())
					{
						wpts.push_back(wpt);
					}
					if (!urwp.rwp->tagged.is_empty())
					{
						for_every_ptr(wpt, Library::get_current_as<Library>()->get_weapon_part_types().get_tagged(urwp.rwp->tagged))
						{
							if (!wpt->get_tags().get_tag(NAME(nonLoot)) &&
								GameDefinition::check_loot_availability_against_progress_level(wpt->get_tags()))
							{
								wpts.push_back(wpt);
							}
						}
					}
					if (wpts.get_size())
					{
						auto* wpt = wpts[Random::get_int(wpts.get_size())]; todo_note(TXT("use probability?"));
						weaponSetup.add_part(WeaponPartAddress(), WeaponPart::create_instance_of(&Persistence::access_current(), wpt, wscData->should_use_non_randomised_parts().get(false)));
						--loosePartsLeft;
						if (urwp.left.is_set())
						{
							urwp.left = urwp.left.get() - 1;
							if (urwp.left.get() <= 0)
							{
								urwps.remove_at(idx);
							}
						}
					}
					else
					{
						urwps.remove_at(idx);
					}
				}
			}
		}
		if (addAsUsual)
		{
			Array<WeaponPartType const*> wpts;
			wpts.make_space_for_additional(wscData->get_random_parts().get_size());
			for_every(rwp, wscData->get_random_parts())
			{
				if (auto* wpt = rwp->weaponPartType.get())
				{
					wpts.push_back(rwp->weaponPartType.get());
				}
				if (!rwp->tagged.is_empty())
				{
					for_every_ptr(wpt, Library::get_current_as<Library>()->get_weapon_part_types().get_tagged(rwp->tagged))
					{
						if (!wpt->get_tags().get_tag(NAME(nonLoot)) &&
							GameDefinition::check_loot_availability_against_progress_level(wpt->get_tags()))
						{
							wpts.push_back(wpt);
						}
					}
				}
			}
			for_every_ptr(wpt, wpts)
			{
				DEFINE_STATIC_NAME(nodeSlots);
				if (wpt->get_tags().get_tag(NAME(nodeSlots)) >= 2.1f)
				{
					output(TXT("invalid node slots for %S"), get_owner()->ai_get_name().to_char());
					for_every_ptr(wpt, wpts)
					{
						output(TXT("+- %S"), wpt->get_name().to_string().to_char());
					}
					warn_dev_investigate(TXT("invalid node slots for %S"), get_owner()->ai_get_name().to_char());
				}
			}
			weaponSetup.fill_with_random_parts_at(WeaponPartAddress(), wscData->should_use_non_randomised_parts(), &wpts);
		}
	}
}

//

REGISTER_FOR_FAST_CAST(WeaponSetupContainerData);

WeaponSetupContainerData::WeaponSetupContainerData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

WeaponSetupContainerData::~WeaponSetupContainerData()
{
}

bool WeaponSetupContainerData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	fillWithRandomParts = _node->get_bool_attribute_or_from_child_presence(TXT("fillWithRandomParts"), fillWithRandomParts);
	nonRandomisedParts.load_from_xml(_node, TXT("nonRandomisedParts"));

	weaponSetup.clear();

	weaponSetup.load_loose_parts_from_xml(_node);

	for_every(node, _node->children_named(TXT("parts")))
	{
		weaponSetup.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("weaponSetup")))
	{
		weaponSetup.load_from_xml(node);
	}

	for_every(containerNode, _node->children_named(TXT("randomParts")))
	{
		for_every(node, containerNode->children_named(TXT("weaponPart")))
		{
			if (node->has_attribute(TXT("type")))
			{
				Framework::UsedLibraryStored<WeaponPartType> wpt;
				if (wpt.load_from_xml(node, TXT("type"), _lc))
				{
					RandomWeaponParts rwp;
					rwp.weaponPartType = wpt;
					rwp.limit.load_from_xml(node, TXT("limit"));
					rwp.probCoef = node->get_float_attribute(TXT("probCoef"), rwp.probCoef);
					randomParts.push_back(rwp);
				}
				else
				{
					error_loading_xml(node, TXT("couldn't load weaponPart type"));
					result = false;
				}
			}
			else if (node->has_attribute(TXT("tagged")))
			{
				TagCondition tc;
				if (tc.load_from_xml_attribute(node, TXT("tagged")))
				{
					RandomWeaponParts rwp;
					rwp.tagged = tc;
					rwp.limit.load_from_xml(node, TXT("limit"));
					rwp.probCoef = node->get_float_attribute(TXT("probCoef"), rwp.probCoef);
					randomParts.push_back(rwp);
				}
				else
				{
					error_loading_xml(node, TXT("couldn't load weaponPart tagged"));
					result = false;
				}
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load weaponPart for random parts"));
				result = false;
			}
		}
	}

	if (!randomParts.is_empty())
	{
		if (!fillWithRandomParts)
		{
			warn_loading_xml(_node, TXT("no fillWithRandomParts set but random parts provided, assuming it should be set to true"));
			fillWithRandomParts = true;
		}
	}

	return result;
}

bool WeaponSetupContainerData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(rwp, randomParts)
	{
		result &= rwp->weaponPartType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		an_assert(_library == Library::get_current());
		result &= weaponSetup.resolve_library_links();
	}

	return result;
}

void WeaponSetupContainerData::prepare_to_unload()
{
	base::prepare_to_unload();
}
