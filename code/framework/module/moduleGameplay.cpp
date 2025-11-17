#include "moduleGameplay.h"

#include "modulePresence.h"

#include "..\advance\advanceContext.h"
#include "..\game\game.h"

#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool g_moduleGameplay_uses__advance__post_logic = true;

//

REGISTER_FOR_FAST_CAST(ModuleGameplay);

bool ModuleGameplay::does_use__advance__post_logic()
{
	return g_moduleGameplay_uses__advance__post_logic;
}

void ModuleGameplay::use__advance__post_logic(bool _use)
{
	g_moduleGameplay_uses__advance__post_logic = _use;
}

ModuleGameplay::ModuleGameplay(IModulesOwner* _owner)
: Module(_owner)
{
}

void ModuleGameplay::advance__post_logic(IMMEDIATE_JOB_PARAMS)
{
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE_COLOURED(moduleGame_postLogic, Colour::zxGreenBright);
		if (ModuleGameplay* self = modulesOwner->get_gameplay())
		{
			self->advance_post_logic(ac->get_delta_time());
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModuleGameplay::advance__post_move(IMMEDIATE_JOB_PARAMS)
{
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE_COLOURED(moduleGame_postMove, Colour::zxGreenBright);
		if (ModuleGameplay* self = modulesOwner->get_gameplay())
		{
			if (self->should_update_attached())
			{
				ArrayStatic<IModulesOwner*, 32> attached; // should be enough to store these, we shouldn't go to crazy with hierarchies
				SET_EXTRA_DEBUG_INFO(attached, TXT("ModuleGameplay::advance__post_move.attached"));
				struct ShouldAddIt
				{
					static bool check(IModulesOwner const* imo)
					{
						if (auto* g = imo->get_gameplay())
						{
							if (g->should_update_with_attached_to())
							{
								return true;
							}
						}
						if (auto* p = imo->get_presence())
						{
							if (p->has_anything_attached())
							{
								return true;
							}
						}
						return false;
					}
				};
				struct SendBugOnce
				{
					static void send(IModulesOwner* _owner, IModulesOwner* _object, ArrayStatic<IModulesOwner*, 32> const & _attached, int i, Array<IModulesOwner*> const& _tryingToAdd, int tryingToAddIdx)
					{
						static bool sentOnce = false;
						if (!sentOnce)
						{
							sentOnce = true;
							String shortText = String::printf(TXT("ModuleGameplay::advance__post_move no place in array"));
							String moreInfo;
							{
								moreInfo += TXT("ModuleGameplay::advance__post_move no place in array\n");
								moreInfo += String::printf(TXT("owner: %S\n"), _owner->ai_get_name().to_char());
								moreInfo += String::printf(TXT("object: %S\n"), _object->ai_get_name().to_char());
								moreInfo += String::printf(TXT("at: %i\n"), i);
								moreInfo += String::printf(TXT("the list:\n"));
								for_every_ptr(a, _attached)
								{
									moreInfo += String::printf(TXT(" %c %02i : %S\n"), for_everys_index(a) == i? '>' : ' ', for_everys_index(a), a ? a->ai_get_name().to_char() : TXT("--"));
								}
								moreInfo += String::printf(TXT("trying to add:\n"));
								for_every_ptr(a, _tryingToAdd)
								{
									moreInfo += String::printf(TXT(" %c %02i : %S\n"), for_everys_index(a) == tryingToAddIdx ? '>' : ' ', for_everys_index(a), a? a->ai_get_name().to_char() : TXT("--"));
								}
							}
							if (auto* g = Game::get())
							{
								g->send_quick_note_in_background(shortText, moreInfo);
							}
						}
					}
				};
				{
					auto* p = modulesOwner->get_presence();
					Concurrency::ScopedSpinLock lock(p->access_attached_lock());
					for_every_ptr(a, p->get_attached())
					{
						if (ShouldAddIt::check(a))
						{
							if (attached.has_place_left())
							{
								attached.push_back(a);
							}
							else
							{
								warn(TXT("no place for \"%S\""), a->ai_get_name().to_char());
								SendBugOnce::send(modulesOwner, a, attached, -1000, p->get_attached(), for_everys_index(a));
							}
						}
					}
				}
				for (int i = 0; i < attached.get_size(); ++i)
				{
					auto* attachedImo = attached[i];
					if (ModuleGameplay* attachedGameplay = attachedImo->get_gameplay())
					{
						if (attachedGameplay->should_update_with_attached_to())
						{
							attachedGameplay->advance_post_move(ac->get_delta_time());
						}
					}
					{
						auto* p = attachedImo->get_presence();
						Concurrency::ScopedSpinLock lock(p->access_attached_lock());
						if (attached.get_space_left() < p->get_attached().get_size())
						{
							// let's make more space, remove everything up to here
							if (i >= 0)
							{
								attached.remove_at(0, i + 1);
							}
							i = -1;
						}
						for_every_ptr(a, p->get_attached())
						{
							if (ShouldAddIt::check(a))
							{
								if (attached.has_place_left())
								{
									attached.push_back(a);
								}
								else
								{
									warn(TXT("no place for \"%S\""), a->ai_get_name().to_char());
									SendBugOnce::send(modulesOwner, a, attached, i, p->get_attached(), for_everys_index(a));
								}
							}
						}
					}
				}
			}
			self->advance_post_move(ac->get_delta_time());
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

