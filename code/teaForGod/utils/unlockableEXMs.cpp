#include "unlockableEXMs.h"

#include "..\library\library.h"
#include "..\library\gameDefinition.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

int UnlockableEXMs::count_unlocked(Name const& _exm)
{
	if (auto* persistence = Persistence::access_current_if_exists())
	{
		Concurrency::ScopedSpinLock lock(persistence->access_lock(), true);

		int unlockedTimes = 0;

		Array<Name> const& allUnlockedEXMs = persistence->get_all_exms();

		for_every(unlockedEXM, allUnlockedEXMs)
		{
			if (*unlockedEXM == _exm)
			{
				++unlockedTimes;
			}
		}
		
		return unlockedTimes;
	}

	return 0;
}

void UnlockableEXMs::update(bool _simpleRules, Optional<bool> const& _nonPermanentFromLastSetup)
{
	// by default for simple rules there are no non-permanent
	bool nonPermanentFromLastSetup = _nonPermanentFromLastSetup.get(!_simpleRules);

	Persistence::UsedSetup lastUsedSetup;
	Persistence::UsedSetup availableSetup; // if invalid, can't be unlocked as it is already unlocked

	lastUsedSetup = Persistence::UsedSetup();
	availableSetup = Persistence::UsedSetup();

	if (auto* persistence = Persistence::access_current_if_exists())
	{
		Concurrency::ScopedSpinLock lock(persistence->access_lock(), true);

		lastUsedSetup = persistence->get_last_used_setup();
		availableSetup = lastUsedSetup;

		availableEXMs.clear();
		unlockedEXMs = persistence->get_all_exms();

		// non permanent
		{
			if (nonPermanentFromLastSetup)
			{
				Array<Name> const& allUnlockedEXMs = persistence->get_all_exms();

				{
					// here we do not remove from allUnlockedEXMs as it is a blueprint
					for_count(int, hIdx, Hand::MAX)
					{
						if (allUnlockedEXMs.does_contain(availableSetup.activeEXMs[hIdx]))
						{
							availableSetup.activeEXMs[hIdx] = Name::invalid();
						}
						if (availableSetup.activeEXMs[hIdx].is_valid())
						{
							availableEXMs.push_back_unique(availableSetup.activeEXMs[hIdx]);
						}
					}
					for_count(int, hIdx, Hand::MAX)
					{
						for_every(exm, availableSetup.passiveEXMs[hIdx])
						{
							if (allUnlockedEXMs.does_contain(*exm))
							{
								*exm = Name::invalid();
							}
							if (exm->is_valid())
							{
								availableEXMs.push_back_unique(*exm);
							}
						}
					}
				}

				// and all the others
				for_every(exm, availableSetup.availableEXMs)
				{
					if (allUnlockedEXMs.does_contain(*exm))
					{
						// already unlocked, we don't need to make it available for unlock again
						*exm = Name::invalid();
					}
					if (exm->is_valid())
					{
						availableEXMs.push_back_unique(*exm);
					}
				}
			}
			else
			{
			}
		}

		// permanent
		{
			if (!_simpleRules)
			{
				Array<Name> allUnlockedEXMs = persistence->get_all_exms();

				// here we remove as we allow stacking
				for_every(exm, availableSetup.permanentEXMs)
				{
					if (allUnlockedEXMs.does_contain(*exm))
					{
						// we removed from unlocked as we may have more available than we have already unlocked and we want to keep that extra one
						allUnlockedEXMs.remove(*exm);
						*exm = Name::invalid();
					}
					if (exm->is_valid())
					{
						availableEXMs.push_back_unique(*exm);
					}
				}
			}
			else
			{
				Array<Name> const & allUnlockedEXMs = persistence->get_all_exms();

				TagCondition gdRequirement;
				if (auto* gd = GameDefinition::get_chosen())
				{
					gdRequirement = gd->get_exms_tagged();
				}

				Library::get_current_as<Library>()->get_exm_types().do_for_every(
					[this, &availableSetup, &allUnlockedEXMs, &gdRequirement](Framework::LibraryStored* _libraryStored)
					{
						if (auto* e = fast_cast<EXMType>(_libraryStored))
						{
							if (e->is_permanent())
							{
								if (gdRequirement.check(e->get_tags()))
								{
									int alreadyUnlocked = 0;
									for_every(au, allUnlockedEXMs)
									{
										if (e->get_id() == *au)
										{
											++alreadyUnlocked;
										}
									}
									if (alreadyUnlocked < e->get_permanent_limit())
									{
										availableSetup.permanentEXMs.push_back(e->get_id());
										availableEXMs.push_back(e->get_id());
									}
								}
							}
						}
					});
			}
		}

		/*
#ifdef BUILD_NON_RELEASE
		{
			Array<Name> allUnlockedEXMs = persistence->get_all_exms();
		
			TagCondition gdRequirement;
			if (auto* gd = GameDefinition::get_chosen())
			{
				gdRequirement = gd->get_exms_tagged();
			}

			Library::get_current_as<Library>()->get_exm_types().do_for_every(
				[this, &allUnlockedEXMs, &gdRequirement](Framework::LibraryStored* _libraryStored)
				{
					if (auto* e = fast_cast<EXMType>(_libraryStored))
					{
						if (gdRequirement.check(e->get_tags()))
						{
							int alreadyUnlocked = 0;
							for_every(au, allUnlockedEXMs)
							{
								if (e->get_id() == *au)
								{
									++alreadyUnlocked;
								}
							}
							//if (! e->is_permanent() || alreadyUnlocked < e->get_permanent_limit())
							if (e->is_permanent() && alreadyUnlocked < e->get_permanent_limit())
							{
								availableEXMs.push_back(e->get_id());
							}
						}
					}
				});
		}
#endif
		*/

	}
}
