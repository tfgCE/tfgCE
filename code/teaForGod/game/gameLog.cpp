#include "gameLog.h"

#include "energy.h"
#include "gameStats.h"

#include "..\modules\energyTransfer.h"
#include "..\modules\gameplay\moduleEquipment.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\custom\health\mc_health.h"

#include "..\tutorials\tutorial.h"
#include "..\tutorials\tutorialSystem.h"

#include "..\..\core\concurrency\scopedMRSWLock.h"
#include "..\..\core\system\core.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// tutorial global references
// ...

// reasons
// ...

// events
DEFINE_STATIC_NAME(died);
DEFINE_STATIC_NAME(grabbedEnergyQuantum);
DEFINE_STATIC_NAME(healthRegenOnDeathStatus);
DEFINE_STATIC_NAME(madeEnergyQuantumRelease);

//

GameLog::Entry::Entry(Name const& _name)
: frame(::System::Core::get_frame())
, name(_name)
{
	params[0] = 0;
	params[1] = 0;
	params[2] = 0;
	params[3] = 0;
}

GameLog::Entry& GameLog::Entry::set_as_int(int _param, int _value)
{
	params[_param] = _value;
	return *this;
}

int GameLog::Entry::get_as_int(int _param) const
{
	return params[_param];
}

GameLog::Entry& GameLog::Entry::set_as_bool(int _param, bool _value)
{
	params[_param] = _value;
	return *this;
}

bool GameLog::Entry::get_as_bool(int _param) const
{
	return params[_param];
}

GameLog::Entry& GameLog::Entry::set_as_float(int _param, float _value)
{
	*(plain_cast<float>(&params[_param])) = _value;
	return *this;
}

float GameLog::Entry::get_as_float(int _param) const
{
	return *(plain_cast<float>(&params[_param]));
}

GameLog::Entry& GameLog::Entry::set_as_name(int _param, Name const & _value)
{
	params[_param] = _value.get_index();
	return *this;
}

Name GameLog::Entry::get_as_name(int _param) const
{
	return Name(params[_param]);
}

GameLog::Entry& GameLog::Entry::set_as_energy(int _param, Energy const& _value)
{
	params[_param] = _value.get_pure();
	return *this;
}

Energy GameLog::Entry::get_as_energy(int _param) const
{
	return Energy::pure(params[_param]);
}

//

int GameLog::SuggestedTutorial::compare(void const* _a, void const* _b)
{
	GameLog::SuggestedTutorial const & a = *plain_cast<GameLog::SuggestedTutorial>(_a);
	GameLog::SuggestedTutorial const & b = *plain_cast<GameLog::SuggestedTutorial>(_b);
	if (a.done && ! b.done) return B_BEFORE_A;
	if (! a.done && b.done) return A_BEFORE_B;
	int aOrder = a.idx;
	int bOrder = b.idx;
	if (a.done && b.done)
	{	// prioritise those tutorials that were not played for a certain time
		aOrder -= a.minutesSinceLastPlayed / 600;
		bOrder -= b.minutesSinceLastPlayed / 600;
	}
	{	// if we done something more times, push it back
		aOrder += a.howManyTimes * 2;
		bOrder += b.howManyTimes * 2;
	}
	if (aOrder < bOrder) return A_BEFORE_B;
	if (aOrder > bOrder) return B_BEFORE_A;
	return A_AS_B;
}

//

GameLog::NamedEntries::Iterator& GameLog::NamedEntries::Iterator::operator ++ ()
{
	++iEntry;
	while (iEntry != owner->entries->end())
	{
		auto e = *iEntry;
		if (e.get_name() == owner->entryName)
		{
			break;
		}
		++iEntry;
	}
	return *this;
}

GameLog::NamedEntries::Iterator GameLog::NamedEntries::Iterator::operator ++ (int)
{
	an_assert(false, TXT("why are you using this?"));
	GameLog::NamedEntries::Iterator copy = *this;
	operator ++();
	return copy;
}

//

GameLog* GameLog::s_log = nullptr;

GameLog::GameLog()
{
	an_assert(!s_log);
	s_log = this;
}

GameLog::~GameLog()
{
	an_assert(s_log == this);
	s_log = nullptr;
}

void GameLog::lookup_tutorials()
{
}

void GameLog::lookup_tutorial(Name const& _name)
{
	if (auto* tut = Framework::Library::get_current()->get_global_references().get<TeaForGodEmperor::Tutorial>(_name, true))
	{
		tutorials[_name] = Framework::UsedLibraryStored<TeaForGodEmperor::Tutorial>(tut);
	}
	else
	{
		warn(TXT("could not find tutorial \"%S\""), _name.to_char());
	}
}

void GameLog::clear_and_open()
{
	Concurrency::ScopedMRSWLockWrite lock(entriesLock);
	entries.clear();
	suggestedTutorials.clear();
	counters.set_size(Counter::MAX);
	for_every(c, counters)
	{
		*c = 0;
	}
	isOpen = true;
}

void GameLog::close()
{
	Concurrency::ScopedMRSWLockWrite lock(entriesLock);
	isOpen = false;
}

