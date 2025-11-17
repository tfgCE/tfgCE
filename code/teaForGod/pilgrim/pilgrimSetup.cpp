#include "pilgrimSetup.h"

#include "..\library\library.h"

#include "..\modules\gameplay\modulePilgrim.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\modulesOwner\modulesOwner.h"


//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// default pilgrim setup weapons
DEFINE_STATIC_NAME_STR(corePlasma, TXT("default pilgrim setup; weapon; plasma core"));

DEFINE_STATIC_NAME_STR(pistolChassis, TXT("default pilgrim setup; weapon; pistol; chassis"));
DEFINE_STATIC_NAME_STR(pistolMuzzle, TXT("default pilgrim setup; weapon; pistol; muzzle"));
DEFINE_STATIC_NAME_STR(pistolNode, TXT("default pilgrim setup; weapon; pistol; node"));

DEFINE_STATIC_NAME_STR(shotgunChassis, TXT("default pilgrim setup; weapon; shotgun; chassis"));
DEFINE_STATIC_NAME_STR(shotgunMuzzle, TXT("default pilgrim setup; weapon; shotgun; muzzle"));
DEFINE_STATIC_NAME_STR(shotgunNode, TXT("default pilgrim setup; weapon; shotgun; node"));

//

using namespace TeaForGodEmperor;

//

bool PilgrimHandEXMSetup::operator == (PilgrimHandEXMSetup const & _other) const
{
	return exm == _other.exm;
}

void PilgrimHandEXMSetup::clear()
{
	exm = Name::invalid();
}

bool PilgrimHandEXMSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	clear();

	warn_loading_xml_on_assert(! _node->has_attribute(TXT("id")), _node, TXT("\"id\" is deprecated, use \"exm\""));

	exm = _node->get_name_attribute(TXT("id"), exm);
	exm = _node->get_name_attribute(TXT("exm"), exm);
	error_loading_xml_on_assert(exm.is_valid(), result, _node, TXT("no \"exm\" provided"));

	return result;
}

bool PilgrimHandEXMSetup::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;

	_node->set_attribute(TXT("exm"), exm.to_string());

	return result;
}

//

PilgrimHandSetup::PilgrimHandSetup(Persistence* _persistence)
: SafeObject<PilgrimHandSetup>(this)
, weaponSetup(_persistence)
{
	SET_EXTRA_DEBUG_INFO(passiveEXMs, TXT("PilgrimHandSetup.passiveEXMs"));
}

PilgrimHandSetup::~PilgrimHandSetup()
{
	make_safe_object_unavailable();
}

void PilgrimHandSetup::copy_exms_from(PilgrimHandSetup const& _other)
{
	an_assert(exmsLock.is_locked_on_this_thread());
	an_assert(_other.exmsLock.is_locked_on_this_thread());
	passiveEXMs = _other.passiveEXMs;
	activeEXM = _other.activeEXM;
}

void PilgrimHandSetup::copy_exms_from(ArrayStatic<PilgrimHandEXMSetup, MAX_PASSIVE_EXMS> const& _passiveEXMs, PilgrimHandEXMSetup const& _activeEXM)
{
	an_assert(exmsLock.is_locked_on_this_thread());
	passiveEXMs = _passiveEXMs;
	activeEXM = _activeEXM;
}

void PilgrimHandSetup::remove_exms()
{
	an_assert(exmsLock.is_locked_on_this_thread());
	passiveEXMs.clear();
	activeEXM.clear();
}

void PilgrimHandSetup::keep_only_unlocked_exms()
{
	an_assert(exmsLock.is_locked_on_this_thread());
	if (Persistence* persistence = Persistence::access_current_if_exists())
	{
		Concurrency::ScopedSpinLock lock(persistence->access_lock(), true);
		if (activeEXM.is_set())
		{
			if (!persistence->get_unlocked_exms().does_contain(activeEXM.exm))
			{
				activeEXM.clear();
			}
		}
		for(int i = 0; i < passiveEXMs.get_size(); ++ i)
		{
			auto& pexm = passiveEXMs[i];
			if (pexm.is_set() &&
				!persistence->get_unlocked_exms().does_contain(pexm.exm))
			{
				passiveEXMs.remove_at(i);
				--i;
			}
		}
	}
	else
	{
		passiveEXMs.clear();
		activeEXM.clear();
	}
}

