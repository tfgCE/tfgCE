#include "persistence.h"

#include "gameStats.h"
#include "playerSetup.h"

#include "..\library\library.h"
#include "..\library\gameDefinition.h"
#include "..\library\missionDefinition.h"
#include "..\library\missionGeneralProgress.h"

#include "..\modules\gameplay\modulePilgrim.h"
#include "..\pilgrim\pilgrimSetup.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\other\parserUtils.h"
#include "..\..\core\random\random.h"
#include "..\..\core\system\core.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\object\itemType.h"
#include "..\..\framework\object\temporaryObjectType.h"
#include "..\..\framework\world\region.h"
#include "..\..\framework\world\room.h"
#include "..\..\framework\world\subWorld.h"
#include "..\..\framework\video\texture.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(centre);
DEFINE_STATIC_NAME(core);
DEFINE_STATIC_NAME(hole);
DEFINE_STATIC_NAME(exm);

// hardcoded exms "maze element only"
DEFINE_STATIC_NAME(_core);
DEFINE_STATIC_NAME_STR(_gSkip, TXT("_g/skip")); // grid skip
DEFINE_STATIC_NAME_STR(_mExmEf1, TXT("_m-exm-ef/1")); // multiple exm efficiency

//

#define INFO_PROGRESS_LOAD(infoProgressVar) infoProgressVar = (State)_node->get_int_attribute(TXT(#infoProgressVar), infoProgressVar);
#define INFO_PROGRESS_SAVE(infoProgressVar) _node->set_int_attribute(TXT(#infoProgressVar), infoProgressVar);
#define INFO_PROGRESS_FORCE_TO_SHOW_IF_NO(infoProgressVar) if (infoProgressVar == No) infoProgressVar = ToShow;
#define INFO_PROGRESS_ADVANCE_TO_DONE(infoProgressVar) if (infoProgressVar == ToShow) infoProgressVar = Done;

bool PersistenceInfoProgress::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	INFO_PROGRESS_LOAD(welcomeMessagePresented);
	INFO_PROGRESS_LOAD(loadoutSetupAvailable);
	return result;
}

bool PersistenceInfoProgress::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;
	INFO_PROGRESS_SAVE(welcomeMessagePresented);
	INFO_PROGRESS_SAVE(loadoutSetupAvailable);
	return result;
}

void PersistenceInfoProgress::force_to_show_if_no()
{
	INFO_PROGRESS_FORCE_TO_SHOW_IF_NO(welcomeMessagePresented);
	INFO_PROGRESS_FORCE_TO_SHOW_IF_NO(loadoutSetupAvailable);
}

void PersistenceInfoProgress::on_pilgrimage_started()
{
	INFO_PROGRESS_ADVANCE_TO_DONE(welcomeMessagePresented);
	INFO_PROGRESS_ADVANCE_TO_DONE(loadoutSetupAvailable);
}

void PersistenceInfoProgress::advance_to_show(Persistence const & persistence)
{
	if (welcomeMessagePresented < Done)
	{
		welcomeMessagePresented = ToShow;
	}
	else
	{
		if (loadoutSetupAvailable == No)
		{
			loadoutSetupAvailable = ToShow;
		}
	}
}

//

bool PersistenceProgress::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	lootProgressLevel.load_from_xml(_node->first_child_named(TXT("loot")));
	return result;
}

bool PersistenceProgress::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;
	lootProgressLevel.save_to_xml(_node->add_node(TXT("loot")));
	return result;
}

//

bool PersistenceProgressLevel::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	if (!_node)
	{
		return result;
	}
	mileStones.load_from_xml_attribute_or_child_node(_node, TXT("mileStones"));
	progressLevel100 = _node->get_int_attribute(TXT("progressLevel100"), progressLevel100);
	runTimePendingSeconds = _node->get_int_attribute(TXT("runTimePendingSeconds"), runTimePendingSeconds);
	return result;
}

bool PersistenceProgressLevel::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;
	_node->set_attribute(TXT("mileStones"), mileStones.to_string().to_char());
	_node->set_int_attribute(TXT("progressLevel100"), progressLevel100);
	_node->set_int_attribute(TXT("runTimePendingSeconds"), runTimePendingSeconds);
	return result;
}


Persistence::Persistence()
: SafeObject<Persistence>(this)
{
}

Persistence::~Persistence()
{
	make_safe_object_unavailable();
}

void Persistence::cache_exms()
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);
	
	unlockedEXMs.clear();
	permanentEXMs.clear();
	for_every(exmId, allEXMs)
	{
		if (auto* exm = EXMType::find(*exmId))
		{
			if (!exm->is_integral())
			{
				if (exm->is_permanent())
				{
					permanentEXMs.push_back(*exmId);
				}
				else 
				{
					unlockedEXMs.push_back_unique(*exmId);
				}
			}
		}
	}
}

