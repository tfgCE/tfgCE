#include "lootSelector.h"

#include "gameDefinition.h"
#include "library.h"

#include "..\game\weaponPart.h"
#include "..\modules\custom\mc_lootProvider.h"
#include "..\modules\custom\mc_weaponSetupContainer.h"
#include "..\modules\gameplay\moduleOrbItem.h"
#include "..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\modules\gameplay\equipment\me_gun.h"

#include "..\..\core\other\parserUtils.h"
#include "..\..\core\random\randomUtils.h"

#include "..\..\framework\game\game.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\moduleCollision.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\object.h"
#include "..\..\framework\world\room.h"
#include "..\..\framework\world\worldObject.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// library global referencer
DEFINE_STATIC_NAME_STR(energyQuantumHealth, TXT("energy quantum health"));
DEFINE_STATIC_NAME_STR(energyQuantumAmmo, TXT("energy quantum ammo"));
DEFINE_STATIC_NAME_STR(energyQuantumPowerAmmo, TXT("energy quantum power ammo"));
DEFINE_STATIC_NAME_STR(weaponItem, TXT("weapon item type to use with weapon parts"));
#ifdef WITH_CRYSTALITE
DEFINE_STATIC_NAME_STR(crystaliteItem, TXT("crystalite item type"));
#endif

// energy quantum types
DEFINE_STATIC_NAME(ammo);
DEFINE_STATIC_NAME(health);
DEFINE_STATIC_NAME_STR(powerAmmo, TXT("power ammo"));

// tags
DEFINE_STATIC_NAME(chance);
DEFINE_STATIC_NAME(nonLoot);

// code prob coef tags
DEFINE_STATIC_NAME(item);
DEFINE_STATIC_NAME(weapon);

// variables
DEFINE_STATIC_NAME(lootObject);
DEFINE_STATIC_NAME(lootSelectorAddProbCoef);

//

struct LootSelectorManager
{
public:
	static void initialise_static();
	static void close_static();
	static void clear_static();
	static void make_sure_is_built();
	static void output_log();
	static LootSelector const * select_for(LootSelectorContext& _context);

private:
	static LootSelectorManager* s_selector;

	Concurrency::MRSWLock accessLock;

	// ideally, library should not be reloaded
	Array<SafePtr<LootSelector>> sortedSelectors;

	void internal_make_sure_is_built();
	void internal_output_log();

	LootSelector const * internal_select_for(LootSelectorContext& _context, Random::Generator & _rg);

	void log_selectors(LogInfoContext& _log, LootSelector const * _for = nullptr);
	void check_for_circularities();
};

LootSelectorManager* LootSelectorManager::s_selector = nullptr;

void LootSelectorManager::initialise_static()
{
	// might be reloaded
	if (!s_selector)
	{
		s_selector = new LootSelectorManager();
	}
}

void LootSelectorManager::close_static()
{
	delete_and_clear(s_selector);
}

void LootSelectorManager::clear_static()
{
	s_selector->sortedSelectors.clear();
}

LootSelector const* LootSelectorManager::select_for(LootSelectorContext& _context)
{
	an_assert(s_selector);
	Random::Generator rg = _context.lootProvider->get_owner()->get_individual_random_generator();
	rg.advance_seed(2397, 9754);
	return s_selector->internal_select_for(_context, rg);
}

void LootSelectorManager::make_sure_is_built()
{
	an_assert(s_selector);
	s_selector->internal_make_sure_is_built();
}

void LootSelectorManager::output_log()
{
	an_assert(s_selector);
	s_selector->internal_output_log();
}

void LootSelectorManager::internal_make_sure_is_built()
{
	if (sortedSelectors.is_empty())
	{
		accessLock.release_read(); // leave for a moment
		{
			Concurrency::ScopedMRSWLockWrite lock(accessLock);
			if (sortedSelectors.is_empty())
			{
				for_every_ref(ls, Library::get_current_as<Library>()->get_loot_selectors().get_all_stored())
				{
					sortedSelectors.push_back(SafePtr<LootSelector>(ls));
				}
				sort(sortedSelectors, LootSelector::compare_safe_ptrs_by_priority);
				check_for_circularities();
			}
		}
		accessLock.acquire_read(); // get back
	}
}