void PilgrimHandSetup::copy_from(PilgrimHandSetup const & _other)
{
	{
		Concurrency::ScopedSpinLock lock(exmsLock, true);
		Concurrency::ScopedSpinLock lockOther(_other.exmsLock, true);
		copy_exms_from(_other);
	}
	copy_weapon_setup_from(_other);
}

void PilgrimHandSetup::copy_presetable_from(PilgrimHandSetup const & _other)
{
	{
		Concurrency::ScopedSpinLock lock(exmsLock, true);
		Concurrency::ScopedSpinLock lockOther(_other.exmsLock, true);
		copy_exms_from(_other);
	}
}

bool PilgrimHandSetup::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	// empty!
	passiveEXMs.clear();
	activeEXM.clear();

	for_every(node, _node->children_named(TXT("exms")))
	{
		for_every(subNode, node->children_named(TXT("passive")))
		{
			PilgrimHandEXMSetup exm;
			if (exm.load_from_xml(subNode))
			{
				if (passiveEXMs.has_place_left())
				{
					passiveEXMs.push_back(exm);
				}
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

	if (auto* node = _node->first_child_named(TXT("weaponSetup")))
	{
		result &= weaponSetup.load_from_xml(node);
	}

	return result;
}

bool PilgrimHandSetup::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;

	{
		auto* node = _node->add_node(TXT("exms"));
		for_every(exm, passiveEXMs)
		{
			if (exm->is_set())
			{
				auto* subNode = node->add_node(TXT("passive"));
				exm->save_to_xml(subNode);
			}
		}
		if (activeEXM.is_set())
		{
			auto* subNode = node->add_node(TXT("active"));
			activeEXM.save_to_xml(subNode);
		}
	}

	{
		auto* node = _node->add_node(TXT("weaponSetup"));
		weaponSetup.save_to_xml(node);
	}

	return result;
}

bool PilgrimHandSetup::resolve_library_links()
{
	bool result = true;

	result &= weaponSetup.resolve_library_links();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		result = true;
	}
#endif

	return result;
}

int PilgrimHandSetup::get_exms_count() const
{
	Concurrency::ScopedSpinLock lock(exmsLock);

	int count = 0;
	for_every(exm, passiveEXMs)
	{
		count += exm->is_set() ? 1 : 0;
	}
	count += activeEXM.is_set() ? 1 : 0;
	return count;
}

bool PilgrimHandSetup::has_exm_installed(Name const& _exm) const
{
	Concurrency::ScopedSpinLock lock(exmsLock);

	for_every(exm, passiveEXMs)
	{
		if (exm->is_set() && exm->exm == _exm)
		{
			return true;
		}
	}
	if (activeEXM.is_set() && activeEXM.exm == _exm)
	{
		return true;
	}
	return false;
}

void PilgrimHandSetup::copy_weapon_setup_from(PilgrimHandSetup const& _other)
{
	Concurrency::ScopedSpinLock lock(weaponSetupLock);
	Concurrency::ScopedSpinLock otherLock(_other.weaponSetupLock);
	
	weaponSetup.copy_from(_other.weaponSetup);
}

bool PilgrimHandSetup::compare(PilgrimHandSetup const & _a, PilgrimHandSetup const & _b, OUT_ PilgrimHandSetupComparison & _comparisonResult)
{
	bool result = true;

	_comparisonResult.exms = true;
	if (_a.activeEXM != _b.activeEXM)
	{
		_comparisonResult.exms = false;
		result = false;
	}
	{
		PilgrimHandEXMSetup emptyEXM;
		for_count(int, i, max(_a.passiveEXMs.get_size(), _b.passiveEXMs.get_size()))
		{
			auto* a = _a.passiveEXMs.is_index_valid(i) ? &_a.passiveEXMs[i] : &emptyEXM;
			auto* b = _b.passiveEXMs.is_index_valid(i) ? &_b.passiveEXMs[i] : &emptyEXM;

			if (a->exm != b->exm)
			{
				_comparisonResult.exms = false;
				result = false;
				break;
			}
		}
	}

	_comparisonResult.equipment = true;
	if (_a.weaponSetup != _b.weaponSetup)
	{
		_comparisonResult.equipment = false;
		result = false;
	}

	return result;
}