bool Persistence::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;

	killInfos.clear();
	allEXMs.clear();
	unlockedEXMs.clear();
	permanentEXMs.clear();
	lastUsedSetup.clear();
	experienceToSpend = Energy::zero();
	meritPointsToSpend = 0;
	missionIntel = 0;
	missionIntelInfo.clear();
	missionSeeds.clear();
	missionGeneralProgressInfo.clear();
	unlockedMissionGeneralProgress.clear();
	infoProgress = PersistenceInfoProgress();
	progress = PersistenceProgress();
	weaponsInArmouryColumns = baseWeaponsInArmouryColumns;
	weaponsInArmoury.clear();

	for_every(node, _node->children_named(_childName))
	{
		for_every(progressNode, node->children_named(TXT("progress")))
		{
			result &= progress.load_from_xml(progressNode);
		}

		todo_note(TXT("legacy load, remove"));
		for_every(infoProgressNode, node->children_named(TXT("progress")))
		{
			result &= infoProgress.load_from_xml(infoProgressNode);
		}

		for_every(infoProgressNode, node->children_named(TXT("infoProgress")))
		{
			result &= infoProgress.load_from_xml(infoProgressNode);
		}

		for_every(xpNode, node->children_named(TXT("experience")))
		{
			experienceToSpend.load_from_xml(xpNode, TXT("toSpend"));
		}

		for_every(xpNode, node->children_named(TXT("meritPoints")))
		{
			meritPointsToSpend = xpNode->get_int_attribute(TXT("toSpend"), meritPointsToSpend);
		}

		for_every(xpNode, node->children_named(TXT("mission")))
		{
			missionIntel = xpNode->get_int_attribute(TXT("intel"), missionIntel);
			missionIntelInfo.load_from_xml(xpNode, TXT("info"));
			if (auto* attr = xpNode->get_attribute(TXT("seed")))
			{
				String seedString = attr->get_as_string();
				List<String> seeds;
				seedString.split(String::comma(), seeds);
				missionSeeds.clear();
				for_every(seed, seeds)
				{
					missionSeeds.push_back(ParserUtils::parse_int(*seed));
				}
			}
			missionGeneralProgressInfo.load_from_xml(xpNode, TXT("generalProgressInfo"));

			for_every(un, xpNode->children_named(TXT("unlocked")))
			{
				unlockedMissionGeneralProgress.push_back();
				if (unlockedMissionGeneralProgress.get_last().load_from_xml(un, TXT("missionGeneralProgress")))
				{
					unlockedMissionGeneralProgress.get_last().find_may_fail(Library::get_current());
				}
				else
				{
					unlockedMissionGeneralProgress.pop_back();
				}
			}
		}

		for_every(contNode, node->children_named(TXT("allEXMs")))
		{
			for_every(sNode, contNode->children_named(TXT("unlocked")))
			{
				Name exm = sNode->get_name_attribute(TXT("exm"));
				if (exm.is_valid())
				{
					allEXMs.push_back(exm);
				}
			}
		}

		cache_exms();

		result &= lastUsedSetup.load_from_xml_child_node(node, TXT("lastUsedSetup"));

		for_every(kisNode, node->children_named(TXT("killInfos")))
		{
			for_every(kiNode, kisNode->children_named(TXT("killInfo")))
			{
				KillInfo ki;
				ki.what.from_string(kiNode->get_string_attribute(TXT("what")));
				ki.count = kiNode->get_int_attribute(TXT("count"));
				if (ki.what.is_valid())
				{
					bool added = false;
					for_every(kie, killInfos)
					{
						if (kie->what == ki.what)
						{
							kie->count += ki.count;
							added = true;
							break;
						}
					}
					if (!added)
					{
						killInfos.push_back(ki);
					}
				}
			}
		}

		for_every(wiaNode, node->children_named(TXT("weaponsInArmoury")))
		{
			int extraWeaponsInArmouryColumns = wiaNode->get_int_attribute(TXT("extraColumns"), max(0, weaponsInArmouryColumns - baseWeaponsInArmouryColumns));
			weaponsInArmouryColumns = baseWeaponsInArmouryColumns + max(0, extraWeaponsInArmouryColumns);
			for_every(wNode, wiaNode->children_named(TXT("weapon")))
			{
				weaponsInArmoury.push_back(WeaponInArmoury(this));
				auto& wia = weaponsInArmoury.get_last();
				if (wNode->has_attribute(TXT("armouryX")) || wNode->has_attribute(TXT("armouryY")))
				{
					wia.at = VectorInt2::zero;
					wia.at.access().load_from_xml(wNode, TXT("armouryX"), TXT("armouryY"));
				}
				wia.onPilgrimInfo.onPilgrim = wNode->get_bool_attribute(TXT("onPilgrim"), false);
				wia.onPilgrimInfo.mainWeaponForHand.clear();
				wia.onPilgrimInfo.inPocket.clear();
				if (wia.onPilgrimInfo.onPilgrim)
				{
					if (auto* attr = wNode->get_attribute(TXT("mainWeaponForHand")))
					{
						wia.onPilgrimInfo.mainWeaponForHand = Hand::parse(attr->get_as_string());
					}
					if (auto* attr = wNode->get_attribute(TXT("inPocket")))
					{
						wia.onPilgrimInfo.inPocket = attr->get_as_int();
					}
				}
				result &= wia.setup.load_from_xml(wNode);
			}
		}
	}

	return result;
}