void LootSelectorManager::internal_output_log()
{
	internal_make_sure_is_built();
	accessLock.release_read(); // leave for a moment
	{
		LogInfoContext log;
		log.set_colour(Colour::green);
		log.log(TXT("LOOT SELECTORS"));
		{
			LOG_INDENT(log);
			log.set_colour();
			log_selectors(log);
			log.output_to_output();
		}
	}
	accessLock.acquire_read(); // get back
}

LootSelector const* LootSelectorManager::internal_select_for(LootSelectorContext& _context, Random::Generator & _rg)
{
	Concurrency::ScopedMRSWLockRead lock(accessLock);

#ifdef AN_DEVELOPMENT
	{
		bool anySelector = false;
		for_every_ref(ls, sortedSelectors)
		{
			if (!ls)
			{
				continue;
			}
			anySelector = true;
			break;
		}
		if (!anySelector)
		{
			sortedSelectors.clear();
		}
	}
#endif

	internal_make_sure_is_built();

	if (!_context.lootProvider)
	{
		return nullptr;
	}

	struct Candidate
	{
		LootSelector const* candidate = nullptr;
		float probCoef = 0.0f;

		Candidate() {}
		Candidate(LootSelector const* _candidate, float _probCoef) : candidate(_candidate), probCoef(_probCoef) {}
	};
	ARRAY_STATIC(Candidate, candidates, 20);

	LootSelector const* selected = nullptr;
	int depthLeft = sortedSelectors.get_size();
	while (depthLeft > 0)
	{
		--depthLeft;
		candidates.clear();
		Optional<int> candidatesPriority;
		LootSelector const* alreadySelected = selected;
		for_every_ref(ls, sortedSelectors)
		{
			if (!ls)
			{
				continue;
			}

			if (ls->is_specialisation_of(alreadySelected))
			{
				if (ls->check_conditions_for(_context.lootProvider))
				{
					float probCoef = ls->get_prob_coef_for(_context.lootProvider);
					if (probCoef > 0.0f)
					{
						if (!candidatesPriority.is_set() ||
							ls->get_priority() > candidatesPriority.get())
						{
							candidates.clear();
							candidatesPriority = ls->get_priority();
						}
						if (!candidatesPriority.is_set() || ls->get_priority() == candidatesPriority.get())
						{
							candidates.push_back_or_replace(Candidate(ls, probCoef), _rg);
						}
					}
				}
			}
		}

		if (candidates.is_empty())
		{
			break;
		}

		if (candidates.get_size() == 1)
		{
			selected = candidates[0].candidate;
		}
		else
		{
			int idx = RandomUtils::ChooseFromContainer<ArrayStatic<Candidate, 20>, Candidate>::choose(_rg, candidates, [](Candidate const & _candidate) { return _candidate.probCoef; });
			selected = candidates[idx].candidate;
		}
	}

	return selected;
}

void LootSelectorManager::check_for_circularities()
{
	todo_note(TXT("this has to be reimplemented for multiple specialisations of"));
	/*
	ARRAY_STACK(LootSelector const*, selectorStack, sortedSelectors.get_size());
	for_every_ref(s, sortedSelectors)
	{
		selectorStack.clear();
		auto const * si = s;
		while (si)
		{
			if (selectorStack.does_contain(si))
			{
				error(TXT("circularity detected for \"%S\""), si->get_name().to_string().to_char());
				break;
			}
			if (! selectorStack.has_place_left())
			{
				error(TXT("circularity detected for \"%S\" (no duplicates, though)"), si->get_name().to_string().to_char());
				break;
			}
			selectorStack.push_back(si);
			si = si->get_specialisation_of();
		}
	}
	*/
}

