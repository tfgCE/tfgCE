#pragma once

#include "weaponPart.h"

namespace TeaForGodEmperor
{
	struct WeaponSetup;
	struct WeaponSetupTemplate;

	struct WeaponSetup
	{
	public:
		WeaponSetup(Persistence* _persistence); // if null, it is a dry address

		Persistence* get_persistence() const { return persistence.get(); }

		void clear_for(Persistence* _persistence);

		void copy_from(WeaponSetup const& _other);
		void swap_with(WeaponSetup & _other);

		void setup_with_template(WeaponSetupTemplate const& _template, Persistence* _persistenceForParts = nullptr);

		void copy_for_different_persistence(WeaponSetup const& _other, Optional<bool> const& _forStoring = NP);

		Framework::ItemType* get_item_type() const { return itemType.get(); }
		Array<UsedWeaponPart> const& get_parts() const { return parts; }
		WeaponPart* get_part_at(WeaponPartAddress const& _address) const;
		bool is_empty() const { return parts.is_empty(); }
		bool is_valid() const; // has chassis and core
		bool is_loose_parts() const { return looseParts.is_set(); }

		void set_loose_parts(Optional<int> const& _looseParts) { looseParts = _looseParts; }
		Optional<int> const & get_loose_parts() const { return looseParts; }

		void clear();
		bool does_contain(WeaponPart const * _part) const;
		void add_part(WeaponPartAddress const & _at, WeaponPart * _part, Optional<bool> const & _nonRandomised = NP, Optional<EnergyCoef> const & _damaged = NP, Optional<bool> const & _defaultMeshParams = NP);
		void add_nonrandomised_part(WeaponPartAddress const& _at, Framework::LibraryName const & _name);
		void set_item_type(Framework::ItemType const* _itemType) { itemType = _itemType; }
		void remove_part(WeaponPartAddress const& _at);
		void remove_part(WeaponPart * _wp);
		void on_weapon_part_discard(WeaponPart * _wp);

		void fill_with_random_parts_at(WeaponPartAddress const& _addr, Optional<bool> const& _useNonRandomisedParts, Array<WeaponPartType const*> const* _usingWeaponParts);

		void increase_weapon_part_usage();

	public:
		bool load_loose_parts_from_xml(IO::XML::Node const* _node);
		bool load_from_xml(IO::XML::Node const* _node);
		bool save_to_xml(IO::XML::Node* _node) const;
		bool resolve_library_links();

	public:
		bool operator == (WeaponSetup const& _other) const;
		bool operator != (WeaponSetup const& _other) const { return !(operator==(_other)); }

	private:
		SafePtr<Persistence> persistence;
		Array<UsedWeaponPart> parts;
		
		Optional<int> looseParts; // if loose parts is set, chassis is not required, all parts can be added anywhere, this is also used for creating loos weapon parts for loot
		
		Framework::UsedLibraryStored<Framework::ItemType> itemType; // if provided, ignores parts

		// do not implement
		WeaponSetup(WeaponSetup const& _other);
		WeaponSetup& operator=(WeaponSetup const& _other);

		int find_free_loose_part_idx(Optional<int> const& _tryIdx = NP) const;
	};

	struct WeaponSetupTemplate
	{
	public:
		struct WeaponPart
		{
			Framework::UsedLibraryStored<WeaponPartType> weaponPartType;
			bool nonRandomised = false;
			EnergyCoef damaged = EnergyCoef::zero();
			bool defaultMeshParams = false;

			TeaForGodEmperor::WeaponPart* create_weapon_part(Persistence* _persistence, Optional<int> const& _rgIndex = NP) const;
		};
		struct UsedWeaponPart
		: public WeaponPart
		{
			WeaponPartAddress at;
		};

		bool is_empty() const { return parts.is_empty(); }
		void clear();

		Array<UsedWeaponPart> const& get_parts() const { return parts; }

		bool is_loose_parts() const { return looseParts.is_set(); }
		Optional<int> const& get_loose_parts() const { return looseParts; }

		bool load_loose_parts_from_xml(IO::XML::Node const* _node);
		bool load_from_xml(IO::XML::Node const* _node);
		bool resolve_library_links();
		bool save_to_xml(IO::XML::Node* _node) const;

	private:
		Array<UsedWeaponPart> parts;

		Optional<int> looseParts; // if loose parts is set, chassis is not required, all parts can be added anywhere, this is also used for creating loos weapon parts for loot
	};
};

DECLARE_REGISTERED_TYPE(TeaForGodEmperor::WeaponSetup);

template <>
inline void ObjectHelpers<TeaForGodEmperor::WeaponSetup>::call_constructor_on(TeaForGodEmperor::WeaponSetup* ptr) { new (ptr)TeaForGodEmperor::WeaponSetup(nullptr); }

template <>
inline void ObjectHelpers<TeaForGodEmperor::WeaponSetup>::call_copy_on(void* ptr, void const* from) { plain_cast<TeaForGodEmperor::WeaponSetup>(ptr)->copy_from(*(plain_cast<TeaForGodEmperor::WeaponSetup>(from))); }