bool Persistence::save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName) const
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	bool result = true;

	IO::XML::Node* node = _node->add_node(_childName);

	{
		IO::XML::Node* progressNode = node->add_node(TXT("progress"));
		progress.save_to_xml(progressNode);
	}

	{
		IO::XML::Node* infoProgressNode = node->add_node(TXT("infoProgress"));
		infoProgress.save_to_xml(infoProgressNode);
	}

	{
		IO::XML::Node* xpNode = node->add_node(TXT("experience"));
		xpNode->set_attribute(TXT("toSpend"), experienceToSpend.as_string_auto_decimals());
	} 
	
	{
		IO::XML::Node* xpNode = node->add_node(TXT("meritPoints"));
		xpNode->set_int_attribute(TXT("toSpend"), meritPointsToSpend);
	} 
	
	{
		IO::XML::Node* xpNode = node->add_node(TXT("mission"));
		xpNode->set_int_attribute(TXT("intel"), missionIntel);
		xpNode->set_attribute(TXT("info"), missionIntelInfo.to_string());
		if (! missionSeeds.is_empty())
		{
			String seeds;
			for_every(seed, missionSeeds)
			{
				if (!seeds.is_empty()) seeds += String::comma();
				seeds += String::printf(TXT("%i"), *seed);
			}
			xpNode->set_attribute(TXT("seed"), seeds);
		}
		xpNode->set_attribute(TXT("generalProgressInfo"), missionGeneralProgressInfo.to_string());
		for_every(umgp, unlockedMissionGeneralProgress)
		{
			auto* un = xpNode->add_node(TXT("unlocked"));
			umgp->save_to_xml(un, TXT("missionGeneralProgress"));
		}
	} 
	
	{
		IO::XML::Node* contNode = node->add_node(TXT("allEXMs"));
		for_every(exm, allEXMs)
		{
			IO::XML::Node* sNode = contNode->add_node(TXT("unlocked"));
			sNode->set_attribute(TXT("exm"), exm->to_string());
		}
	}

	result &= lastUsedSetup.save_to_xml_child_node(node, TXT("lastUsedSetup"));

	{
		IO::XML::Node* kisNode = node->add_node(TXT("killInfos"));
		for_every(ki, killInfos)
		{
			IO::XML::Node* kiNode = kisNode->add_node(TXT("killInfo"));
			kiNode->set_attribute(TXT("what"), ki->what.to_string());
			kiNode->set_int_attribute(TXT("count"), ki->count);
		}
	}

	{
		IO::XML::Node* wiaNode = node->add_node(TXT("weaponsInArmoury"));
		wiaNode->set_int_attribute(TXT("extraColumns"), max(0, weaponsInArmouryColumns - baseWeaponsInArmouryColumns));
		for_every(wia, weaponsInArmoury)
		{
			IO::XML::Node* wNode = wiaNode->add_node(TXT("weapon"));
			if (wia->at.is_set())
			{
				wia->at.get().save_to_xml(wNode, TXT("armouryX"), TXT("armouryY"));
			}
			if (wia->onPilgrimInfo.onPilgrim)
			{
				wNode->set_bool_attribute(TXT("onPilgrim"), true);
				if (wia->onPilgrimInfo.mainWeaponForHand.is_set())
				{
					wNode->set_attribute(TXT("mainWeaponForHand"), Hand::to_char(wia->onPilgrimInfo.mainWeaponForHand.get()));
				}
				if (wia->onPilgrimInfo.inPocket.is_set())
				{
					wNode->set_int_attribute(TXT("inPocket"), wia->onPilgrimInfo.inPocket.get());
				}
			}
			wia->setup.save_to_xml(wNode);
		}
	}

	return result;
}

bool Persistence::resolve_library_links()
{
	bool result = true;

	return result;
}

void Persistence::setup_auto()
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);
}

void Persistence::store_last_setup(ModulePilgrim* _pilgrim)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
	output(TXT("[persistence] store last setup"));
