#include "pilgrimInventory.h"

#include "..\game\game.h"
#include "..\game\persistence.h"
#include "..\modules\gameplay\moduleEquipment.h"

#include "..\..\core\concurrency\scopedSpinLock.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

void PilgrimInventory::sync_copy_exms(PilgrimInventory const& _src, PilgrimInventory& _dest)
{
	ASSERT_SYNC;

	Concurrency::ScopedMRSWLockRead lockSrc(_src.exmsLock);
	Concurrency::ScopedMRSWLockWrite lockDest(_dest.exmsLock);

	for_every(exm, _src.availableEXMs)
	{
		_dest.availableEXMs.push_back_unique(*exm);
	}
	for_every(exm, _src.permanentEXMs)
	{
		// not unique, may double, check how many there are to have the right amount
		int shouldBeAmount = 0;
		for_every(sexm, _src.permanentEXMs)
		{
			if (*sexm == *exm)
			{
				++shouldBeAmount;
			}
		}
		int isAmount = 0;
		for_every(dexm, _dest.permanentEXMs)
		{
			if (*dexm == *exm)
			{
				++isAmount;
			}
		}
		while (isAmount < shouldBeAmount)
		{
			_dest.permanentEXMs.push_back(*exm);
			++isAmount;
		}
	}
}

int PilgrimInventory::get_count_of_permanent_EXMs_unlocked_in_persistence() const
{
	Array<Name> leftToCheckInInventory;
	Array<Name> leftInPersistence;
	{
		Concurrency::ScopedMRSWLockRead lock(exmsLock);
		leftToCheckInInventory = permanentEXMs;
	}
	leftInPersistence = Persistence::get_current().get_permanent_exms();

	int unlockedInPersistence = 0;
	while (!leftToCheckInInventory.is_empty())
	{
		auto exm = leftToCheckInInventory.get_last();
		if (leftInPersistence.does_contain(exm))
		{
			++unlockedInPersistence;
			leftInPersistence.remove_fast(exm);
		}
		leftToCheckInInventory.pop_back();
	}

	return unlockedInPersistence;
}

void PilgrimInventory::add_permanent_exm(Name const& _exm)
{
	Concurrency::ScopedMRSWLockWrite lock(exmsLock);

	permanentEXMs.push_back(_exm); // not unique, may double
}

int PilgrimInventory::get_permanent_exm_count(Name const& _exm) const
{
	Concurrency::ScopedMRSWLockRead lock(exmsLock);

	int count = 0;
	for_every(exm, permanentEXMs)
	{
		if (*exm == _exm)
		{
			++count;
		}
	}
	return count;
}

bool PilgrimInventory::has_permanent_exm(Name const& _exm) const
{
	Concurrency::ScopedMRSWLockRead lock(exmsLock);

	for_every(exm, permanentEXMs)
	{
		if (*exm == _exm)
		{
			return true;
		}
	}
	return false;
}

void PilgrimInventory::make_exm_available(Name const& _exm)
{
	Concurrency::ScopedMRSWLockWrite lock(exmsLock);

	availableEXMs.push_back_unique(_exm);
}

bool PilgrimInventory::is_exm_available(Name const& _exm) const
{
	Concurrency::ScopedMRSWLockRead lock(exmsLock);

	return availableEXMs.does_contain(_exm);
}

WeaponPart* PilgrimInventory::get_weapon_part(ID _id) const
{
	Concurrency::ScopedSpinLock lock(weaponPartsLock, true);

	for_every(pp, weaponParts)
	{
		if (pp->id == _id)
		{
			return pp->weaponPart.get();
		}
	}
	
	return nullptr;
}

PilgrimInventory::PlacedPart PilgrimInventory::get_placed_weapon_part(ID _id) const
{
	for_every(pp, weaponParts)
	{
		if (pp->id == _id)
		{
			return *pp;
		}
	}

	return PlacedPart();
}

void PilgrimInventory::remove_weapon_part(ID _id)
{
	Concurrency::ScopedSpinLock lock(weaponPartsLock, true);

	for_every(pp, weaponParts)
	{
		if (pp->id == _id)
		{
			weaponParts.remove_at(for_everys_index(pp));
			break;
		}
	}
}