void LootSelectorManager::log_selectors(LogInfoContext& _log, LootSelector const* _for)
{
	for_every_ref(s, sortedSelectors)
	{
		if (s->is_specialisation_of(_for))
		{
			_log.log(TXT("%S"), s->get_name().to_string().to_char());
			{
				LOG_INDENT(_log);
				log_selectors(_log, s);
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(LootSelector);
LIBRARY_STORED_DEFINE_TYPE(LootSelector, lootSelector);

int LootSelector::compare_safe_ptrs_by_priority(void const* _a, void const* _b)
{
	::SafePtr<LootSelector> const& a = *(plain_cast<::SafePtr<LootSelector>>(_a));
	::SafePtr<LootSelector> const& b = *(plain_cast<::SafePtr<LootSelector>>(_b));

	if (a.get() && b.get())
	{
		if (a->get_priority() > b->get_priority()) return A_BEFORE_B;
		if (a->get_priority() < b->get_priority()) return B_BEFORE_A;
		return String::compare_icase(a->get_name().get_group().to_string(), b->get_name().get_group().to_string());
	}
	return A_AS_B;
}

void LootSelector::initialise_static()
{
	LootSelectorManager::initialise_static();
}

void LootSelector::close_static()
{
	LootSelectorManager::close_static();
}

void LootSelector::clear_static()
{
	LootSelectorManager::clear_static();
}

void LootSelector::build_for_game()
{
	LootSelectorManager::make_sure_is_built();
}

void LootSelector::output_loot_selector()
{
	LootSelectorManager::output_log();
}

void LootSelector::output_loot_providers()
{
	LogInfoContext log;
	Array<Framework::LibraryStoreBase*> stores;

	stores.push_back(&Library::get_current()->get_actor_types());
	stores.push_back(&Library::get_current()->get_item_types());
	stores.push_back(&Library::get_current()->get_scenery_types());

	for_every_ptr(store, stores)
	{
		store->do_for_every([&log](Framework::LibraryStored* _libraryStored)
		{
			if (auto* ot = fast_cast<Framework::ObjectType>(_libraryStored))
			{
				bool outputOTName = true;
				for_every(mc, ot->get_customs())
				{
					if (auto* lp = fast_cast<CustomModules::LootProviderData>(mc->get_data()))
					{
						if (outputOTName)
						{
							log.log(TXT("%S"), ot->get_name().to_string().to_char());
							log.increase_indent();
							outputOTName = false;
						}
						lp->output_log(log);
					}
				}
				if (!outputOTName)
				{
					log.decrease_indent();
				}
			}
		});
	}

	log.output_to_output();
}

void LootSelector::create_loot_for(LootSelectorContext& _context)
{
	if (auto* ls = LootSelectorManager::select_for(_context))
	{
		ls->create_loot_objects_for(_context);
	}
#ifdef WITH_CRYSTALITE
	create_crystalite_for(_context);
#endif
}

LootSelector::LootSelector(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
, SafeObject<LootSelector>(this)
{
}

LootSelector::~LootSelector()
{
	make_safe_object_unavailable();
}

bool LootSelector::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	priority = _node->get_int_attribute(TXT("priority"), priority);
	probCoef = _node->get_float_attribute(TXT("probCoef"), probCoef);
	probCoef = _node->get_float_attribute(TXT("probabilityCoef"), probCoef);
	if (auto* attr = _node->get_attribute(TXT("mulProbCoefByTag")))
	{
		mulProbCoefByTag.push_back(attr->get_as_name());
	}
	for_every(node, _node->children_named(TXT("mulProbCoef")))
	{
		Name byTag = node->get_name_attribute(TXT("byTag"));
		if (byTag.is_valid())
		{
			mulProbCoefByTag.push_back(byTag);
		}
		else
		{
			error_loading_xml(node, TXT("no \"byTag\""));
			result = false;
		}
	}
	if (auto* attr = _node->get_attribute(TXT("overrideProbCoefByTag")))
	{
		overrideProbCoefByTag.push_back(attr->get_as_name());
	}
	for_every(node, _node->children_named(TXT("overrideProbCoef")))
	{
		Name byTag = node->get_name_attribute(TXT("byTag"));
		if (byTag.is_valid())
		{
			overrideProbCoefByTag.push_back(byTag);
		}
		else
		{
			error_loading_xml(node, TXT("no \"byTag\""));
			result = false;
		}
	}
	if (auto* attr = _node->get_attribute(TXT("codeProbCoefTag")))
	{
		codeProbCoefTag.push_back(attr->get_as_name());
	}
	for_every(node, _node->children_named(TXT("codeProbCoef")))
	{
		Name byTag = node->get_name_attribute(TXT("tag"));
		if (byTag.is_valid())
		{
			codeProbCoefTag.push_back(byTag);
		}
		else
		{
			error_loading_xml(node, TXT("no \"byTag\""));
			result = false;
		}
	}
	if (_node->has_attribute(TXT("specialisationOf")))
	{
		Framework::UsedLibraryStored<LootSelector> so;
		so.load_from_xml(_node, TXT("specialisationOf"), _lc);
		specialisationOf.push_back(so);
	}
	for_every(node, _node->children_named(TXT("specialisation")))
	{
		Framework::UsedLibraryStored<LootSelector> so;
		if (so.load_from_xml(node, TXT("of"), _lc))
		{
			specialisationOf.push_back(so);
		}
		else
		{
			result = false;
		}
	}

	// conditions
	//

	lootTags.load_from_xml_attribute_or_child_node(_node, TXT("lootTagged"));

	// loot
	//

	for_every(lootNode, _node->children_named(TXT("loot")))
	{
		for_every(node, lootNode->all_children())
		{
			if (node->get_name() == TXT("makeWeapon"))
			{
				// ok
			}
			else if (node->get_name() == TXT("energyQuantum"))
			{
				Loot::EnergyQuantum eq;
				if (eq.load_from_xml(node, _lc))
				{
					loot.energyQuantums.push_back(eq);
				}
				else
				{
					error_loading_xml(node, TXT("can't load energy quantum"));
					result = false;
				}
			}
			else if (node->get_name() == TXT("object") ||
					 node->get_name() == TXT("item") ||
					 node->get_name() == TXT("actor"))
			{
				Loot::Object object;
				if (object.load_from_xml(node, _lc))
				{
					loot.objects.push_back(object);
				}
				else
				{
					error_loading_xml(node, TXT("can't load object"));
					result = false;
				}
			}
			else if (node->get_name() == TXT("weaponPart"))
			{
				Loot::WeaponPart wp;
				if (wp.load_from_xml(node, _lc))
				{
					loot.weaponParts.push_back(wp);
				}
				else
				{
					error_loading_xml(node, TXT("can't load weapon part"));
					result = false;
				}
			}
			else
			{
				error_loading_xml(node, TXT("loot not recognised"));
				result = false;
			}
		}
		{
			loot.makeWeapon = lootNode->get_bool_attribute_or_from_child_presence(TXT("makeWeapon"), loot.makeWeapon);
			if (loot.weaponParts.is_empty())
			{
				if (lootNode->get_bool_attribute_or_from_child_presence(TXT("makeWeapon")))
				{
					// we need one empty
					Loot::WeaponPart wp;
					loot.weaponParts.push_back(wp);
				}
			}
		}
	}

	return result;
}

bool LootSelector::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(lootObject, loot.objects)
	{
		result = lootObject->objectType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	for_every(lootWP, loot.weaponParts)
	{
		result = lootWP->weaponPartType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	for_every(so, specialisationOf)
	{
		result = so->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}

float LootSelector::get_prob_coef_for(CustomModules::LootProvider const* _lootProvider) const
{
	float result = probCoef;
	float overrideResult = 0.0f;
	bool forceOverride = false;
	for_every(byTag, mulProbCoefByTag)
	{
		result *= _lootProvider->get_loot_tags().get_tag(*byTag, 1.0f); // if not present, assume it doesn't influence
	}
	for_every(byTag, overrideProbCoefByTag)
	{
		overrideResult += _lootProvider->get_loot_tags().get_tag(*byTag, 0.0f);
	}
	for_every(tag, codeProbCoefTag)
	{
		if (*tag == NAME(weapon))
		{
			if (EXMType::does_player_have_equipped(EXMID::Passive::weapon_collector()))
			{
				if (auto* exm = EXMType::find(EXMID::Passive::weapon_collector()))
				{
					float addProb = exm->get_custom_parameters().get_value<float>(NAME(lootSelectorAddProbCoef), 1.0f);
					result += addProb;
					if (overrideResult > 0.0f)
					{
						overrideResult += addProb;
					}
				}
			}
		}
	}
	return overrideResult > 0.0f || forceOverride ? overrideResult : result;
}

bool LootSelector::is_specialisation_of(LootSelector const* _lootSelector) const
{
	if (!_lootSelector && specialisationOf.is_empty())
	{
		return true;
	}

	for_every_ref(so, specialisationOf)
	{
		if (so == _lootSelector)
		{
			return true;
		}
	}

	return false;
}

bool LootSelector::check_conditions_for(CustomModules::LootProvider const* _lootProvider) const
{
	if (!lootTags.is_empty())
	{
		if (!lootTags.check(_lootProvider->get_loot_tags()))
		{
			return false;
		}
	}
	return true;
}

#ifdef WITH_CRYSTALITE
void LootSelector::create_crystalite_for(LootSelectorContext& _context)
{
	if (_context.crystalite > 0
#ifndef AN_DEVELOPMENT_OR_PROFILER
		&& GameSettings::get().difficulty.withCrystalite
#endif
		)
	{
		auto* lootProvider = _context.lootProvider;
		auto* lootProviderObject = lootProvider->get_owner();
		auto* inRoom = lootProviderObject->get_presence()->get_in_room();

		if (!inRoom)
		{
			warn(TXT("not placed in room, can't create loot objects"));
			return;
		}

		Random::Generator rg;

		// reset random generator
		rg = lootProvider->get_owner()->get_individual_random_generator();
		rg.advance_seed(80653, 697345);

		Framework::LibraryName itCrystaliteName = Framework::Library::get_current()->get_global_references().get_name(NAME(crystaliteItem));
		{
			Framework::ItemType* itCrystalite = itCrystaliteName.is_valid() ? Framework::Library::get_current()->get_item_types().find(itCrystaliteName) : nullptr;
			{
				int idx = 0;
				while (_context.crystalite > 0)
				{
					Framework::ItemType* it = itCrystalite;
					if (it)
					{
						Framework::Object* spawnedObject = nullptr;
						String objectName = lootProviderObject->ai_get_name() + String::printf(TXT("[cr%i]:%S"), idx, it->get_name().to_string().to_char());

						{
							Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(lootProviderObject), inRoom, false); // will be activated with others

							if (it)
							{
								it->load_on_demand_if_required();

								Framework::Game::get()->perform_sync_world_job(TXT("spawn crystalite"), [it, objectName, &spawnedObject, inRoom]()
									{
										spawnedObject = it->create(objectName);
										spawnedObject->init(inRoom->get_in_sub_world());
									});
							}
							else
							{
								an_assert(false);
							}
							spawnedObject->access_individual_random_generator() = Random::Generator();

							spawnedObject->access_variables().set_from(lootProviderObject->get_variables());
							spawnedObject->learn_from(lootProviderObject->access_variables());

							spawnedObject->initialise_modules();

							spawnedObject->get_collision()->dont_collide_with_up_to_top_instigator(lootProviderObject);

							spawnedObject->be_non_autonomous();

							spawnedObject->suspend_advancement();

							spawnedObject->set_instigator(lootProviderObject);

							if (auto* moi = spawnedObject->get_gameplay_as<ModuleOrbItem>())
							{
								moi->set_crystalite(1);
							}
						}

						lootProvider->add_loot(spawnedObject);
					}
					else
					{
						error(TXT("no crystalite item found! maybe not defined in global referencer?"));
						break;
					}
					--_context.crystalite;
					++idx;
				}
			}
		}
	}
}
#endif

void LootSelector::create_loot_objects_for(LootSelectorContext& _context) const
{
	auto* lootProvider = _context.lootProvider;
	auto* lootProviderObject = lootProvider->get_owner();
	auto* inRoom = lootProviderObject->get_presence()->get_in_room();

	if (!inRoom)
	{
		warn(TXT("not placed in room, can't create loot objects"));
		return;
	}

	Random::Generator rg;

	// reset random generator
	rg = lootProvider->get_owner()->get_individual_random_generator();
	rg.advance_seed(7954, 972597);

	for_every(object, loot.objects)
	{
		auto* objectType = object->objectType.get();
		if (objectType)
		{
			Framework::Object* spawnedObject = nullptr;
			String objectName = lootProviderObject->ai_get_name() + String::printf(TXT("[i%i]:%S"), for_everys_index(object), objectType->get_name().to_string().to_char());

			objectType->load_on_demand_if_required();

			{
				Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(lootProviderObject), inRoom, false); // will be activated with others
				Framework::Game::get()->perform_sync_world_job(TXT("spawn object"), [objectType, &objectName, &spawnedObject, inRoom]()
				{
					spawnedObject = objectType->create(objectName);
					spawnedObject->init(inRoom->get_in_sub_world());
				});
				spawnedObject->access_individual_random_generator() = rg.spawn();

				inRoom->collect_variables(spawnedObject->access_variables());
				//spawnedObject->access_variables().set_from(lootProviderObject->get_variables()); with this we will copy size! we don't want this but we want to remember why we don't do so
				spawnedObject->access_variables().set_from(object->variables);
				spawnedObject->access_variables().access<bool>(NAME(lootObject)) = true;
				spawnedObject->learn_from(spawnedObject->access_variables());

				spawnedObject->initialise_modules();

				spawnedObject->be_non_autonomous();

				spawnedObject->suspend_advancement();
			}
			lootProvider->add_loot(spawnedObject);
		}
	}

	if (! loot.energyQuantums.is_empty())
	{
		// reset random generator
		rg = lootProvider->get_owner()->get_individual_random_generator();
		rg.advance_seed(80653, 697345);

		Framework::LibraryName eqAmmoName = Framework::Library::get_current()->get_global_references().get_name(NAME(energyQuantumAmmo));
		Framework::LibraryName eqHealthName = Framework::Library::get_current()->get_global_references().get_name(NAME(energyQuantumHealth));
		Framework::LibraryName eqPowerAmmoName = Framework::Library::get_current()->get_global_references().get_name(NAME(energyQuantumPowerAmmo));
		{
			Framework::ItemType* itAmmo = eqAmmoName.is_valid() ? Framework::Library::get_current()->get_item_types().find(eqAmmoName) : nullptr;
			Framework::ItemType* itHealth = eqHealthName.is_valid() ? Framework::Library::get_current()->get_item_types().find(eqHealthName) : nullptr;
			Framework::ItemType* itPowerAmmo = eqPowerAmmoName.is_valid() ? Framework::Library::get_current()->get_item_types().find(eqPowerAmmoName) : nullptr;
			{
				for_every(eq, loot.energyQuantums)
				{
					EnergyType::Type energyType = EnergyType::Ammo;
					auto type = eq->type;
					Framework::ItemType* it = nullptr;
					if (type == NAME(ammo)) { it = itAmmo; energyType = EnergyType::Ammo; }
					if (type == NAME(health)) { it = itHealth; energyType = EnergyType::Health; }
					if (type == NAME(powerAmmo)) { it = itPowerAmmo; energyType = EnergyType::Ammo; }
					if (it)
					{
						Energy energy = eq->energy.get();
						Energy sideEffectDamage = eq->sideEffectDamage.get();

						Framework::Object* spawnedObject = nullptr;
						String objectName = lootProviderObject->ai_get_name() + String::printf(TXT("[eq%i]:%S"), for_everys_index(eq), it->get_name().to_string().to_char());

						{
							Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(lootProviderObject), inRoom, false); // will be activated with others

							if (it)
							{
								it->load_on_demand_if_required();

								Framework::Game::get()->perform_sync_world_job(TXT("spawn energy quantum"), [it, objectName, &spawnedObject, inRoom]()
								{
									spawnedObject = it->create(objectName);
									spawnedObject->init(inRoom->get_in_sub_world());
								});
							}
							else
							{
								an_assert(false);
							}
							spawnedObject->access_individual_random_generator() = Random::Generator();

							spawnedObject->access_variables().set_from(lootProviderObject->get_variables());
							spawnedObject->learn_from(lootProviderObject->access_variables());

							spawnedObject->initialise_modules();

							spawnedObject->get_collision()->dont_collide_with_up_to_top_instigator(lootProviderObject);

							spawnedObject->be_non_autonomous();

							spawnedObject->suspend_advancement();

							spawnedObject->set_instigator(lootProviderObject);

							if (auto* meq = spawnedObject->get_gameplay_as<ModuleEnergyQuantum>())
							{
								meq->start_energy_quantum_setup();
								meq->set_energy(energy);
								meq->set_energy_type(energyType);
								meq->set_side_effect_damage(sideEffectDamage);
								meq->end_energy_quantum_setup();
							}
						}

						lootProvider->add_loot(spawnedObject);
					}
					else
					{
						error(TXT("no energy quantum found! maybe not defined in global referencer?"));
					}
				}
			}
		}
	}

	if (!loot.weaponParts.is_empty()
		|| (loot.makeWeapon && _context.lootProvider && _context.lootProvider->is_weapon_setup_from_weapon_setup_container())
		)
	{
		// reset random generator
		rg = lootProvider->get_owner()->get_individual_random_generator();
		rg.advance_seed(8055, 29734);

		struct WeaponPartSource
		{
			TeaForGodEmperor::WeaponSetup const * weaponSetupFromWSC = nullptr;
			Loot::WeaponPart const* lootWeaponPart = nullptr;
		};
		Array<WeaponPartSource> weaponPartSources;

		bool getWeaponPartsFromLoot = true;

		if (loot.makeWeapon && _context.lootProvider && _context.lootProvider->is_weapon_setup_from_weapon_setup_container())
		{
			CustomModules::WeaponSetupContainer* wsc = nullptr;
			{
				Name id = _context.lootProvider->get_weapon_setup_from_weapon_setup_container_id();
				for_every_ptr(custom, lootProvider->get_owner()->get_customs())
				{
					if (auto* c = fast_cast<CustomModules::WeaponSetupContainer>(custom))
					{
						if (c->get_id() == id)
						{
							wsc = c;
							break;
						}
					}
				}
				if (!wsc)
				{
					error_dev_ignore(TXT("couldn't find WeaponSetupContainer module with id \"%S\""), id.to_char());
				}
			}
			if (wsc)
			{
				WeaponPartSource wps;
				wps.weaponSetupFromWSC = &wsc->get_weapon_setup();
				weaponPartSources.push_back(wps);
				getWeaponPartsFromLoot = false;
			}
		}

		if (getWeaponPartsFromLoot)
		{
			for_every(wp, loot.weaponParts)
			{
				WeaponPartSource wps;
				wps.lootWeaponPart = wp;
				weaponPartSources.push_back(wps);
			}
		}

		Framework::LibraryName itemName = Framework::Library::get_current()->get_global_references().get_name(NAME(weaponItem));
		if (Framework::ItemType* itemType = itemName.is_valid() ? Framework::Library::get_current()->get_item_types().find(itemName) : nullptr)
		{
			{
				bool corePresent = false;
				bool chassisPresent = false;
				Array<WeaponPartType const*> chosenWeaponParts;
				WeaponSetup const* chosenWeaponSetup = nullptr; // more important than parts
				for_every(wps, weaponPartSources)
				{
					if (auto* ws = wps->weaponSetupFromWSC)
					{
						if (ws->is_valid())
						{
							chassisPresent = true;
							corePresent = true;
							chosenWeaponSetup = ws;
						}
					}
					if (!chosenWeaponSetup) // doesn't make any sense to use parts if we have a weapon setup
					{
						if (auto* wp = wps->lootWeaponPart)
						{
							for_every_ptr(wpt, Library::get_current_as<Library>()->get_weapon_part_types().get_tagged(wp->weaponPartTags))
							{
								if (! wpt->get_tags().get_tag(NAME(nonLoot)) &&
									GameDefinition::check_loot_availability_against_progress_level(wpt->get_tags()))
								{
									if (wpt->get_type() == WeaponPartType::GunChassis)
									{
										chassisPresent = true;
									}
									if (wpt->get_type() == WeaponPartType::GunCore)
									{
										corePresent = true;
									}
									chosenWeaponParts.push_back(wpt);
								}
							}
						}
					}
				}

				if (corePresent && chassisPresent)
				{
					Framework::Object* spawnedObject = nullptr;
					String objectName = lootProviderObject->ai_get_name() + String::printf(TXT("[weapon]:%S"), itemType->get_name().to_string().to_char());

					itemType->load_on_demand_if_required();
					{
						Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(lootProviderObject), inRoom, false); // will be activated with others
						Framework::Game::get()->perform_sync_world_job(TXT("spawn item"), [itemType, &objectName, &spawnedObject, inRoom]()
						{
							spawnedObject = itemType->create(objectName);
							spawnedObject->init(inRoom->get_in_sub_world());
						});
						spawnedObject->access_individual_random_generator() = rg.spawn();

						inRoom->collect_variables(spawnedObject->access_variables());
						spawnedObject->access_variables().set_from(lootProviderObject->get_variables());
						spawnedObject->learn_from(spawnedObject->access_variables());

						{
							bool moduleGunPresent = false;
							spawnedObject->initialise_modules([chosenWeaponSetup, &chosenWeaponParts, &moduleGunPresent](Framework::Module* _module)
							{
								if (auto* moduleGun = fast_cast<ModuleEquipments::Gun>(_module))
								{
									moduleGunPresent = true;
									// doing it in pre setup (initialise_modules) will ignore default "non randomised parts" - our provided setups will be used
									if (chosenWeaponSetup)
									{
										moduleGun->setup_with_weapon_setup(*chosenWeaponSetup);
									}
									else
									{
										moduleGun->fill_with_random_parts(&chosenWeaponParts);
									}
								}
							});
							if (!moduleGunPresent)
							{
								warn(TXT("item type \"%S\" was expected to have module Gun"), itemName.to_string().to_char());
							}
						}

						spawnedObject->be_non_autonomous();

						spawnedObject->suspend_advancement();
					}
					lootProvider->add_loot(spawnedObject);
				}
				else
				{
					if (!chosenWeaponSetup && chosenWeaponParts.is_empty())
					{
						warn(TXT("no weapon setup nor weapon parts to build a weapon"));
					}
					else if (!corePresent)
					{
						warn(TXT("no core part to build a weapon"));
					}
					else if (!chassisPresent)
					{
						warn(TXT("no chassis part to build a weapon"));
					}
					else
					{
						warn(TXT("can't build a weapon"));
					}
				}
			}
		}
	}
}

//

bool LootSelector::Loot::EnergyQuantum::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	type = _node->get_name_attribute_or_from_child(TXT("type"), type);

	energy.load_from_xml(_node, TXT("energy"));
	sideEffectDamage.load_from_xml(_node, TXT("sideEffectDamage"));

	error_loading_xml_on_assert(!energy.is_empty(), result, _node, TXT("no energy provided"));

	return result;
}

//

bool LootSelector::Loot::Object::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	objectType.load_from_xml(_node, TXT("type"), _lc);

	if (!objectType.is_name_valid())
	{
		error_loading_xml(_node, TXT("no type provided"));
		result = false;
	}

	variables.load_from_xml_child_node(_node, TXT("parameters"));

	return result;
}

//

bool LootSelector::Loot::WeaponPart::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	weaponPartType.load_from_xml(_node, TXT("weaponPartType"), _lc);
	weaponPartTags.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));

	return result;
}