#endif

	lastUsedSetup = UsedSetup(); // clear
	{
		auto& ps = _pilgrim->get_pilgrim_setup();
		Concurrency::ScopedMRSWLockRead lockPS(ps.access_lock());
		for_count(int, hIdx, Hand::MAX)
		{
			Hand::Type hand = (Hand::Type)hIdx;
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
			output(TXT("[persistence] hand %S"), Hand::to_char(hand));
#endif
			auto& handSetup = ps.get_hand(hand);
			lastUsedSetup.activeEXMs[hand] = handSetup.activeEXM.exm;
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
			output(TXT("[persistence] +- active %S"), handSetup.activeEXM.exm.to_char());
			output(TXT("[persistence] +- passive"));
#endif
			for_every(exm, handSetup.passiveEXMs)
			{
				if (exm->exm.is_valid())
				{
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
					output(TXT("[persistence]    +- %S"), exm->exm.to_char());
#endif
					lastUsedSetup.passiveEXMs[hand].push_back(exm->exm);
				}
			}
		}
	}
	{
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
		output(TXT("[persistence] inventory -> permanent"));
#endif
		auto& pi = _pilgrim->get_pilgrim_inventory();
		Concurrency::ScopedMRSWLockRead lockPS(pi.exmsLock);
		for_every(exm, pi.permanentEXMs)
		{
			if (exm->is_valid())
			{
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
				output(TXT("[persistence] +- %S"), exm->to_char());
#endif
				lastUsedSetup.permanentEXMs.push_back(*exm);
			}
		}
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
		output(TXT("[persistence] inventory -> available"));
#endif
		for_every(exm, pi.availableEXMs)
		{
			if (exm->is_valid())
			{
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
				output(TXT("[persistence] +- %S"), exm->to_char());
#endif
				lastUsedSetup.availableEXMs.push_back_unique(*exm);
			}
		}
	}
}

void Persistence::provide_experience(Energy const & _experience)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	experienceToSpend += _experience;

	remove_too_old_experiences_gained();

	ExperienceGained eg;
	eg.amount = _experience;
	eg.when.reset();
	experienceGained.push_front(eg);
	++experienceGainedVer;
}

bool Persistence::spend_experience(Energy const & _experience)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	if (experienceToSpend >= _experience)
	{
		experienceToSpend -= _experience;
		return true;
	}
	return false;
}

void Persistence::provide_merit_points(int _meritPoints)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	meritPointsToSpend += _meritPoints;
}

bool Persistence::spend_merit_points(int _meritPoints)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	if (meritPointsToSpend >= _meritPoints)
	{
		meritPointsToSpend -= _meritPoints;
		return true;
	}
	return false;
}

void Persistence::add_mission_intel(int _intel, MissionDefinition const* _mission)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	if (_mission)
	{
		output(TXT("modify mission intel: %i by %i"), missionIntel, _intel);
		auto& cap = _mission->get_mission_intel().get_cap();
		if (_intel <= 0)
		{
			missionIntel = max(0, missionIntel + _intel);
		}
		else
		{
			if (cap.is_set())
			{
				if (missionIntel <= cap.get())
				{
					missionIntel = min(cap.get(), missionIntel + _intel);
				}
				// we can't get any bigger
			}
			else
			{
				missionIntel += _intel;
			}
		}
		output(TXT("modified mission intel: %i by %i"), missionIntel, _intel);
	}
}

void Persistence::add_mission_intel_info(Tags const& _add)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	output(TXT("[mission intel] adding \"%S\" to \"%S\""), _add.to_string().to_char(), missionIntelInfo.to_string().to_char());

	missionIntelInfo.add_tags_by_relevance(_add);

	output(TXT("[mission intel] added, result  \"%S\""), missionIntelInfo.to_string().to_char());
}

void Persistence::set_mission_intel_info(Tags const& _set)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	output(TXT("[mission intel] setting \"%S\" to \"%S\""), _set.to_string().to_char(), missionIntelInfo.to_string().to_char());

	missionIntelInfo.set_tags_from(_set);

	output(TXT("[mission intel] set, result  \"%S\""), missionIntelInfo.to_string().to_char());
}

void Persistence::remove_mission_intel_info(Tags const& _clear)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	output(TXT("[mission intel] removing \"%S\" from \"%S\""), _clear.to_string().to_char(), missionIntelInfo.to_string().to_char());

	missionIntelInfo.remove_tags(_clear);

	output(TXT("[mission intel] removed, result  \"%S\""), missionIntelInfo.to_string().to_char());
}

void Persistence::set_mission_seed(int _tierIdx, int _seed)
{
	if (_tierIdx < 0 || _tierIdx > 100)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	while (missionSeeds.get_size() <= _tierIdx)
	{
		missionSeeds.push_back(0);
	}

	missionSeeds[_tierIdx] = _seed;
}

void Persistence::advance_mission_seed(int _tierIdx, int _by)
{
	if (_tierIdx < 0 || _tierIdx > 100)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	while (missionSeeds.get_size() <= _tierIdx)
	{
		missionSeeds.push_back(0);
	}

	missionSeeds[_tierIdx] += _by;
}

