#pragma once

#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\memory\refCountObjectPtr.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"

#include "..\game\weaponPart.h"

namespace TeaForGodEmperor
{
	class EXMType;

	/**
	 *	Pilgrim inventory, most of the time holds just inventory items
	 *	But can be arranged to hold items for transfer (all weapon parts from items)
	 */
	struct PilgrimInventory
	{
	public:
		static void sync_copy_exms(PilgrimInventory const& _src, PilgrimInventory& _dest);

	public: // EXMS
		mutable Concurrency::MRSWLock exmsLock;

		Array<Name> availableEXMs; // the ones that were unlocked by upgrade machine during the run
		Array<Name> permanentEXMs; // may repeat/be instanced numerous times

		int get_count_of_permanent_EXMs_unlocked_in_persistence() const; // out of installed here

	public: // WEAPON PARTS
		typedef int ID;
		enum Source
		{
			Unknown,
			Inventory, // in actual inventory - found, weapon broken into parts etc
			HeldItem, // not really in inventory but if we're requested to show all parts, other parts are marked like this
		};

		mutable Concurrency::SpinLock weaponPartsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimInventory.weaponsPartsLock"));
		struct PlacedPart
		{
			ID id; // unique within pilgrim inventory
			Source source = Source::Unknown;
			bool partOfMainEquipment = false;
			VectorInt2 at = VectorInt2::zero;
			RefCountObjectPtr<WeaponPart> weaponPart;
			
			// if from an item:
			SafePtr<Framework::IModulesOwner> item;
			WeaponPartAddress address;

			bool is_visible() const { return at.x >= 0 && at.y >= 0; }
		};
		Array<PlacedPart> weaponParts;

	public: // exms
		void make_exm_available(Name const& _exm);
		bool is_exm_available(Name const& _exm) const;

		void add_permanent_exm(Name const& _exm);
		int get_permanent_exm_count(Name const& _exm) const;
		bool has_permanent_exm(Name const& _exm) const;

	public: // weapon parts
		// this always adds a weapon part (if negative _at, it doesn't check)
		ID add_inventory_weapon_part(WeaponPart* _wp, Optional<VectorInt2> const& _at = NP, Optional<VectorInt2> const& _preferredGridSize = NP) { return add_weapon_part_internal(_wp, Source::Inventory, nullptr, WeaponPartAddress(), _at, _preferredGridSize, true); }
		// this updates existing weapon part or adds a new one
		ID add_weapon_part_from_item(WeaponPart* _wp, Framework::IModulesOwner* _item, WeaponPartAddress const & _address, Optional<VectorInt2> const& _at = NP, Optional<VectorInt2> const& _preferredGridSize = NP) { return add_weapon_part_internal(_wp, Source::HeldItem, _item, _address, _at, _preferredGridSize, false); }
		void place_weapon_part(ID _id, Optional<VectorInt2> const& _at = NP, Optional<VectorInt2> const& _preferredGridSize = NP);
		void remove_weapon_part(ID _id); // this should be called by pilgrim's module
		WeaponPart* get_weapon_part(ID _id) const;
		PlacedPart get_placed_weapon_part(ID _id) const;

		void keep_only_inventory_items(); // to make it useful for weapon crafting etc

		bool is_empty_at_weapon_part(VectorInt2 const& _at) const;

	private:
		ID get_free_id() const;

		ID add_weapon_part_internal(WeaponPart* _wp, Source _source, Framework::IModulesOwner* _item, WeaponPartAddress const& _address, Optional<VectorInt2> const& _at, Optional<VectorInt2> const& _preferredGridSize, bool _alwaysAdd);

	};
};

