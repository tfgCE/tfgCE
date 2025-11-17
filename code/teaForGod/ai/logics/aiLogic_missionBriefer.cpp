#include "aiLogic_missionBriefer.h"

#include "..\..\game\game.h"
#include "..\..\game\missionState.h"
#include "..\..\library\library.h"
#include "..\..\library\missionDefinition.h"
#include "..\..\library\missionsDefinition.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\overlayInfo\elements\oie_customDraw.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"
#include "..\..\utils\reward.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\door.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define BRIEFER_STATS_WIDTH 2.5f
#define BRIEFER_WIDTH_CHARS max(4, TypeConversions::Normal::f_i_closest(5.0f / size))
#define BRIEFER_LINES_PER_PAGE max(4, TypeConversions::Normal::f_i_closest(1.5f / size))

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(brieferId);
DEFINE_STATIC_NAME(brieferChangeInteractiveDeviceId);
DEFINE_STATIC_NAME(brieferChange2InteractiveDeviceId);
DEFINE_STATIC_NAME(brieferChooseInteractiveDeviceId);
DEFINE_STATIC_NAME(brieferBriefingDebriefingInteractiveDeviceId);

// door tags
DEFINE_STATIC_NAME_STR(dt_exitToCapsule, TXT("exitToCapsule"));

// pilgrim overlay info
DEFINE_STATIC_NAME_STR(poi_missionBriefer, TXT("missionBriefer"));
DEFINE_STATIC_NAME_STR(poi_missionBriefer_page, TXT("missionBriefer_page"));
DEFINE_STATIC_NAME_STR(poi_missionBriefer_dial, TXT("missionBriefer_dial"));
DEFINE_STATIC_NAME_STR(poi_missionBriefer_start, TXT("missionBriefer_start"));

// text colours
DEFINE_STATIC_NAME(title);
DEFINE_STATIC_NAME(status);

// emissives
DEFINE_STATIC_NAME(busy);

// sounds
DEFINE_STATIC_NAME(missionSelected);

// localised strings
DEFINE_STATIC_NAME_STR(lsTaskDialInfo, TXT("mission briefer; task dial info"));
DEFINE_STATIC_NAME_STR(lsPageDialInfo, TXT("mission briefer; page dial info"));
DEFINE_STATIC_NAME_STR(lsChooseSwitchInfo, TXT("mission briefer; choose switch info"));
DEFINE_STATIC_NAME_STR(lsUnlock, TXT("mission briefer; unlock"));
	DEFINE_STATIC_NAME(cost);
	DEFINE_STATIC_NAME(has);
DEFINE_STATIC_NAME_STR(lsBriefingInfo, TXT("mission briefer; briefing info"));
DEFINE_STATIC_NAME_STR(lsDebriefingInfo, TXT("mission briefer; debriefing info"));
DEFINE_STATIC_NAME_STR(lsDebriefingDialInfo, TXT("mission briefer; debriefing dial info"));
DEFINE_STATIC_NAME_STR(lsMissionSuccess, TXT("mission briefer; mission success"));
DEFINE_STATIC_NAME_STR(lsMissionFailed, TXT("mission briefer; mission failed"));

DEFINE_STATIC_NAME_STR(lsDistance, TXT("hub; summary; distance"));
DEFINE_STATIC_NAME_STR(lsTime, TXT("hub; summary; time"));
DEFINE_STATIC_NAME_STR(lsKills, TXT("hub; summary; kills"));
DEFINE_STATIC_NAME_STR(lsExperience, TXT("hub; summary; experience"));
DEFINE_STATIC_NAME_STR(lsExperienceGained, TXT("hub; summary; experience; gained"));
DEFINE_STATIC_NAME_STR(lsExperienceMission, TXT("hub; summary; experience; mission"));
DEFINE_STATIC_NAME_STR(lsExperienceTotal, TXT("hub; summary; experience; total"));
DEFINE_STATIC_NAME_STR(lsExperienceToSpend, TXT("hub; summary; experience; to spend"));
DEFINE_STATIC_NAME_STR(lsMeritPoints, TXT("hub; summary; merit points"));
DEFINE_STATIC_NAME_STR(lsMeritPointsGained, TXT("hub; summary; merit points; gained"));
DEFINE_STATIC_NAME_STR(lsMeritPointsToSpend, TXT("hub; summary; merit points; to spend"));

DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsVisitedCell, TXT("missions; obj stats; visited cells"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsVisitedInterfaceBox, TXT("missions; obj stats; visited interface boxes"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsBroughtItem, TXT("missions; obj stats; brought items"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsHackedBoxes, TXT("missions; obj stats; hacked boxes"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsInfestations, TXT("missions; obj stats; infestations"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsRefills, TXT("missions; obj stats; refills"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsSuccess, TXT("missions; obj stats; success"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStatsAppliedBonus, TXT("missions; obj stats; applied bonus"));

DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));

// ai messages
DEFINE_STATIC_NAME_STR(aim_mgp_unlocked, TXT("mission general progress; unlocked"));

//

namespace DebriefingScreenIdx
{
	enum Type
	{
		MissionDesc,
		Stats,
		NUM
	};
};

//

static void remove_trailing_new_lines(REF_ String & desc)
{
	while (desc.get_length() > 0 && desc.get_right(1) == TXT("~"))
	{
		desc = desc.get_left(desc.get_length() - 1);
	}
}

// interactive devices

static Transform overlay_info_int_dev_offset()
{
	Transform intDevOffset = Transform(Vector3::zAxis * 0.08f + Vector3::yAxis * 0.10f, Rotator3(-60.0f, 0.0f, 0.0f).to_quat());
	return intDevOffset;
}

static float overlay_info_int_dev_size()
{
	float size = 0.025f;
	return size;
}

// actual overlay

static float overlay_info_text_size()
{
	float size = 0.1f;
	return size;
}

struct MissionBrieferDescUtils
{
	String fullDesc;
	int maxCharsPerLine;
	int maxLinesPerPage;
	List<String> lines;
	Array<int> pageStartsAt;

	MissionBrieferDescUtils(String const& _fullDesc, int _maxCharsPerLine, int _maxLinesPerPage)
	: fullDesc(_fullDesc)
	, maxCharsPerLine(_maxCharsPerLine)
	, maxLinesPerPage(_maxLinesPerPage)
	{
		break_into_lines();
		break_lines_into_pages();
	}

	void break_into_lines()
	{
		lines.clear();
		fullDesc.split(String(TXT("~")), lines);

		for (int i = 0; i < lines.get_size(); ++i)
		{
			int lineLength = lines[i].get_length();
			int lastHashAt = lines[i].find_last_of('#');
			if (lastHashAt != NONE)
			{
				lineLength = lineLength - (lastHashAt + 1);
			}

			if (lineLength > maxCharsPerLine)
			{
				int cutAt = lines[i].find_last_of(' ', maxCharsPerLine - 1);
				bool removeSpace = cutAt > lastHashAt;
				if (cutAt <= lastHashAt)
				{
					cutAt = maxCharsPerLine;
				}
				String left = lines[i].get_sub(removeSpace ? cutAt + 1 : cutAt);
				lines[i].keep_left_inline(cutAt);
				if (!left.is_empty())
				{
					lines.insert_after(lines.iterator_for(i), left);
				}
			}
		}
	}
	
	void break_lines_into_pages()
	{
		pageStartsAt.push_back(0);

		int lineIdx = 0;
		int lastEmptyLineAt = NONE;
		while (lineIdx < lines.get_size())
		{
			if (lines[lineIdx].is_empty())
			{
				lastEmptyLineAt = lineIdx;
			}
			else
			{
				if (lineIdx - pageStartsAt.get_last() + 1 > maxLinesPerPage)
				{
					if (lastEmptyLineAt != NONE)
					{
						lineIdx = lastEmptyLineAt + 1;
						lastEmptyLineAt = NONE;
						pageStartsAt.push_back(lineIdx);
					}
					else
					{
						pageStartsAt.push_back(lineIdx);
					}
				}
			}
			++lineIdx;
		}
	}

	int get_page_count() const { return pageStartsAt.get_size(); }

	String get_page_desc(int _idx, bool _withTrailingPages = true, bool _withPageNumber = true) const
	{
		String desc;
		int linesAdded = 0;
		if (pageStartsAt.is_index_valid(_idx))
		{
			int firstLine = pageStartsAt[_idx];
			int lastLine = pageStartsAt.is_index_valid(_idx + 1) ? pageStartsAt[_idx + 1] - 1 : lines.get_size() - 1;
			for (int i = firstLine; i <= lastLine; ++i)
			{
				desc += lines[i];
				desc += TXT("~");
				++linesAdded;
			}
		}
		if (_withTrailingPages)
		{
			while (linesAdded < maxLinesPerPage)
			{
				desc += TXT("~");
				++linesAdded;
			}
		}
		if (_withPageNumber)
		{
			if (get_page_count() > 1)
			{
				desc += String::printf(TXT("~%i/%i"), _idx + 1, get_page_count());
			}
		}
		return desc;
	}
};

//

REGISTER_FOR_FAST_CAST(MissionBriefer);

MissionBriefer::MissionBriefer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	missionBrieferData = fast_cast<MissionBrieferData>(_logicData);
	executionData = new ExecutionData();

	hitIndicatorColour.parse_from_string(String(TXT("hi_activated_upgrade_canister")));
	hitIndicatorUnlockColour.parse_from_string(String(TXT("hi_activated_upgrade_canister_mgp")));
	missionTitleColour.parse_from_string(String(TXT("pilgrim_overlay_mission_title")));
	missionSelectedColour.parse_from_string(String(TXT("pilgrim_overlay_mission_selected")));
	missionSuccessColour.parse_from_string(String(TXT("pilgrim_overlay_mission_success")));
	missionFailedColour.parse_from_string(String(TXT("pilgrim_overlay_mission_failed")));
	missionResultColour.parse_from_string(String(TXT("pilgrim_overlay_mission_result")));
}

MissionBriefer::~MissionBriefer()
{
	hide_any_overlay_info();
	if (auto* imo = get_imo())
	{
	}
}

void MissionBriefer::play_sound(Name const& _sound)
{
	if (auto* imo = get_imo())
	{
		if (auto* s = imo->get_sound())
		{
			s->play_sound(_sound);
		}
	}
}

void MissionBriefer::stop_sound(Name const& _sound)
{
	if (auto* imo = get_imo())
	{
		if (auto* s = imo->get_sound())
		{
			s->stop_sound(_sound);
		}
	}
}

void MissionBriefer::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	// wait till anything available
	if (!executionData->changeDialHandler.is_valid() ||
		!executionData->change2DialHandler.is_valid() ||
		!executionData->chooseSwitchHandler.is_valid() ||
		!executionData->briefingDebriefingButtonHandler.is_valid())
	{
		return;
	}

	{
		bool newActive = false;

		if (executionData->lastMissionResult.get())
		{
			newActive = true;
		}

		if (executionData->briefingDebriefingButtonHandlerEmissiveActive != newActive)
		{
			executionData->briefingDebriefingButtonHandlerEmissiveActive = newActive;
			for_every_ref(imo, executionData->briefingDebriefingButtonHandler.get_buttons())
			{
				if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
				{
					if (newActive)
					{
						e->emissive_activate(NAME(busy));
					}
					else
					{
						e->emissive_deactivate(NAME(busy));
					}
				}
			}
		}
	}

	if (executionData->updateUnlockStatus)
	{
		// check all missions and mark as unlocked
		// this may happen if we unlocked here or bridge shop unlocked
		executionData->updateUnlockStatus = false;
		if (auto* persistence = Persistence::access_current_if_exists())
		{
			for_every(am, executionData->availableMissions)
			{
				if (am->locked)
				{
					// only unlock, we won't be locking - it could be unlocked only if was locked
					if (auto* mgp = am->mission->get_mission_general_progress())
					{
						if (persistence->is_mission_general_progress_unlocked(mgp))
						{
							am->locked = false;
						}
					}
				}
			}
		}
		// we may need to update "start"
		executionData->updateStartInfoRequired = true;
	}

	// advance controls
	{
		executionData->changeDialHandler.advance(_deltaTime);
		executionData->change2DialHandler.advance(_deltaTime);
		executionData->chooseSwitchHandler.advance(_deltaTime);
		executionData->briefingDebriefingButtonHandler.advance(_deltaTime);

		if ((executionData->inDebriefing && executionData->debriefingScreenIdx == DebriefingScreenIdx::MissionDesc) ||
			(! executionData->inDebriefing))
		{
			int newPageDialAt = executionData->changeDialHandler.get_dial_at();
			if (executionData->sharedPageDialAt != newPageDialAt)
			{
				// will draw a different page
				hide_any_overlay_info();
			}
		}

		if (executionData->inDebriefing)
		{
			int idx = mod<int>(executionData->change2DialHandler.get_dial_at(), DebriefingScreenIdx::NUM);
			if (executionData->debriefingScreenIdx != idx)
			{
				executionData->debriefingScreenIdx = idx;
				hide_any_overlay_info();
			}
		}
		else
		{
			if (!executionData->availableMissions.is_empty())
			{
				if (!executionData->chosenMissionDefinition)
				{
					int idx = mod(executionData->change2DialHandler.get_dial_at(), executionData->availableMissions.get_size());
					auto* ns = &executionData->availableMissions[idx];
					if (executionData->selectedMission != ns)
					{
						executionData->selectedMission = ns;
						update_shared_page_ref();
						update_page_dial_info();
						update_task_dial_info_for_not_chosen_mission();
						update_start_info_for_not_chosen_mission();
					}
					if (executionData->chooseSwitchHandler.get_switch_at() >= 0.99f &&
						executionData->chooseSwitchHandler.get_prev_switch_at() < 0.99f &&
						!executionData->pendingChosenMission)
					{
						if (executionData->selectedMission &&
							executionData->selectedMission->mission &&
							executionData->selectedMission->locked)
						{
							if (auto* persistence = Persistence::access_current_if_exists())
							{
								if (auto* mgp = executionData->selectedMission->mission->get_mission_general_progress())
								{
									Concurrency::ScopedSpinLock lock(persistence->access_lock()); // we lock to make it impossible for anything else to stop us
									int costMP = executionData->selectedMission->unlockCost;
									an_assert(mgp->get_unlock_cost() == costMP);
									if (persistence->get_merit_points_to_spend() >= costMP)
									{
										bool unlocked = persistence->unlock_mission_general_progress(mgp);
										if (unlocked)
										{
											if (costMP > 0)
											{
												// we locked persistence, so we should still have cost
												persistence->spend_merit_points(costMP);
											}

											// mark as not locked - we don't need to 
											executionData->selectedMission->locked = false;

											auto* w = Game::get()->get_world();
											if (auto* aim = w->create_ai_message(NAME(aim_mgp_unlocked)))
											{
												aim->to_world(w);
											}

											{
												// do not do it via store game state as that should be only called from within pilgrimage
#ifdef AN_ALLOW_EXTENSIVE_LOGS
												output(TXT("store player profile - unlocked mission general progress"));
#endif
												Game::get_as<Game>()->add_async_save_player_profile();
											}

											{
												effect_on_selection(hitIndicatorUnlockColour, 2);
											}
										}
										else
										{
											if (persistence->is_mission_general_progress_unlocked(mgp))
											{
												executionData->selectedMission->locked = false;
											}
										}
									}
								}
							}
						}
						else
						{
							executionData->pendingChosenMission = true;

							{
								auto* selectedMission = executionData->selectedMission;
								Game::get()->add_sync_world_job(TXT("set mission"), [selectedMission]()
									{
										if (auto* ms = MissionState::get_current())
										{
											if (selectedMission)
											{
												ms->set_mission(selectedMission->mission, &selectedMission->objectives, selectedMission->difficultyModifier);
											}
											else
											{
												ms->set_mission(nullptr, nullptr, nullptr);
											}
										}
									});
							}

							if (executionData->selectedMission &&
								executionData->selectedMission->mission)
							{
								effect_on_selection(hitIndicatorColour, 3);
							}
						}
					}
				}
			}
		}

		if (executionData->briefingDebriefingButtonHandler.has_button_been_pressed())
		{
			if (executionData->inDebriefing)
			{
				executionData->debriefingPageDialRef = executionData->changeDialHandler.get_dial_at();
				executionData->debriefingScreenDialRef = executionData->change2DialHandler.get_dial_at();
			}
			else
			{
				executionData->briefingPageDialRef = executionData->changeDialHandler.get_dial_at();
				executionData->availableMissionDialRef = executionData->change2DialHandler.get_dial_at();
			}
			if (!executionData->lastMissionResult.get())
			{
				executionData->inDebriefing = false;
			}
			else
			{
				executionData->inDebriefing = !executionData->inDebriefing;
			}
			hide_any_overlay_info();
			if (executionData->inDebriefing)
			{
				executionData->changeDialHandler.reset_dial_zero(executionData->debriefingPageDialRef);
				executionData->change2DialHandler.reset_dial_zero(executionData->debriefingScreenDialRef);
			}
			else
			{
				executionData->changeDialHandler.reset_dial_zero(executionData->briefingPageDialRef);
				executionData->change2DialHandler.reset_dial_zero(executionData->availableMissionDialRef);
			}
			update_shared_page_ref();
		}
	}

	// show mission info if in the same room
	if (auto* imo = get_mind()->get_owner_as_modules_owner()) 
	{
		bool showSomething = false;
		if (auto* r = imo->get_presence()->get_in_room())
		{
			if (auto* p = executionData->pilgrim.get())
			{
				if (p->get_presence()->get_in_room() == r)
				{
					showSomething = true;
				}
			}
		}
		if (showSomething)
		{
			showSomething = false;
			if (executionData->inDebriefing)
			{
				if (MissionResult* shouldShow = executionData->lastMissionResult.get())
				{
					showSomething = true;
					if (executionData->showingDebriefingInfo != shouldShow)
					{
						show_debriefing_info(shouldShow);
					}
				}
			}
			else
			{
				if (AvailableMission* shouldShow = executionData->selectedMission)
				{
					showSomething = true;
					if (executionData->showingMissionInfo != shouldShow)
					{
						show_mission_info(shouldShow);
					}
				}
			}
		}
		if (!showSomething)
		{
			if (executionData->showingMissionInfo ||
				executionData->showingDebriefingInfo)
			{
				hide_any_overlay_info();
			}
		}
	}

	if (executionData->updateStartInfoRequired)
	{
		executionData->updateStartInfoRequired = false;
		if (executionData->showingMissionInfo)
		{
			update_start_info_for_not_chosen_mission();
		}
	}
}

void MissionBriefer::effect_on_selection(Colour const & _colour, int _count)
{
	if (auto* p = executionData->pilgrim.get())
	{
		if (auto* ha = p->get_custom<CustomModules::HitIndicator>())
		{
			Framework::RelativeToPresencePlacement r2pp;
			r2pp.be_temporary_snapshot();
			auto* heldSwitch = executionData->chooseSwitchHandler.get_switches()[0].get();
			if (r2pp.find_path(p, heldSwitch))
			{
				Vector3 buttonHereWS = r2pp.location_from_target_to_owner(heldSwitch->get_presence()->get_centre_of_presence_WS());
				float delay = 0.0f;
				for_count(int, i, _count)
				{
					ha->indicate_location(0.0f, buttonHereWS, CustomModules::HitIndicator::IndicateParams()
						.with_colour_override(_colour).with_delay(delay).try_being_relative_to(heldSwitch).not_damage());
					delay += 0.02f;
				}
			}
		}
		if (auto* s = p->get_sound())
		{
			s->play_sound(NAME(missionSelected));
		}
	}
}

void MissionBriefer::on_chosen_mission()
{
	hide_any_overlay_info(); // to draw it in a different colour
}

PilgrimOverlayInfo* MissionBriefer::access_overlay_info()
{
	if (auto* imo = executionData->pilgrim.get())
	{
		if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
		{
			return &mp->access_overlay_info();
		}
	}
	return nullptr;
}

void MissionBriefer::hide_any_overlay_info()
{
	if (auto* poi = access_overlay_info())
	{
		poi->hide_show_info(NAME(poi_missionBriefer));
		poi->hide_show_info(NAME(poi_missionBriefer_page));
		poi->hide_show_info(NAME(poi_missionBriefer_dial));
		poi->hide_show_info(NAME(poi_missionBriefer_start));
	}
	executionData->showingMissionInfo = nullptr;
	executionData->showingDebriefingInfo = nullptr;
}

void MissionBriefer::show_mission_info(AvailableMission* _show)
{
	hide_any_overlay_info();

	if (!_show)
	{
		_show = executionData->showingMissionInfo;
	}
	executionData->showingMissionInfo = _show;

	show_overlay_info();
}

void MissionBriefer::update_shared_page_ref()
{
	executionData->sharedPageRef = executionData->changeDialHandler.get_dial_at();
}

void MissionBriefer::show_debriefing_info(MissionResult* _result)
{
	hide_any_overlay_info();

	if (!_result)
	{
		_result = executionData->showingDebriefingInfo;
	}
	executionData->showingDebriefingInfo = _result;

	show_overlay_info();
}

void MissionBriefer::show_overlay_info()
{
	auto* poi = access_overlay_info();

	if (!poi)
	{
		return;
	}

	Transform intDevOffset = overlay_info_int_dev_offset();
	float intDevSize = overlay_info_int_dev_size();

	Transform descAt = Transform::identity;
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* p = imo->get_presence())
		{
			if (auto* r = p->get_in_room())
			{
				Framework::PointOfInterestInstance* foundPOI = nullptr;
				if (r->find_any_point_of_interest(missionBrieferData->overlayPOI, foundPOI))
				{
					descAt = foundPOI->calculate_placement();
				}
			}
		}
	}

	if (executionData->showingDebriefingInfo && executionData->inDebriefing)
	{
		// debriefing
		if (executionData->lastMissionResult.get())
		{
			float size = overlay_info_text_size();

			OverlayInfo::TextColours textColours;
			textColours.add(NAME(title), missionResultColour);
			if (auto* missionDefinition = executionData->showingDebriefingInfo->get_mission())
			{
				if (executionData->showingDebriefingInfo->is_success() && !missionDefinition->get_no_success())
				{
					textColours.add(NAME(status), missionSuccessColour);
				}
				else if (executionData->showingDebriefingInfo->is_failed() && !missionDefinition->get_no_failure())
				{
					textColours.add(NAME(status), missionFailedColour);
				}
			}

			if (executionData->debriefingScreenIdx == DebriefingScreenIdx::Stats)
			{
				struct HandleStats
				{
					Transform at;
					float size;
					float width;
					Transform offsetValue;
					Transform offsetLine;
					Transform offsetSpacing;
					HandleStats(Transform const& _at, float _size, float _width = BRIEFER_STATS_WIDTH)
					: at(_at)
					, size(_size)
					, width(_width)
					{
						offsetValue = Transform(Vector3::xAxis * width, Quat::identity);
						offsetLine = Transform(Vector3::zAxis * (-1.1f * size), Quat::identity);
						offsetSpacing = Transform(Vector3::zAxis * (-0.2f * size), Quat::identity);
					}

					void add(PilgrimOverlayInfo* poi, String const& _desc, String const& _value, Optional<OverlayInfo::TextColours> const & _textColours = NP, Optional<bool> const & _withSmallerDecimals = NP)
					{
						poi->show_info(NAME(poi_missionBriefer), at, _desc, _textColours, size, NP, false, Vector2(0.0f, 1.0f), _withSmallerDecimals);
						if (!_value.is_empty())
						{
							poi->show_info(NAME(poi_missionBriefer), at.to_world(offsetValue), _value, _textColours, size, NP, false, Vector2(1.0f, 1.0f), _withSmallerDecimals);
						}
						at = at.to_world(offsetLine);
					}
					void blank()
					{
						at = at.to_world(offsetLine);
					}
					void spacing()
					{
						at = at.to_world(offsetSpacing);
					}
				};
				HandleStats handleStats(descAt, size);
				if (auto* missionDefinition = executionData->showingDebriefingInfo->get_mission())
				{
					{
						String desc;
						desc += TXT("#title#");
						desc += missionDefinition->get_mission_name();
						handleStats.add(poi, desc, String::empty(), textColours);
						handleStats.spacing();
					}
					{
						String desc;
						if (executionData->showingDebriefingInfo->is_success() && ! missionDefinition->get_no_success())
						{
							desc += TXT("#status#");
							desc += LOC_STR(NAME(lsMissionSuccess));
						}
						else if (executionData->showingDebriefingInfo->is_failed() && !missionDefinition->get_no_failure())
						{
							desc += TXT("#status#");
							desc += LOC_STR(NAME(lsMissionFailed));
						}
						if (!desc.is_empty())
						{
							handleStats.add(poi, desc, String::empty(), textColours);
							handleStats.spacing();
						}
					}
				}
				{
					if (auto* md = executionData->showingDebriefingInfo->get_mission())
					{
						{
							Energy xp = md->get_mission_objectives().get_experience_for_visited_cell();
							int pp = md->get_mission_objectives().get_merit_points_for_visited_cell();
							if (!xp.is_zero() ||
								pp != 0 )
							{
								if (!xp.is_zero())
								{
									xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
								}
								String value = String::printf(TXT("%i"), executionData->lastMissionResult->get_visited_cells());
								if (pp != 0)
								{
									value += String::printf(TXT(" (%S)"), Reward(xp * (executionData->lastMissionResult->get_visited_cells()), executionData->lastMissionResult->get_visited_cells() * pp).no_xp().as_string_no_xp_char().to_char());
								}
								handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsVisitedCell)), value);
								handleStats.spacing();
							}
						}
						{
							Energy xp = md->get_mission_objectives().get_experience_for_visited_interface_box();
							int pp = md->get_mission_objectives().get_merit_points_for_visited_interface_box();
							if (!xp.is_zero() ||
								pp != 0 )
							{
								if (!xp.is_zero())
								{
									xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
								}
								String value = String::printf(TXT("%i"), executionData->lastMissionResult->get_visited_interface_boxes());
								if (pp != 0)
								{
									value += String::printf(TXT(" (%S)"), Reward(xp * (executionData->lastMissionResult->get_visited_interface_boxes()), executionData->lastMissionResult->get_visited_interface_boxes() * pp).no_xp().as_string_no_xp_char().to_char());
								}
								handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsVisitedInterfaceBox)), value);
								handleStats.spacing();
							}
						}
						{
							Energy xp = md->get_mission_objectives().get_experience_for_brought_item();
							int pp = md->get_mission_objectives().get_merit_points_for_brought_item();
							if (!xp.is_zero() ||
								pp != 0 )
							{
								if (!xp.is_zero())
								{
									xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
								}
								String value = String::printf(TXT("%i"), executionData->lastMissionResult->get_brought_items());
								if (pp != 0)
								{
									value += String::printf(TXT(" (%S)"), Reward(xp * (executionData->lastMissionResult->get_brought_items()), executionData->lastMissionResult->get_brought_items() * pp).no_xp().as_string_no_xp_char().to_char());
								}
								handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsBroughtItem)), value);
								handleStats.spacing();
							}
						}
						{
							Energy xp = md->get_mission_objectives().get_experience_for_hacked_box();
							int pp = md->get_mission_objectives().get_merit_points_for_hacked_box();
							if (!xp.is_zero() ||
								pp != 0 )
							{
								if (!xp.is_zero())
								{
									xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
								}
								String value = String::printf(TXT("%i"), executionData->lastMissionResult->get_hacked_boxes());
								if (pp != 0)
								{
									value += String::printf(TXT(" (%S)"), Reward(xp * (executionData->lastMissionResult->get_hacked_boxes()), executionData->lastMissionResult->get_hacked_boxes() * pp).no_xp().as_string_no_xp_char().to_char());
								}
								handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsHackedBoxes)), value);
								handleStats.spacing();
							}
						}
						{
							Energy xp = md->get_mission_objectives().get_experience_for_stopped_infestation();
							int pp = md->get_mission_objectives().get_merit_points_for_stopped_infestation();
							if (!xp.is_zero() ||
								pp != 0 )
							{
								if (!xp.is_zero())
								{
									xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
								}
								String value = String::printf(TXT("%i"), executionData->lastMissionResult->get_stopped_infestations());
								if (pp != 0)
								{
									value += String::printf(TXT(" (%S)"), Reward(xp * (executionData->lastMissionResult->get_stopped_infestations()), executionData->lastMissionResult->get_stopped_infestations() * pp).no_xp().as_string_no_xp_char().to_char());
								}
								handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsInfestations)), value);
								handleStats.spacing();
							}
						}
						{
							Energy xp = md->get_mission_objectives().get_experience_for_refill();
							int pp = md->get_mission_objectives().get_merit_points_for_refill();
							if (!xp.is_zero() ||
								pp != 0 )
							{
								if (!xp.is_zero())
								{
									xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
								}
								String value = String::printf(TXT("%i"), executionData->lastMissionResult->get_refills());
								if (pp != 0)
								{
									value += String::printf(TXT(" (%S)"), Reward(xp * (executionData->lastMissionResult->get_refills()), executionData->lastMissionResult->get_refills() * pp).no_xp().as_string_no_xp_char().to_char());
								}
								handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsRefills)), value);
								handleStats.spacing();
							}
						}
						if (executionData->lastMissionResult->is_success())
						{
							Energy xp = md->get_mission_objectives().get_experience_for_success();
							int pp = md->get_mission_objectives().get_merit_points_for_success();
							if (!xp.is_zero() ||
								pp != 0 )
							{
								if (!xp.is_zero())
								{
									xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
								}
								String value = String::printf(TXT("%S"), Reward(xp, pp).no_xp().as_string_no_xp_char().to_char());
								handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsSuccess)), value);
								handleStats.spacing();
							}
						}
					}
				}
				auto& stats = executionData->lastMissionResult->get_game_stats();
				{
					String value = Framework::Time::from_seconds(stats.timeInSeconds).get_string_short_time();
					handleStats.add(poi, LOC_STR(NAME(lsTime)), value);
				}
				handleStats.spacing();
				{
					String value;
					{
						MeasurementSystem::Type ms = PlayerSetup::get_current().get_preferences().get_measurement_system();
						if (ms == MeasurementSystem::Imperial)
						{
							value = String::printf(TXT("%.0fyd"), MeasurementSystem::to_yards(stats.distance));
						}
						else
						{
							value = String::printf(TXT("%.1fm"), stats.distance);
						}
					}
					handleStats.add(poi, LOC_STR(NAME(lsDistance)), value, NP, false);
				}
				handleStats.spacing();
				{
					String value = String::printf(TXT("%i"), stats.kills);
					handleStats.add(poi, LOC_STR(NAME(lsKills)), value);
				}
				handleStats.spacing();
				{
					handleStats.add(poi, LOC_STR(NAME(lsExperience)), String::empty());
				}
				// no spacing
				{
					String value = String::printf(TXT("+%S"), (stats.experience).as_string_auto_decimals().to_char());
					handleStats.add(poi, LOC_STR(NAME(lsExperienceGained)), value);
				}
				// no spacing
				{
					String value = String::printf(TXT("+%S"), (stats.experienceFromMission).as_string_auto_decimals().to_char());
					handleStats.add(poi, LOC_STR(NAME(lsExperienceMission)), value);
				}
				// no spacing
				{
					String value;
					if (auto* gs = PlayerSetup::get_current().get_active_game_slot())
					{
						value = gs->stats.experience.as_string_auto_decimals();
					}
					handleStats.add(poi, LOC_STR(NAME(lsExperienceTotal)), value);
				}
				// no spacing
				{
					String value = Persistence::get_current().get_experience_to_spend().as_string_auto_decimals() + LOC_STR(NAME(lsCharsExperience));
					handleStats.add(poi, LOC_STR(NAME(lsExperienceToSpend)), value);
				}
				handleStats.spacing();
				{
					handleStats.add(poi, LOC_STR(NAME(lsMeritPoints)), String::empty());
				}
				// no spacing
				{
					String value = String::printf(TXT("+%i%S"), (stats.meritPoints), LOC_STR(NAME(lsCharsMeritPoints)).to_char());
					handleStats.add(poi, LOC_STR(NAME(lsMeritPointsGained)), value);
				}
				// no spacing
				{
					String value = String::printf(TXT("%i%S"), Persistence::get_current().get_merit_points_to_spend(), LOC_STR(NAME(lsCharsMeritPoints)).to_char());
					handleStats.add(poi, LOC_STR(NAME(lsMeritPointsToSpend)), value);
				}
				handleStats.spacing();
				if (!executionData->lastMissionResult->get_applied_bonus().is_empty())
				{
					handleStats.add(poi, LOC_STR(NAME(lsMissionObjectiveStatsAppliedBonus)), executionData->lastMissionResult->get_applied_bonus().as_string());
					handleStats.spacing();
				}
			}
			else
			{
				String descTitle;
				String desc;
				if (auto* missionDefinition = executionData->showingDebriefingInfo->get_mission())
				{
					descTitle += TXT("#title#");
					descTitle += missionDefinition->get_mission_name();
					descTitle += TXT("~");
					if (auto* mdm = executionData->showingDebriefingInfo->get_difficulty_modifier())
					{
						String t = mdm->get_type_desc();
						if (!t.is_empty())
						{
							descTitle += TXT("(");
							descTitle += t;
							descTitle += TXT(")");
							descTitle += TXT("~");
						}
					}
					{
						if (executionData->showingDebriefingInfo->is_success() && !missionDefinition->get_no_success())
						{
							descTitle += TXT("#status#");
							descTitle += LOC_STR(NAME(lsMissionSuccess));
							descTitle += TXT("~");
						}
						else if (executionData->showingDebriefingInfo->is_failed() && !missionDefinition->get_no_failure())
						{
							descTitle += TXT("#status#");
							descTitle += LOC_STR(NAME(lsMissionFailed));
							descTitle += TXT("~");
						}
					}
					descTitle += TXT("~");
				}
				struct HandleDesc
				{
					static void add(REF_ String& desc, String const& _add, String const& _altAdd = String::empty())
					{
						if (!_add.is_empty())
						{
							desc += _add;
							desc += TXT("~~");
						}
						else if (!_altAdd.is_empty())
						{
							desc += _altAdd;
							desc += TXT("~~");
						}
					}
				};
				if (auto* missionDefinition = executionData->showingDebriefingInfo->get_mission())
				{
					{
						String subDesc;
						// did we come back?
						{
							if (executionData->showingDebriefingInfo->has_came_back())
							{
								HandleDesc::add(REF_ subDesc, missionDefinition->get_came_back_description());
							}
							else if (executionData->showingDebriefingInfo->has_died())
							{
								HandleDesc::add(REF_ subDesc, missionDefinition->get_died_description(), missionDefinition->get_did_not_come_back_description()); // if "died" not provided, use "did not come back"
							}
							else
							{
								// most likely if interrupted
								HandleDesc::add(REF_ subDesc, missionDefinition->get_did_not_come_back_description());
							}
						}
						// what did we do?
						{
							HandleDesc::add(REF_ subDesc, missionDefinition->get_mission_objectives().get_summary_description(executionData->lastMissionResult.get()));
						}
						// what was the immediate result
						{
							if (executionData->showingDebriefingInfo->is_success() && !missionDefinition->get_no_success())
							{
								HandleDesc::add(REF_ subDesc, missionDefinition->get_success_description());
							}
							else if (executionData->showingDebriefingInfo->is_failed() && !missionDefinition->get_no_failure())
							{
								HandleDesc::add(REF_ subDesc, missionDefinition->get_failed_description());
							}
							else
							{
								HandleDesc::add(REF_ subDesc, missionDefinition->get_not_success_not_failed_description());
							}
						}
						// what are the consequences and further steps?
						{
							if (executionData->showingDebriefingInfo->has_next_tier_been_made_available())
							{
								HandleDesc::add(REF_ subDesc, missionDefinition->get_next_tier_description());
							}
							else
							{
								HandleDesc::add(REF_ subDesc, missionDefinition->get_no_next_tier_description());
							}
						}
						desc += subDesc;
					}
				}

				remove_trailing_new_lines(REF_ desc);

				float size = overlay_info_text_size();

				{
					MissionBrieferDescUtils descUtils(desc, BRIEFER_WIDTH_CHARS, BRIEFER_LINES_PER_PAGE);

					executionData->showingDebriefingPageCount = descUtils.get_page_count();
					executionData->sharedPageDialAt = executionData->changeDialHandler.get_dial_at();
					int pageIdx = mod(executionData->changeDialHandler.get_dial_at() - executionData->sharedPageRef, descUtils.get_page_count());
					desc = descUtils.get_page_desc(pageIdx);
				}

				desc = descTitle + desc;

				poi->show_info(NAME(poi_missionBriefer), descAt, desc, textColours, size, NP, false, Vector2(0.0f, 1.0f));
			}
		}

		{
			for_every_ref(imo, executionData->change2DialHandler.get_dials())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);
				poi->show_info(NAME(poi_missionBriefer), at, LOC_STR(NAME(lsDebriefingDialInfo)), NP, intDevSize, NP, false);
			}
		}

		{
			for_every_ref(imo, executionData->briefingDebriefingButtonHandler.get_buttons())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);
				poi->show_info(NAME(poi_missionBriefer), at, LOC_STR(NAME(lsBriefingInfo)), NP, intDevSize, NP, false);
			}
		}

		update_page_dial_info();
	}
	else if (executionData->showingMissionInfo && !executionData->inDebriefing)
	{
		// briefing
		if (auto* mission = executionData->showingMissionInfo->mission)
		{
			{
				auto* mdm = executionData->showingMissionInfo->difficultyModifier;
					
				String descTitle;
				descTitle += TXT("#title#");
				descTitle += mission->get_mission_name();
				descTitle += TXT("~");
				if (auto* mdm = executionData->showingMissionInfo->difficultyModifier)
				{
					String t = mdm->get_type_desc();
					if (!t.is_empty())
					{
						descTitle += TXT("(");
						descTitle += t;
						descTitle += TXT(")");
						descTitle += TXT("~");
					}
				}
				descTitle += TXT("~");

				String desc;
				desc += mission->get_mission_description();

				if (mdm)
				{
					String subDesc = mdm->get_description();
					if (!subDesc.is_empty())
					{
						desc += TXT("~~");
						desc += subDesc;
					}
				}

				{
					String subDesc = mission->get_mission_objectives().get_description();
					if (!subDesc.is_empty())
					{
						desc += TXT("~~");
						desc += subDesc;
					}
				}

				if (mdm)
				{
					String subDesc = mdm->get_bonus_info(! mission->get_no_success());
					if (!subDesc.is_empty())
					{
						desc += TXT("~~");
						desc += subDesc;
					}
				}

				remove_trailing_new_lines(REF_ desc);

				float size = overlay_info_text_size();

				{
					MissionBrieferDescUtils descUtils(desc, BRIEFER_WIDTH_CHARS, BRIEFER_LINES_PER_PAGE);

					executionData->showingBriefingPageCount = descUtils.get_page_count();
					executionData->sharedPageDialAt = executionData->changeDialHandler.get_dial_at(); 
					int pageIdx = mod(executionData->sharedPageDialAt - executionData->sharedPageRef, descUtils.get_page_count());
					desc = descUtils.get_page_desc(pageIdx);
				}

				desc = descTitle + desc;

				OverlayInfo::TextColours textColours;
				textColours.add(NAME(title), executionData->chosenMissionDefinition ? missionSelectedColour : missionTitleColour);
				poi->show_info(NAME(poi_missionBriefer), descAt, desc, textColours, size, NP, false, Vector2(0.0f, 1.0f));
			}

			{
				executionData->showingMissionInfo->objectives.prepare_to_render(executionData->missionItemsRenderableData, Vector3::xAxis, Vector3::zAxis, Vector2(-1.2f, -0.5f), 1.0f);
				
				{
					if (!executionData->missionItemsRenderableData.is_empty())
					{
						float useSize = 0.5f;

						RefCountObjectPtr<ExecutionData> keepExecutionData = executionData;
						poi->show_info(NAME(poi_missionBriefer), descAt, [keepExecutionData, useSize]()
							{
								auto* e = new OverlayInfo::Elements::CustomDraw();

								e->with_draw([keepExecutionData](float _active, float _pulse, Colour const& _colour)
									{
										if (!keepExecutionData->missionItemsRenderableData.is_empty())
										{
											if (auto* imo = keepExecutionData->pilgrim.get())
											{
												if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
												{
													auto& poi = mp->access_overlay_info();
													keepExecutionData->missionItemsRenderableData.render(_active, _pulse, _colour, NP, poi.get_overlay_shader_instance());
												}
											}
										}
									});
								e->with_size(useSize, false);

								return e;
							}, false);

					}
				}
			}
		}

		if (! executionData->chosenMissionDefinition)
		{
			update_task_dial_info_for_not_chosen_mission();
			update_start_info_for_not_chosen_mission();
		}

		update_page_dial_info();

		if (executionData->lastMissionResult.get())
		{
			for_every_ref(imo, executionData->briefingDebriefingButtonHandler.get_buttons())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);
				poi->show_info(NAME(poi_missionBriefer), at, LOC_STR(NAME(lsDebriefingInfo)), NP, intDevSize, NP, false);
			}
		}
	}
}