int Persistence::get_mission_seed(int _tierIdx) const
{
	int seed;
	{
		Concurrency::ScopedSpinLock lock(persistenceLock, true);
		if (missionSeeds.is_index_valid(_tierIdx))
		{
			seed = missionSeeds[_tierIdx];
		}
		else
		{
			seed = 0;
		}
	}
	return seed;
}

int Persistence::get_mission_seed_cumulative() const
{
	int seed = 0;
	{
		Concurrency::ScopedSpinLock lock(persistenceLock, true);
		for_every(s, missionSeeds)
		{
			seed *= 6073761;
			seed += *s;
		}
	}
	return seed;
}

void Persistence::add_mission_general_progress_info(Tags const& _add)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	output(TXT("[mission general info] adding \"%S\" to \"%S\""), _add.to_string().to_char(), missionGeneralProgressInfo.to_string().to_char());

	missionGeneralProgressInfo.add_tags_by_relevance(_add);

	output(TXT("[mission general info] added, result  \"%S\""), missionGeneralProgressInfo.to_string().to_char());
}

void Persistence::remove_mission_general_progress_info(Tags const& _clear)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	output(TXT("[mission intel] removing \"%S\" from \"%S\""), _clear.to_string().to_char(), missionGeneralProgressInfo.to_string().to_char());

	missionGeneralProgressInfo.remove_tags(_clear);

	output(TXT("[mission intel] removed, result  \"%S\""), missionGeneralProgressInfo.to_string().to_char());
}

bool Persistence::unlock_mission_general_progress(MissionGeneralProgress const* _mgp)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	if (is_mission_general_progress_unlocked(_mgp))
	{
		return false;
	}

	unlockedMissionGeneralProgress.push_back();
	unlockedMissionGeneralProgress.get_last() = _mgp;
	
	return true;
}

bool Persistence::is_mission_general_progress_unlocked(MissionGeneralProgress const* _mgp) const
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	for_every(umgp, unlockedMissionGeneralProgress)
	{
		if (*umgp == _mgp)
		{
			return true;
		}
	}

	return false;
}

void Persistence::mark_killed(Framework::IModulesOwner const* _imo)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	Framework::LibraryName imoLibName = _imo->get_library_name();

	for_every(ki, killInfos)
	{
		if (ki->what == imoLibName)
		{
			++ ki->count;
			return;
		}
	}

	KillInfo ki;
	ki.what = imoLibName;
	killInfos.push_back(ki);
}

int Persistence::get_kill_count(Framework::IModulesOwner const* _imo) const
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	Framework::LibraryName imoLibName = _imo->get_library_name();

	for_every(ki, killInfos)
	{
		if (ki->what == imoLibName)
		{
			return ki->count;
		}
	}

	return 0;
}

Persistence& Persistence::access_current()
{
	return PlayerSetup::access_current().access_active_game_slot()->persistence;
}

Persistence const& Persistence::get_current()
{
	return PlayerSetup::get_current().get_active_game_slot()->persistence;
}

Persistence * Persistence::access_current_if_exists()
{
	if (auto * p = PlayerSetup::access_current_if_exists())
	{
		return &p->access_active_game_slot()->persistence;
	}
	error(TXT("no persistence available"));
	return nullptr;
}

void Persistence::copy_from(Persistence const& _source)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);
	Concurrency::ScopedSpinLock lockSource(_source.persistenceLock, true);

	allEXMs = _source.allEXMs;
	cache_exms();
	
	lastUsedSetup = _source.lastUsedSetup;

	infoProgress = _source.infoProgress;

	experienceToSpend = _source.experienceToSpend;
	meritPointsToSpend = _source.meritPointsToSpend;

	killInfos = _source.killInfos;

	progress = _source.progress;

	weaponsInArmouryColumns = _source.weaponsInArmouryColumns;
	weaponsInArmoury.clear();
	for_every(w, _source.weaponsInArmoury)
	{
		weaponsInArmoury.push_back(WeaponInArmoury(this));
		auto& wia = weaponsInArmoury.get_last();
		wia.at = w->at;
		wia.setup.copy_from(w->setup);
	}
}

void Persistence::start_pilgrimage()
{
	pilgrimageInProgress = true;
}

void Persistence::end_pilgrimage()
{
	pilgrimageInProgress = false;
}

float experienceGainedShowForTime = 60.0f;

void Persistence::remove_too_old_experiences_gained()
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	while (!experienceGained.is_empty())
	{
		if (experienceGained.get_last().when.get_time_since() > experienceGainedShowForTime)
		{
			experienceGained.pop_back();
			++experienceGainedVer;
		}
		else
		{
			break;
		}
	}
}

