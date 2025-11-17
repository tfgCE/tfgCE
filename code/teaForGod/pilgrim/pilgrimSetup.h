#pragma once

#include "..\teaEnums.h"
#include "..\teaForGod.h"

#include "..\game\energy.h"
#include "..\game\weaponPart.h"
#include "..\game\weaponSetup.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\concurrency\mrswLock.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\hand.h"
#include "..\..\core\types\name.h"

#include "..\..\framework\library\libraryName.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	struct PilgrimHandSetupComparison
	{
		bool exms = false;
		bool equipment = false;
	};

	struct PilgrimSetupComparison
	{
		PilgrimHandSetupComparison hands[Hand::MAX];
	};

	struct PilgrimHandEXMSetup
	{
		Name exm;

		PilgrimHandEXMSetup() {}
		explicit PilgrimHandEXMSetup(Name const & _exm) : exm(_exm) {}

		bool is_set() const { return exm.is_valid(); }
		void clear();

		bool load_from_xml(IO::XML::Node const * _node);
		bool save_to_xml(IO::XML::Node * _node) const;

		bool operator == (PilgrimHandEXMSetup const & _other) const;
		bool operator != (PilgrimHandEXMSetup const& _other) const { return !(operator==(_other)); }
	};

	struct PilgrimHandSetup
	: public SafeObject<PilgrimHandSetup>
	{
		// locks are here to allow modifying when creating

		ArrayStatic<PilgrimHandEXMSetup, MAX_PASSIVE_EXMS> passiveEXMs;
		PilgrimHandEXMSetup activeEXM;
		mutable Concurrency::SpinLock exmsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimHandSetup.exmsLock"));

		WeaponSetup weaponSetup;
		mutable Concurrency::SpinLock weaponSetupLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimHandSetup.weaponSetupLock"));

		PilgrimHandSetup(Persistence* _persistence);
		~PilgrimHandSetup();

		bool load_from_xml(IO::XML::Node const * _node);
		bool save_to_xml(IO::XML::Node * _node) const;
		bool resolve_library_links();

		void copy_from(PilgrimHandSetup const& _other);
		void copy_presetable_from(PilgrimHandSetup const& _other);
		void copy_exms_from(PilgrimHandSetup const& _other);
		void copy_exms_from(ArrayStatic<PilgrimHandEXMSetup, MAX_PASSIVE_EXMS> const& _passiveEXMs, PilgrimHandEXMSetup const& _activeEXM);
		void remove_exms();
		void keep_only_unlocked_exms();

		void make_sure_there_is_no_exm(Name const& _exmId);

		void copy_weapon_setup_from(PilgrimHandSetup const& _other);

		int get_exms_count() const;
		bool has_exm_installed(Name const& _exm) const;

		static bool compare(PilgrimHandSetup const & _a, PilgrimHandSetup const & _b, OUT_ PilgrimHandSetupComparison & _comparisonResult);

	private:
		// do not implement
		PilgrimHandSetup(PilgrimHandSetup const& _other);
		PilgrimHandSetup& operator=(PilgrimHandSetup const& _other);
	};

	struct PilgrimInitialEnergyLevels
	{
		Energy health = -Energy::one();
		Energy hand[Hand::MAX] = { -Energy::one(), -Energy::one() };

		bool load_from_xml(IO::XML::Node const* _node);
		bool save_to_xml(IO::XML::Node* _node) const;

		PilgrimInitialEnergyLevels set_defaults_if_missing(Energy const& _defaultHealth, Energy const& _defaultHandLeft, Energy const& _defaultHandRight) const;
	};

	/**
	 *	Pilgrim setup contains all things that player may change to the pilgrim, this can be the default setup or current one (during the level)
	 *
	 *		1.	Main one is located in PlayerSetup in one of the game slots. 
	 *			This is the setup that player sets in PilgrimageSetup/QuickGameSetup.
	 *			It is then copied to ModulePilgrim.
	 *
	 *		2.	Tutorial one is located in Tutorial, can be accessed through PlayerSetup though.
	 *			Most likely it is loaded from tutorial data.
	 *			If required, is copied to ModulePilgrim.
	 *	
	 *		3.	In-game. Located in ModulePilgrim.
	 *			It can be modified during the game.
	 *			Doesn't get to the PlayerSetup.
	 *
	 *	Pilgrim setup is bound to persistence (to know from where to use things). Unless it is a preset, then it has no persistence (and items that make sense for persistence, like weapon parts, cannot be used)
	 */
	class PilgrimSetup
	: public SafeObject<PilgrimSetup>
	{
	public:
		PilgrimSetup(Persistence* _persistence);
		virtual ~PilgrimSetup();

		Persistence* get_persistence() const { return persistence; }

		void increase_weapon_part_usage();

		void copy_from(PilgrimSetup const& _other);
		void copy_presetable_from(PilgrimSetup const& _other);
		void copy_exms_from(Array<Name> const& _exms);
		void copy_exms_from(PilgrimSetup const& _other);
		void copy_weapon_setup_from(PilgrimSetup const& _other);

		PilgrimHandSetup const & get_hand(Hand::Type _hand) const { return _hand == Hand::Left? leftHand : rightHand; }
		PilgrimHandSetup & access_hand(Hand::Type _hand) { return _hand == Hand::Left ? leftHand : rightHand; }

		void set_initial_energy_levels(PilgrimInitialEnergyLevels const & _initialEnveryLevels) { initialEnergyLevels = _initialEnveryLevels; }
		PilgrimInitialEnergyLevels const& get_initial_energy_levels() const { return initialEnergyLevels; }

		bool load_from_xml(IO::XML::Node const * _node);
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName = TXT("pilgrimSetup"));
		bool save_to_xml(IO::XML::Node * _node) const;
		bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childName = TXT("pilgrimSetup")) const;
		bool resolve_library_links();

		Concurrency::MRSWLock & access_lock() const { return accessLock; } // this is where we use stuff manually

		static bool compare(PilgrimSetup const & _a, PilgrimSetup const & _b, OUT_ PilgrimSetupComparison & _comparisonResult);

		int get_version() const { return version; }

		void update_passive_exm_slot_count(int _slotCount);

	private:
		Persistence* persistence;
		int version = 0; // this is important only for the main one, to update with defaults? although at one time this should be gone
		PilgrimHandSetup leftHand;
		PilgrimHandSetup rightHand;

		PilgrimInitialEnergyLevels initialEnergyLevels;

		mutable Concurrency::MRSWLock accessLock; // has to be locked to copy, everyone should have their own copy, if possible and manage it and maybe just have a "pending buffer" that can be access by others

		// do not implement
		PilgrimSetup(PilgrimSetup const& _other);
		PilgrimSetup& operator = (PilgrimSetup const& _other);
	
	private: friend struct PersistenceWeaponParts;
	   void on_weapon_part_discard(WeaponPart* _wp);

	};

	struct PilgrimSetupPreset
	{
		String id;
		PilgrimSetup setup;

		PilgrimSetupPreset()
		: setup(nullptr)
		{}

		PilgrimSetupPreset(PilgrimSetupPreset const& _other)
		: setup(nullptr)
		{
			id = _other.id;
			setup.copy_from(_other.setup);
		}
		
		PilgrimSetupPreset& operator=(PilgrimSetupPreset const& _other)
		{
			id = _other.id;
			setup.copy_from(_other.setup);
			return *this;
		}

		static bool is_name_default(String const & name) { return String::does_contain(name.to_char(), TXT("[")); }
		static void dedefaultise(REF_ String& name) { name.replace_inline(String(TXT("[")), String::empty()); name.replace_inline(String(TXT("]")), String::empty()); }
		bool is_default() const { return is_name_default(id); }

		static int compare(void const* _a, void const* _b)
		{
			PilgrimSetupPreset const& a = *(plain_cast<PilgrimSetupPreset>(_a));
			PilgrimSetupPreset const& b = *(plain_cast<PilgrimSetupPreset>(_b));
			bool aIsDefault = a.is_default();
			bool bIsDefault = b.is_default();
			if (aIsDefault && !bIsDefault)
			{
				return A_BEFORE_B;
			}
			if (! aIsDefault && bIsDefault)
			{
				return B_BEFORE_A;
			}
			return String::diff_icase(a.id, b.id);
		};
	};
};