void MissionBriefer::update_page_dial_info()
{
	Transform intDevOffset = overlay_info_int_dev_offset();
	float intDevSize = overlay_info_int_dev_size();

	if (auto* poi = access_overlay_info())
	{
		poi->hide_show_info(NAME(poi_missionBriefer_page));

		int pageCount = executionData->showingDebriefingInfo && executionData->inDebriefing ? executionData->showingDebriefingPageCount : executionData->showingBriefingPageCount;
		int pageIdx = mod(executionData->changeDialHandler.get_dial_at() - executionData->sharedPageRef, pageCount);

		if (pageCount > 1)
		{
			String text = LOC_STR(NAME(lsPageDialInfo));
			text += TXT("~");
			text += String::printf(TXT("%i/%i"), pageIdx + 1, pageCount);

			for_every_ref(imo, executionData->changeDialHandler.get_dials())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);

				poi->show_info(NAME(poi_missionBriefer_page), at, text, NP, intDevSize, NP, false);
			}
		}
	}
}

void MissionBriefer::update_task_dial_info_for_not_chosen_mission()
{
	Transform intDevOffset = overlay_info_int_dev_offset();
	float intDevSize = overlay_info_int_dev_size();

	if (auto* poi = access_overlay_info())
	{
		poi->hide_show_info(NAME(poi_missionBriefer_dial));

		if (executionData->availableMissions.get_size() > 1)
		{
			String text = LOC_STR(NAME(lsTaskDialInfo));
			text += TXT("~");
			{
				int count = executionData->availableMissions.get_size();
				int idx = mod(executionData->change2DialHandler.get_dial_at(), count);
				text += String::printf(TXT("%i/%i"), idx + 1, count);
			}

			for_every_ref(imo, executionData->change2DialHandler.get_dials())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);

				poi->show_info(NAME(poi_missionBriefer_dial), at, text, NP, intDevSize, NP, false);
			}
		}
	}
}