void Persistence::do_for_every_experience_gained(int _limit, std::function<void(Energy const& _amount)> _do)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	remove_too_old_experiences_gained();

	int left = _limit;
	for_every(eg, experienceGained)
	{
		if (experienceGained.get_last().when.get_time_since() <= experienceGainedShowForTime)
		{
			if (left >= 0)
			{
				_do(eg->amount);
			}
		}
		--left;
		if (left < 0)
		{
			break;
		}
	}
}

RangeInt Persistence::get_weapons_in_armoury_columns_range() const
{
	RangeInt columnRange = RangeInt(0);
	int sideColumns = weaponsInArmouryColumns - 1;
	if (sideColumns > 0)
	{
		if ((sideColumns % 2) == 0)
		{
			sideColumns = sideColumns / 2;
			columnRange.include(-sideColumns);
			columnRange.include(sideColumns);
		}
		else
		{
			sideColumns = (sideColumns - 1) / 2; // for 3 we get 2/2 = 1, for 1 we get 0/2 = 0
			columnRange.include(-(sideColumns + 1)); // odd column goes to the left
			columnRange.include(sideColumns);
		}
	}
	return columnRange;
}

void Persistence::setup_weapons_in_armoury_for_game(Framework::IModulesOwner* _armoury, OUT_ Array<ExistingWeaponFromArmoury>* _existingWeapons, bool _armouryIgnoreExistingWeapons)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	RangeInt columnRange = get_weapons_in_armoury_columns_range();

	Array<ModuleEquipments::Gun*> meGuns;
	if (_armoury && ! _armouryIgnoreExistingWeapons)
	{
		// when the armoury is in use, world should be really small
		// scan the world for all the weapons
		if (auto* subWorld = _armoury->get_in_sub_world())
		{
			for_every_ptr(imo, subWorld->get_objects())
			{
				if (imo->get_presence() && imo->get_presence()->get_in_room())
				{
					if (auto* w = imo->get_gameplay_as<ModuleEquipments::Gun>())
					{
						meGuns.push_back(w);
					}
				}
			}
		}
	}

	int id = 1;
	for_every(wia, weaponsInArmoury)
	{
		wia->id = id;
		++id;

		wia->alreadyExists = false;
		Optional<VectorInt2> alreadyExistsAt; // we may have some weapons at the slots already, make sure we know what slots are in use
		{
			for_every_ptr(g, meGuns)
			{
				if (wia->imo == g->get_owner() ||
					g->get_weapon_setup() == wia->setup)
				{
					wia->alreadyExists = true;

					bool alreadyInExistingWeapons = false;
					if (_existingWeapons)
					{
						for_every(ew, *_existingWeapons)
						{
							if (ew->imo == g->get_owner())
							{
								ew->id = wia->id.get(); // set, see above
								ew->imo = g->get_owner();
								wia->imo = g->get_owner();
								if (ew->at.is_set())
								{
									alreadyExistsAt = ew->at;
								}
								alreadyInExistingWeapons = true;
								break;
							}
						}
						if (!alreadyInExistingWeapons)
						{
							_existingWeapons->push_back();
							auto& ew = _existingWeapons->get_last();
							ew.id = wia->id.get(); // set, see above
							ew.imo = g->get_owner();
							wia->imo = g->get_owner();
						}
					}
					meGuns.remove_at(for_everys_index(g));
					break;
				}
			}
		}

		if (wia->alreadyExists)
		{
			wia->at = alreadyExistsAt;
		}
		else
		{
			// remove from the rack, will be put in later
			if (wia->at.is_set() &&
				!columnRange.does_contain(wia->at.get().x))
			{
				wia->at.clear();
			}
		}
	}

	// do two passes to check if weapons occupy the same place
	// in first pass move other weapons to different place, already existing have a priority
	// in second pass check all against all
	for_count(int, checkPass, 2)
	{
		for_every(wia, weaponsInArmoury)
		{
			if (checkPass == 0 && wia->alreadyExists)
			{
				continue;
			}
			// if we're placed in the same spot as something else, remove us from there
			if (wia->at.is_set())
			{
				for_every(wiab, weaponsInArmoury)
				{
					if (wiab == wia)
					{
						break;
					}

					if (wiab->at.is_set() &&
						wiab->at.get() == wia->at.get())
					{
						wia->at.clear();
						break;
					}
				}
			}
		}
	}

	// put all weapons into the rack, try to find a valid spot
	// unless they already exist in the world, ignore these
	for_every(wia, weaponsInArmoury)
	{
		if (!wia->alreadyExists &&
			!wia->at.is_set())
		{
			bool foundPlacement = false;
			if (!foundPlacement)
			{
				// start with 2 columns, try to add there and only if couldn't find, move further
				// this is not optimal, but should be enough as we don't expect too many weapons/columns
				RangeInt workInColumnRange(-1, 0);
				workInColumnRange.min = max(workInColumnRange.min, columnRange.min);
				workInColumnRange.max = min(workInColumnRange.max, columnRange.max);
				while (!foundPlacement)
				{
					// start top row and go sideways, repeat for lowers in workInColumnRange
					VectorInt2 at = VectorInt2::zero;
					at.y = get_weapons_in_armoury_rows() - 1;
					while (!foundPlacement)
					{
						// fill from top
						at.x = 0;
						while (!foundPlacement)
						{
							if (workInColumnRange.does_contain(at.x))
							{
								bool isClear = true;
								for_every(owia, weaponsInArmoury)
								{
									if (owia->at.is_set() && owia->at.get() == at)
									{
										isClear = false;
										break;
									}
								}
								if (isClear)
								{
									foundPlacement = true;
									wia->at = at;
								}
							}
							if (!foundPlacement)
							{
								if (at.x == 0)
								{
									at.x = -1;
								}
								else if (at.x < 0)
								{
									at.x = -at.x;
								}
								else
								{
									at.x = -at.x - 1;
								}
								if (abs(at.x) > abs(workInColumnRange.min) &&
									abs(at.x) > abs(workInColumnRange.max))
								{
									// for sure we won't fit
									break;
								}
							}
						}

						if (!foundPlacement)
						{
							--at.y;
							if (at.y < 0)
							{
								break;
							}
						}
					}

					if (workInColumnRange.min <= columnRange.min &&
						workInColumnRange.max >= columnRange.max)
					{
						// we have no more columns
						break;
					}
					if (workInColumnRange.min > columnRange.min)
					{
						-- workInColumnRange.min;
					}
					if (workInColumnRange.max < columnRange.max)
					{
						++ workInColumnRange.max;
					}
				}
			}

			if (!foundPlacement)
			{
				// go through all in sides first
				VectorInt2 at = VectorInt2::zero;
				while (!foundPlacement)
				{
					// fill from top
					at.y = get_weapons_in_armoury_rows() - 1;
					while (!foundPlacement)
					{
						bool isClear = true;
						for_every(owia, weaponsInArmoury)
						{
							if (owia->at.is_set() && owia->at.get() == at)
							{
								isClear = false;
								break;
							}
						}
						if (isClear)
						{
							foundPlacement = true;
							wia->at = at;
						}
						if (!foundPlacement)
						{
							--at.y;
							if (at.y < 0)
							{
								break;
							}
						}
					}

					if (!foundPlacement)
					{
						if (at.x == 0)
						{
							at.x = -1;
						}
						else if (at.x < 0)
						{
							at.x = -at.x;
						}
						else
						{
							at.x = -at.x - 1;
						}
					}
				}
			}
		}
	}
}