GameLog::Entry & GameLog::add_entry(Entry const& _entry)
{
	if (! isOpen)
	{
		return emptyEntry;
	}
	Concurrency::ScopedMRSWLockWrite lock(entriesLock);

	entries.push_back(_entry);
	return entries.get_last();
}

void GameLog::increase_counter(GameLog::Counter _counter)
{
	if (!isOpen)
	{
		return;
	}
	Concurrency::ScopedMRSWLockWrite lock(entriesLock);

	++ counters[_counter];
}

void GameLog::suggest_tutorial(Name const& _name, Name const& _lsReason)
{
	if (auto* tutPtr = tutorials.get_existing(_name))
	{
		if (auto* tutorial = tutPtr->get())
		{
			for_every(st, suggestedTutorials)
			{
				if (st->tutorial == tutorial)
				{
					if (_lsReason.is_valid())
					{
						st->lsReasons.push_back_unique(_lsReason);
					}
					return;
				}
			}
			SuggestedTutorial st;
			st.idx = suggestedTutorials.get_size();
			st.done = PlayerSetup::get_current().was_tutorial_done(tutorial->get_name(), &st.minutesSinceLastPlayed, &st.howManyTimes);
			st.tutorial = tutorial;
			if (_lsReason.is_valid())
			{
				st.lsReasons.push_back(_lsReason);
			}
			int didTooManyTimes = 3;
			// don't suggest tutorials that were done already too many times, unless they were done more than a week ago
			if (st.howManyTimes > didTooManyTimes &&
				st.minutesSinceLastPlayed < 60 * 24 * 7)
			{
				if (Random::get_int(st.howManyTimes) > didTooManyTimes)
				{
					return;
				}
			}
			suggestedTutorials.push_back(st);
		}
	}
}

GameLog::Entry const* GameLog::find_last(Name const& _name) const
{
	Concurrency::ScopedMRSWLockRead lock(entriesLock);

	for_every_reverse(e, entries)
	{
		if (e->get_name() == _name)
		{
			return e;
		}
	}

	return nullptr;
}