void MissionBriefer::update_start_info_for_not_chosen_mission()
{
	executionData->updateStartInfoRequired = false;

	Transform intDevOffset = overlay_info_int_dev_offset();
	float intDevSize = overlay_info_int_dev_size();

	if (auto* poi = access_overlay_info())
	{
		poi->hide_show_info(NAME(poi_missionBriefer_start));

		{
			String text = LOC_STR(NAME(lsChooseSwitchInfo));

			if (executionData->selectedMission &&
				executionData->selectedMission->locked)
			{
				int hasMP = Persistence::get_current().get_merit_points_to_spend();
				text = Framework::StringFormatter::format(LOC_STR(NAME(lsUnlock)), Framework::StringFormatterParams()
					.add(NAME(cost), executionData->selectedMission->unlockCost != 0? String::printf(TXT("%i%S"), executionData->selectedMission->unlockCost, LOC_STR(NAME(lsCharsMeritPoints)).to_char()) : String::empty())
					.add(NAME(has), executionData->selectedMission->unlockCost != 0? String::printf(TXT("(%i%S)"), hasMP, LOC_STR(NAME(lsCharsMeritPoints)).to_char()) : String::empty())
				);
			}

			for_every_ref(imo, executionData->chooseSwitchHandler.get_switches())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);

				poi->show_info(NAME(poi_missionBriefer_start), at, text, NP, intDevSize, NP, false);
			}
		}
	}
}