bool Persistence::discard_invalid_weapons_from_armoury()
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	RangeInt columnRange = get_weapons_in_armoury_columns_range();

	bool somethingChanged = false;

	bool keepGoing = true;
	while (keepGoing)
	{
		keepGoing = false;
		for_every(wia, weaponsInArmoury)
		{
			if (!wia->at.is_set() ||
				!columnRange.does_contain(wia->at.get().x))
			{
				somethingChanged = true;
				weaponsInArmoury.remove(for_everys_iterator(wia));
				keepGoing = true;
				break;
			}
		}
	}

	return somethingChanged;
}

void Persistence::add_weapon_to_armoury(WeaponSetup const& _setup, bool _allowDuplicates, bool _allowTooMany, Framework::IModulesOwner* _imo, Optional<WeaponOnPilgrimInfo> const& _onPilgrimInfo)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	if (!_allowDuplicates)
	{
		for_every(wia, weaponsInArmoury)
		{
			if (wia->setup == _setup)
			{
				wia->imo = _imo;
				wia->onPilgrimInfo = _onPilgrimInfo.get(WeaponOnPilgrimInfo());
				// duplicate!
				return;
			}
		}
	}

	int maxWeapons = get_max_weapons_in_armoury(); // there was hardcoded 12 but what was it used for?

	if (weaponsInArmoury.get_size() >= maxWeapons && ! _allowTooMany)
	{
		// won't be added
		// this shouldn't happen but if someone finds a way to store multiple weapons over and over again, we should prevent putting too many weapons
		return;
	}

	weaponsInArmoury.push_back(WeaponInArmoury(this));
	auto& wia = weaponsInArmoury.get_last();
	wia.setup.copy_from(_setup);
	wia.imo = _imo;
	wia.onPilgrimInfo = _onPilgrimInfo.get(WeaponOnPilgrimInfo());
}

bool Persistence::move_weapon_in_armoury_to_end(WeaponSetup const& _setup)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	for_every(wia, weaponsInArmoury)
	{
		if (wia->setup == _setup)
		{
			WeaponInArmoury w(*wia);
			weaponsInArmoury.remove(for_everys_iterator(wia));
			weaponsInArmoury.push_back(w);
			return true;
		}
	}

	return false;
}