void GameLog::choose_suggested_tutorials()
{
	suggestedTutorials.clear();
	TutorialSystem::get()->unset_last_tutorial();

	/*
	// analyse game log and choose tutorials
	// higher priority first!

	// !@#
	//suggest_tutorial(NAME(tutDiedByShootingAtPowerAmmoOrb), NAME(lsTutReasonDiedByShootingAtPowerAmmoOrb));
	//suggest_tutorial(NAME(tutSomeHealthLeftInBackup), NAME(lsTutReasonSomeHealthLeftInBackup));
	//suggest_tutorial(NAME(tutDidNotAttractAnyEnergyOrb), NAME(lsTutReasonDidNotAttractAnyEnergyOrb));

	EnergyLevelsIssues energyLevels;

	{
		int weaponPartsToDegrade = 0;
		int weaponPartsLastUse = 0;
		int weaponPartsDegraded = 0;

		auto& ps = TeaForGodEmperor::PlayerSetup::get_current();
		ps.get_pilgrim_setup().inspect_weapon_part_in_use(weaponPartsToDegrade, weaponPartsLastUse, weaponPartsDegraded);

		if (weaponPartsDegraded > 0)
		{
			suggest_tutorial(NAME(tutWeaponPartsDegraded), NAME(lsTutReasonWeaponPartsDegraded));
		}
		else if (weaponPartsToDegrade + weaponPartsLastUse > 0)
		{
			suggest_tutorial(NAME(tutWeaponPartsDegraded), NAME(lsTutReasonWeaponPartsSoonToDegrade));
		}
		if ((weaponPartsDegraded + weaponPartsToDegrade + weaponPartsLastUse > 0) || Random::get_chance(0.2f))
		{
			if (GameStats::get().weaponPartsFound == 0)
			{
				suggest_tutorial(NAME(tutWeaponPartsFindOrbs), NAME(lsTutReasonWeaponPartsFindOrbs));
			}
			if (GameStats::get().weaponPartsDisassembled == 0)
			{
				suggest_tutorial(NAME(tutWeaponPartsDisassembleItems), NAME(lsTutReasonWeaponPartsDisassembleItems));
			}
		}
	}

	if (auto* slot = PlayerSetup::get_current().get_active_game_slot())
	{
		if ((slot->stats.exmsUnlocked > 0 && slot->stats.exmsUnlocked < 5) || Random::get_chance(0.2f))
		{
			suggest_tutorial(NAME(tutUseLoadoutEXM), NAME(lsTutReasonUseLoadoutEXM));
		}
		if ((slot->stats.weaponPartsTransferred == 0 || GameStats::get().weaponPartsTransferred == 0) && GameStats::get().weaponPartsFound + GameStats::get().weaponPartsDisassembled > 0)
		{
			suggest_tutorial(NAME(tutWeaponPartsTransfer), NAME(lsTutReasonWeaponPartsNotTransferred));
		}
	}

	if (auto* e = find_last(NAME(died)))
	{
		if (e->get_as_bool(DIED__BY_LOCAL_PLAYER))
		{
			if (e->get_as_name(DIED__SOURCE) == TXT("power ammo"))
			{
				suggest_tutorial(NAME(tutDiedByShootingAtPowerAmmoOrb), NAME(lsTutReasonDiedByShootingAtPowerAmmoOrb));
			}
		}
	}
	else
	{
		if (auto* e = find_last(NAME(healthRegenOnDeathStatus)))
		{
			if (!e->get_as_energy(HEALTH_REGEN_ON_DEATH_STATUS__ENERGY_IN_BACKUP).is_zero())
			{
				suggest_tutorial(NAME(tutSomeHealthLeftInBackup), NAME(lsTutReasonSomeHealthLeftInBackup));
			}
		}
	}

	if (counters[Counter::InstalledActiveEXM] && !counters[Counter::UsedActiveEXM])
	{
		suggest_tutorial(NAME(tutDidntUseActiveEXM), NAME(lsTutReasonDidntUseActiveEXM));
	}

	if (counters[Counter::AmmoDepleted])
	{
		suggest_tutorial(NAME(tutRunOutOfAmmo), NAME(lsTutReasonRunOutOfAmmo));
		suggest_tutorial(NAME(tutRunOutOfAmmoUseFists), NAME(lsTutReasonRunOutOfAmmoUseFists));
		energyLevels.runOutOfEnergyWeapons = true;
	}

	if (energyLevels.is_it_worth_mentioning())
	{
		bool addEnergyTransfer = true;
		if (Random::get_chance(0.6f))
		{
			energyLevels.suggest_tutorial(this, NAME(tutEnergyManagement));
			addEnergyTransfer = Random::get_chance(0.7f);
		}
		if (addEnergyTransfer)
		{
			if ((!counters[TransferKioskDeposit] || !counters[TransferKioskWithdraw]) || Random::get_chance(0.3f))
			{
				energyLevels.suggest_tutorial(this, NAME(tutEnergyTransfer_TransferKiosk));
			}
			if ((!counters[EXMsEnergyTransfer]) || Random::get_chance(0.3f))
			{
				energyLevels.suggest_tutorial(this, NAME(tutEnergyTransfer_EXMs));
			}
		}
	}

	{
		bool requiresPullingTutorial = false;
		for_every(e, all_entries_named(NAME(madeEnergyQuantumRelease)))
		{
			requiresPullingTutorial = true;
		}
		
		if (requiresPullingTutorial)
		{
			for_every(e, all_entries_named(NAME(grabbedEnergyQuantum)))
			{
				if (e->get_as_int(GRABBED_ENERGY_QUANTUM__NUMBER_OF_PULLS) > 4 &&
					e->get_as_float(GRABBED_ENERGY_QUANTUM__MAX_DISTANCE) > 2.0f)
				{
					requiresPullingTutorial = false;
					break;
				}
			}

			if (requiresPullingTutorial)
			{
				suggest_tutorial(NAME(tutDidNotAttractAnyEnergyOrb), NAME(lsTutReasonDidNotAttractAnyEnergyOrb));
			}
		}
	}
	*/

	// sort to show first tutorials that we haven't done
	sort(suggestedTutorials);
}

void GameLog::EnergyLevelsIssues::suggest_tutorial(GameLog* owner, Name const& _name)
{
	ArrayStatic<Name, 16> reasons; SET_EXTRA_DEBUG_INFO(reasons, TXT("GameLog::EnergyLevelsIssues::suggest_tutorial.reasons"));
	//if (runOutOfEnergyWeapons) reasons.push_back(NAME(lsTutReasonRunOutOfEnergyWeapons));
	//if (runOutOfEnergyHealth) reasons.push_back(NAME(lsTutReasonRunOutOfEnergyHealth));

	for_every(reason, reasons)
	{
		owner->suggest_tutorial(_name, *reason);
	}
}

void GameLog::check_for_energy_balance(Framework::IModulesOwner* imo)
{
	if (!isOpen)
	{
		return;
	}

	{	// we already know it was not balanced
		if (counters.is_empty() || counters[EnergyUnbalanced])
		{
			return;
		}
	}

	Energy health = Energy::zero();
	Energy mainEquipment[2] = { Energy::zero(), Energy::zero() };
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		health += h->calculate_total_energy_available(EnergyType::Health);
	}
	if (auto* p = imo->get_gameplay_as<ModulePilgrim>())
	{
		for_count(int, hIdx, Hand::MAX)
		{
			Hand::Type hand = (Hand::Type)hIdx;
			if (auto* e = p->get_main_equipment(hand))
			{
				mainEquipment[hIdx] += IEnergyTransfer::calculate_total_energy_available_for(e);
			}
		}
	}

	float fHealth = health.as_float();
	float fMainEquipment[2] = { mainEquipment[0].as_float(), mainEquipment[1].as_float() };

	float maxE = max(fHealth, max(fMainEquipment[0], fMainEquipment[1]));
	float minE = min(fHealth, min(fMainEquipment[0], fMainEquipment[1]));
	if (maxE - minE > 200.0f)
	{
		increase_counter(GameLog::EnergyUnbalanced);
	}
}