void PilgrimInventory::place_weapon_part(int _id, Optional<VectorInt2> const& _at, Optional<VectorInt2> const& _preferredGridSize)
{
	Concurrency::ScopedSpinLock lock(weaponPartsLock, true);

	// remove from where it was and place it outside, this will make it easier to use "is_empty_at_weapon_part")
	for_every(wp, weaponParts)
	{
		if (wp->id == _id)
		{
			wp->at = VectorInt2(-1, -1);
		}
	}

	if (_at.is_set() && (_at.get().x < 0 || _at.get().y < 0))
	{
		// that's already fine
		return;
	}

	Optional<VectorInt2> at = _at;
	if (at.is_set() &&
		!is_empty_at_weapon_part(at.get()))
	{
		at.clear();
	}

	if (!at.is_set())
	{
		VectorInt2 gridSize = _preferredGridSize.get(VectorInt2::zero); // this way, if grid size is set to zero, we still get the one from persistence
		if (gridSize.is_zero())
		{
			error(TXT("do not use it"));
		}
		if (!at.is_set())
		{
			// fill first rectangle as usual
			for (int y = 0; y < gridSize.y && !at.is_set(); ++y)
			{
				for (int x = 0; x < gridSize.x && !at.is_set(); ++x)
				{
					VectorInt2 loc(x, gridSize.y - 1 - y);
					if (is_empty_at_weapon_part(loc))
					{
						at = loc;
					}
				}
			}
		}
		if (!at.is_set())
		{
			// fill with rows
			for (int x = gridSize.x; !at.is_set(); ++x)
			{
				for (int y = 0; y < gridSize.y && !at.is_set(); ++y)
				{
					VectorInt2 loc(x, gridSize.y - 1 - y);
					if (is_empty_at_weapon_part(loc))
					{
						at = loc;
					}
				}
			}
		}
	}

	for_every(wp, weaponParts)
	{
		if (wp->id == _id)
		{
			wp->at = at.get();
			break;
		}
	}
}

void PilgrimInventory::keep_only_inventory_items()
{
	Concurrency::ScopedSpinLock lock(weaponPartsLock, true);

	for (int i = 0; i < weaponParts.get_size(); ++i)
	{
		if (weaponParts[i].source != Source::Inventory)
		{
			weaponParts.remove_at(i);
			--i;
		}
	}
}

PilgrimInventory::ID PilgrimInventory::add_weapon_part_internal(WeaponPart* _wp, Source _source, Framework::IModulesOwner* _item, WeaponPartAddress const& _address, Optional<VectorInt2> const& _at, Optional<VectorInt2> const& _preferredGridSize, bool _alwaysAdd)
{
	Concurrency::ScopedSpinLock lock(weaponPartsLock, true);

	if (!_alwaysAdd)
	{
		for_every(pp, weaponParts)
		{
			if (pp->weaponPart == _wp &&
				pp->source == _source &&
				pp->item == _item &&
				pp->address == _address)
			{
				place_weapon_part(pp->id, _at, _preferredGridSize);
				return pp->id;
			}
		}
	}

	PlacedPart pp;
	pp.id = get_free_id();
	pp.at = _at.get(VectorInt2(-1, -1));
	pp.source = _source;
	pp.partOfMainEquipment = false;
	if (_item)
	{
		if (auto* e = _item->get_gameplay_as<ModuleEquipment>())
		{
			pp.partOfMainEquipment = e->is_main_equipment();
		}
	}
	pp.weaponPart = _wp;
	pp.item = _item;
	pp.address = _address;

	weaponParts.push_back(pp);

	place_weapon_part(pp.id, _at, _preferredGridSize);

	return pp.id;
}

PilgrimInventory::ID PilgrimInventory::get_free_id() const
{
	Concurrency::ScopedSpinLock lock(weaponPartsLock, true);

	int id = 0;
	bool found = false;
	while (!found)
	{
		found = true;
		for_every(wp, weaponParts)
		{
			if (wp->id == id)
			{
				found = false;
				++id;
			}
		}
		if (id == NONE)
		{
			++id;
			found = false;
		}
	}

	return id;
}

bool PilgrimInventory::is_empty_at_weapon_part(VectorInt2 const& _at) const
{
	for_every(wp, weaponParts)
	{
		if (wp->at == _at)
		{
			return false;
		}
	}

	return true;
}