bool Persistence::remove_weapon_from_armoury(int _id)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	for_every(wia, weaponsInArmoury)
	{
		if (wia->id.is_set() && wia->id.get() == _id)
		{
			weaponsInArmoury.remove(for_everys_iterator(wia));
			return true;
		}
	}

	return false;
}

bool Persistence::update_weapon_in_armoury(int _id, WeaponSetup const& _setup, Optional<VectorInt2> const& _at, Optional<WeaponOnPilgrimInfo> const & _onPilgrimInfo)
{
	Concurrency::ScopedSpinLock lock(persistenceLock, true);

	for_every(wia, weaponsInArmoury)
	{
		if (wia->id.is_set() && wia->id.get() == _id)
		{
			bool result = false;
			auto storeOnPilgrimInfo = _onPilgrimInfo.get(WeaponOnPilgrimInfo());
			if (wia->at != _at ||
				wia->setup != _setup ||
				wia->onPilgrimInfo != storeOnPilgrimInfo)
			{
				result = true;
			}
			wia->at = _at;
			wia->setup.copy_from(_setup);
			wia->onPilgrimInfo = storeOnPilgrimInfo;
			return result;
		}
	}

	weaponsInArmoury.push_back(WeaponInArmoury(this));
	auto& wia = weaponsInArmoury.get_last();
	wia.id = _id;
	wia.at = _at;
	wia.setup.copy_from(_setup);
	wia.onPilgrimInfo = _onPilgrimInfo.get(WeaponOnPilgrimInfo());
	return true;
}

//

void Persistence::UsedSetup::clear()
{
	for_count(int, hIdx, Hand::MAX)
	{
		activeEXMs[hIdx] = Name::invalid();
		passiveEXMs[hIdx].clear();
	}
	permanentEXMs.clear();
}

bool Persistence::UsedSetup::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;

	for_every(contNode, _node->children_named(_childName))
	{
		for_count(int, hIdx, Hand::MAX)
		{
			for_every(handNode, contNode->children_named(hIdx == Hand::Left ? TXT("leftHand") : TXT("rightHand")))
			{
				for_every(node, handNode->children_named(TXT("active")))
				{
					activeEXMs[hIdx] = node->get_name_attribute(TXT("exm"), activeEXMs[hIdx]);
				}
				for_every(node, handNode->children_named(TXT("passive")))
				{
					Name exm = node->get_name_attribute(TXT("exm"));
					if (exm.is_valid())
					{
						passiveEXMs[hIdx].push_back(exm);
					}
				}
			}
		}
		for_every(permNode, contNode->children_named(TXT("permanentEXMs")))
		{
			for_every(node, permNode->children_named(TXT("permanent")))
			{
				Name exm = node->get_name_attribute(TXT("exm"));
				if (exm.is_valid())
				{
					permanentEXMs.push_back(exm);
				}
			}
		}
	}

	return result;
}

bool Persistence::UsedSetup::save_to_xml_child_node(IO::XML::Node * _node, tchar const* _childName) const
{
	bool result = true;

	auto* contNode = _node->add_node(_childName);

	for_count(int, hIdx, Hand::MAX)
	{
		auto* handNode = contNode->add_node(hIdx == Hand::Left ? TXT("leftHand") : TXT("rightHand"));
		if (activeEXMs[hIdx].is_valid())
		{
			auto* node = handNode->add_node(TXT("active"));
			node->set_attribute(TXT("exm"), activeEXMs[hIdx].to_string());
		}
		for_every(exm, passiveEXMs[hIdx])
		{
			auto* node = handNode->add_node(TXT("passive"));
			node->set_attribute(TXT("exm"), exm->to_string());
		}
	}
	{
		auto* permNode = contNode->add_node(TXT("permanentEXMs"));
		for_every(exm, permanentEXMs)
		{
			auto* node = permNode->add_node(TXT("permanent"));
			node->set_attribute(TXT("exm"), exm->to_string());
		}
	}

	return result;
}

//--

void Persistence::WeaponOnPilgrimInfo::fill(Framework::IModulesOwner* pilgrim, Framework::IModulesOwner* item)
{
	an_assert(pilgrim);
	an_assert(item);

	onPilgrim = false;
	mainWeaponForHand.clear();
	inPocket.clear();

	if (item->get_presence()->is_attached_at_all_to(pilgrim))
	{
		onPilgrim = true;
		if (auto* mp = pilgrim->get_gameplay_as<ModulePilgrim>())
		{
			for_count(int, hIdx, Hand::MAX)
			{
				Hand::Type hand = (Hand::Type)hIdx;
				if (mp->get_main_equipment(hand) == item)
				{
					mainWeaponForHand = hand;
				}
			}
			if (!mainWeaponForHand.is_set())
			{
				int inPocketIdx = mp->get_in_pocket_index(item);
				if (inPocketIdx != NONE)
				{
					inPocket = inPocketIdx;
				}
			}
		}
	}
}