void PilgrimHandSetup::make_sure_there_is_no_exm(Name const& _exm)
{
	scoped_call_stack_info(TXT("make_sure_there_is_no_exm"));
	for_count(int, i, passiveEXMs.get_size())
	{
		if (passiveEXMs[i].exm == _exm)
		{
			passiveEXMs.remove_at(i);
			passiveEXMs.push_back(PilgrimHandEXMSetup());
		}
	}
	if (activeEXM.exm == _exm)
	{
		activeEXM.clear();
	}
}

//

bool PilgrimInitialEnergyLevels::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	result &= health.load_from_attribute_or_from_child(_node, TXT("health"));
	result &= hand[Hand::Left].load_from_attribute_or_from_child(_node, TXT("handLeft"));
	result &= hand[Hand::Right].load_from_attribute_or_from_child(_node, TXT("handRight"));

	return result;
}

bool PilgrimInitialEnergyLevels::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	_node->set_attribute(TXT("health"), health.as_string_auto_decimals());
	_node->set_attribute(TXT("handLeft"), hand[Hand::Left].as_string_auto_decimals());
	_node->set_attribute(TXT("handRight"), hand[Hand::Right].as_string_auto_decimals());

	return result;
}

PilgrimInitialEnergyLevels PilgrimInitialEnergyLevels::set_defaults_if_missing(Energy const& _defaultHealth, Energy const& _defaultHandLeft, Energy const& _defaultHandRight) const
{
	PilgrimInitialEnergyLevels result = *this;

	// if not set, use default setup + "at least"
	if (! result.health.is_positive())
	{
		result.health = _defaultHealth;
	}
	for_count(int, iHand, Hand::MAX)
	{
		if (result.hand[iHand].is_negative()) // allow zero
		{
			result.hand[iHand] = iHand == Hand::Left? _defaultHandLeft : _defaultHandRight;
		}
	}

	return result;
}

//

PilgrimSetup::PilgrimSetup(Persistence* _persistence)
: SafeObject<PilgrimSetup>(this)
, persistence(_persistence)
, leftHand(_persistence)
, rightHand(_persistence)
{
}

void PilgrimSetup::copy_from(PilgrimSetup const & _other)
{
	Concurrency::ScopedMRSWLockRead otherLock(_other.accessLock);
	Concurrency::ScopedMRSWLockWrite lock(accessLock);

	version = _other.version;
	leftHand.copy_from(_other.leftHand);
	rightHand.copy_from(_other.rightHand);
	initialEnergyLevels = _other.initialEnergyLevels;
}

void PilgrimSetup::copy_presetable_from(PilgrimSetup const & _other)
{
	Concurrency::ScopedMRSWLockRead otherLock(_other.accessLock);
	Concurrency::ScopedMRSWLockWrite lock(accessLock);

	version = _other.version;
	leftHand.copy_presetable_from(_other.leftHand);
	rightHand.copy_presetable_from(_other.rightHand);
}

PilgrimSetup::~PilgrimSetup()
{
	make_safe_object_unavailable();
}

// none is when there is no file
#define VER_NONE	0 
#define VER_BASE	1 
#define VER_EXM		2
#define VER_WEAPONS	3
#define CURRENT_VERSION VER_WEAPONS

bool PilgrimSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	{
		Concurrency::ScopedMRSWLockWrite lock(accessLock);

		version = _node->get_int_attribute_or_from_child(TXT("version"), version);
		for_every(ielNode, _node->children_named(TXT("energyLevels")))
		{
			result &= initialEnergyLevels.load_from_xml(ielNode);
		}
		for_every(handNode, _node->children_named(TXT("leftHand")))
		{
			result &= leftHand.load_from_xml(handNode);
		}
		for_every(handNode, _node->children_named(TXT("rightHand")))
		{
			result &= rightHand.load_from_xml(handNode);
		}
	}
	
	return result;
}

bool PilgrimSetup::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		result &= load_from_xml(node);
	}

	return result;
}