void MissionBriefer::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(MissionBriefer::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai mission briefer] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<MissionBriefer>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("mission briefer, hello!"));

	messageHandler.use_with(mind);
	{
		RefCountObjectPtr<ExecutionData> keepExecutionData;
		keepExecutionData = self->executionData;
		{
			messageHandler.set(NAME(aim_mgp_unlocked), [keepExecutionData](Framework::AI::Message const& _message)
				{
					keepExecutionData->updateUnlockStatus = true;
				}
			);
		}
	}


	// wait a bit for interactive controls to appear
	LATENT_WAIT(0.1f);

	if (imo)
	{
		{
			if (auto* r = imo->get_presence()->get_in_room())
			{
				if (auto* id = imo->get_variables().get_existing<int>(NAME(brieferChangeInteractiveDeviceId)))
				{
					int brieferChangeInteractiveDeviceId = *id;

					auto* world = imo->get_in_world();
					an_assert(world);

					world->for_every_object_with_id(NAME(brieferChangeInteractiveDeviceId), brieferChangeInteractiveDeviceId,
						[self, imo](Framework::Object* _object)
						{
							if (_object != imo)
							{
								self->executionData->changeDialHandler.initialise(_object, NAME(brieferChangeInteractiveDeviceId));
							}
							return false; // keep going on - we want to get all
						});
				}
				if (auto* id = imo->get_variables().get_existing<int>(NAME(brieferChange2InteractiveDeviceId)))
				{
					int brieferChange2InteractiveDeviceId = *id;

					auto* world = imo->get_in_world();
					an_assert(world);

					world->for_every_object_with_id(NAME(brieferChange2InteractiveDeviceId), brieferChange2InteractiveDeviceId,
						[self, imo](Framework::Object* _object)
						{
							if (_object != imo)
							{
								self->executionData->change2DialHandler.initialise(_object, NAME(brieferChange2InteractiveDeviceId));
							}
							return false; // keep going on - we want to get all
						});
				}
				if (auto* id = imo->get_variables().get_existing<int>(NAME(brieferChooseInteractiveDeviceId)))
				{
					int brieferChooseInteractiveDeviceId = *id;

					auto* world = imo->get_in_world();
					an_assert(world);

					world->for_every_object_with_id(NAME(brieferChooseInteractiveDeviceId), brieferChooseInteractiveDeviceId,
						[self, imo](Framework::Object* _object)
						{
							if (_object != imo)
							{
								self->executionData->chooseSwitchHandler.initialise(_object, NAME(brieferChooseInteractiveDeviceId));
							}
							return false; // keep going on - we want to get all
						});
				}
				if (auto* id = imo->get_variables().get_existing<int>(NAME(brieferBriefingDebriefingInteractiveDeviceId)))
				{
					int brieferBriefingDebriefingInteractiveDeviceId = *id;

					auto* world = imo->get_in_world();
					an_assert(world);

					world->for_every_object_with_id(NAME(brieferBriefingDebriefingInteractiveDeviceId), brieferBriefingDebriefingInteractiveDeviceId,
						[self, imo](Framework::Object* _object)
						{
							if (_object != imo)
							{
								self->executionData->briefingDebriefingButtonHandler.initialise(_object, NAME(brieferBriefingDebriefingInteractiveDeviceId));
							}
							return false; // keep going on - we want to get all
						});
				}
			}
		}
	}

	{
		self->executionData->pilgrim.clear();
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					self->executionData->pilgrim = pa;
				}
			}
		}
	}

	{
		self->executionData->ourPilgrimage = nullptr;
		// find our pilgrimage, wait till we got it
		while (!self->executionData->ourPilgrimage)
		{
			if (imo)
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					while (piow)
					{
						if (piow->get_starting_room() == imo->get_presence()->get_in_room())
						{
							self->executionData->ourPilgrimage = piow;
							// for time being disallow creating next pilgrimage
							piow->set_can_create_next_pilgrimage(false);
							break;
						}
						else
						{
							if (auto* pi = piow->get_next_pilgrimage_instance())
							{
								piow = fast_cast<PilgrimageInstanceOpenWorld>(pi);
							}
							else
							{
								break;
							}
						}
					}
				}
			}
			LATENT_YIELD();
		}
	}

	if (self->missionBrieferData->exitToCapsuleDoorTag.is_valid())
	{
		while (!self->executionData->exitToCapsuleDoor.get())
		{
			{
				auto* subWorld = imo->get_in_sub_world();
				an_assert(subWorld);
				self->executionData->exitToCapsuleDoor = subWorld->find_door_tagged(self->missionBrieferData->exitToCapsuleDoorTag);
				if (auto* d = self->executionData->exitToCapsuleDoor.get())
				{
					if (! d->get_linked_door_a() ||
						! d->get_linked_door_b() ||
						! d->get_linked_door_a()->get_in_room() ||
						! d->get_linked_door_b()->get_in_room() ||
						! d->get_linked_door_a()->get_in_room()->is_world_active() ||
						! d->get_linked_door_b()->get_in_room()->is_world_active())
					{
						self->executionData->exitToCapsuleDoor.clear();
					}
				}
			}

			LATENT_WAIT(0.1f);
		}

		if (auto* d = self->executionData->exitToCapsuleDoor.get())
		{
			// we will open when the mission is selected
			d->set_operation(Framework::DoorOperation::StayClosedTemporarily);
			// force managing exit door, we will be using exit to capsule door to hold us here
			if (auto* piow = self->executionData->ourPilgrimage)
			{
				piow->set_manage_starting_room_exit_door(true);
			}
		}
	}

	if (!MissionState::get_current())
	{
		if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
		{
			if (gs->gameSlotMode != GameSlotMode::Missions)
			{
				warn(TXT("switching to missions game slot mode"));
				gs->set_game_slot_mode(GameSlotMode::Missions);
			}
			if (gs->gameSlotMode == GameSlotMode::Missions)
			{
				gs->create_new_mission_state();
			}
			else
			{
				error(TXT("not in missions mode?"));
			}
		}
		else
		{
			error(TXT("no game slot?"));
		}
	}

	// wait for mission state to become active
	while (!MissionState::get_current())
	{
		LATENT_YIELD();
	}

	LATENT_YIELD();
	{
		AvailableMission::get_available_missions(OUT_ self->executionData->availableMissions);
		//
		{
			Optional<int> maxTier;
			for_every(am, self->executionData->availableMissions)
			{
				if (maxTier.is_set())
				{
					maxTier = max(maxTier.get(), am->tier);
				}
				else
				{
					maxTier = am->tier;
				}
			}
			MissionState::get_current()->set_starting_max_mission_tier(maxTier.get(0));
		}
		// select one, the first as we sorted by tiers
		if (!self->executionData->availableMissions.is_empty())
		{
			self->executionData->selectedMission = &self->executionData->availableMissions[0];
			self->update_shared_page_ref();
		}
		else
		{
			self->executionData->selectedMission = nullptr;
		}
	}
	{
		// get last mission result, if available, start in debriefing mode
		self->executionData->lastMissionResult.clear();
		if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
		{
			if (auto* mr = gs->get_last_mission_result())
			{
				// debrief only if the mission has actually started and there actually was a mission selected
				if (mr->has_started() &&
					mr->get_mission())
				{
					self->executionData->lastMissionResult = mr;
				}
			}
		}
		self->executionData->inDebriefing = self->executionData->lastMissionResult.is_set();
	}

	// check if maybe we already have selected the mission
	while (! self->executionData->chosenMissionDefinition)
	{
		{
			if (auto* ms = MissionState::get_current())
			{
				if (auto* m = ms->get_mission())
				{
					if (self->executionData->chosenMissionDefinition != m)
					{
						self->executionData->chosenMissionDefinition = m;
						self->executionData->pendingChosenMission = false;
						self->on_chosen_mission();
					}
				}
			}
		}

		LATENT_YIELD(); // to allow advance every frame
	}

	// clear last mission result, we started a new mission, we should not need the old result
	{
		if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
		{
			gs->clear_last_mission_result();
		}
	}

	// open door and allow creating next pilgrimage
	// use a new random seed if required by the mission
	{
		if (auto* piow = self->executionData->ourPilgrimage)
		{
			if (self->executionData->chosenMissionDefinition)
			{
				if (self->executionData->chosenMissionDefinition->get_new_pilgrimage_random_seed_on_start())
				{
					// we set new random seed to this pilgrimage but as it has been already created (or should be fairly static/same, this is allowed to happen
					// the random seed will be carried to the next pilgrimage
					piow->set_seed(Random::Generator());
				}
			}
			piow->set_can_create_next_pilgrimage(true);
		}
		if (auto* d = self->executionData->exitToCapsuleDoor.get())
		{
			d->set_operation(Framework::DoorOperation::StayOpen, Framework::Door::DoorOperationParams().allow_auto());
		}
	}

	// just wait endlessly
	while (true)
	{
		LATENT_WAIT(0.1f);
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(MissionBrieferData);

bool MissionBrieferData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	overlayPOI = _node->get_name_attribute(TXT("overlayPOI"));
	overlayScale = _node->get_float_attribute(TXT("overlayScale"), 1.0f);
	exitToCapsuleDoorTag = _node->get_name_attribute(TXT("exitToCapsuleDoorTag"));

	return result;
}

bool MissionBrieferData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