bool PilgrimSetup::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;

	{
		Concurrency::ScopedMRSWLockRead lock(accessLock);

		_node->set_int_attribute(TXT("version"), version);
		initialEnergyLevels.save_to_xml(_node->add_node(TXT("energyLevels")));
		leftHand.save_to_xml(_node->add_node(TXT("leftHand")));
		rightHand.save_to_xml(_node->add_node(TXT("rightHand")));
	}

	return result;
}

bool PilgrimSetup::save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childName) const
{
	bool result = true;

	auto* node = _node->add_node(_childName);
	result &= save_to_xml(node);

	return result;
}

bool PilgrimSetup::resolve_library_links()
{
	bool result = true;

	{
		Concurrency::ScopedMRSWLockWrite lock(accessLock);

		result &= leftHand.resolve_library_links();
		result &= rightHand.resolve_library_links();
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		result = true;
	}
#endif

	return result;
}

void PilgrimSetup::copy_exms_from(PilgrimSetup const& _other)
{
	Concurrency::ScopedMRSWLockRead otherLock(_other.accessLock);
	Concurrency::ScopedMRSWLockWrite lock(accessLock);

	{
		Concurrency::ScopedSpinLock lock(leftHand.exmsLock);
		Concurrency::ScopedSpinLock otherLock(_other.leftHand.exmsLock);
		leftHand.copy_exms_from(_other.leftHand);
	}
	{
		Concurrency::ScopedSpinLock lock(rightHand.exmsLock);
		Concurrency::ScopedSpinLock otherLock(_other.rightHand.exmsLock);
		rightHand.copy_exms_from(_other.rightHand);
	}
}

void PilgrimSetup::copy_weapon_setup_from(PilgrimSetup const& _other)
{
	Concurrency::ScopedMRSWLockRead otherLock(_other.accessLock);
	Concurrency::ScopedMRSWLockWrite lock(accessLock);

	leftHand.copy_weapon_setup_from(_other.leftHand);
	rightHand.copy_weapon_setup_from(_other.rightHand);
}

bool PilgrimSetup::compare(PilgrimSetup const & _a, PilgrimSetup const & _b, OUT_ PilgrimSetupComparison & _comparisonResult)
{
	bool result = true;
	for_count(int, h, Hand::MAX)
	{
		result &= PilgrimHandSetup::compare(_a.get_hand((Hand::Type)h), _b.get_hand((Hand::Type)h), OUT_ _comparisonResult.hands[h]);
	}
	return result;
}

void PilgrimSetup::on_weapon_part_discard(WeaponPart* _wp)
{
	for_count(int, i, Hand::MAX)
	{
		auto& hand = i == 0 ? leftHand : rightHand;
		Concurrency::ScopedSpinLock lock(hand.weaponSetupLock, true);
		hand.weaponSetup.on_weapon_part_discard(_wp);
	}
}

void PilgrimSetup::increase_weapon_part_usage()
{
	for_count(int, i, Hand::MAX)
	{
		auto& hand = i == 0 ? leftHand : rightHand;
		Concurrency::ScopedSpinLock lock(hand.weaponSetupLock, true);
		hand.weaponSetup.increase_weapon_part_usage();
	}
}

void PilgrimSetup::update_passive_exm_slot_count(int _slotCount)
{
	an_assert_log_always(_slotCount <= leftHand.passiveEXMs.get_max_size());
	an_assert_log_always(_slotCount <= rightHand.passiveEXMs.get_max_size());
	scoped_call_stack_info(TXT("update_passive_exm_slot_count"));

	Concurrency::ScopedMRSWLockWrite lock(accessLock);

	_slotCount = min(_slotCount, leftHand.passiveEXMs.get_max_size());
	_slotCount = min(_slotCount, rightHand.passiveEXMs.get_max_size());
	// always add empty! otherwise we will end up with reusing old ones as ArrayStatic reuses without calling constructor/destructor
	while (leftHand.passiveEXMs.get_size() < _slotCount && leftHand.passiveEXMs.has_place_left())
	{
		leftHand.passiveEXMs.push_back(PilgrimHandEXMSetup());
	}
	while (rightHand.passiveEXMs.get_size() < _slotCount && rightHand.passiveEXMs.has_place_left())
	{
		rightHand.passiveEXMs.push_back(PilgrimHandEXMSetup());
	}
	leftHand.passiveEXMs.set_size(_slotCount);
	rightHand.passiveEXMs.set_size(_slotCount);
}
