#include "lhs_pilgrimageSetup.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\scenes\lhs_calibrate.h"
#include "..\scenes\lhs_handyMessages.h"
#include "..\scenes\lhs_options.h"
#include "..\scenes\lhs_playerProfileSelection.h"
#include "..\screens\lhc_enterText.h"
#include "..\screens\lhc_handGestures.h"
#include "..\screens\lhc_message.h"
#include "..\screens\lhc_messageSetBrowser.h"
#include "..\screens\lhc_question.h"
#include "..\screens\lhc_unlocks.h"
#include "..\screens\settings\lhc_gameModifiers.h"
#include "..\screens\prayStations\lhc_spirographMandala.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_list.h"
#include "..\widgets\lhw_slider.h"
#include "..\widgets\lhw_text.h"
#include "..\utils\lhu_playerStats.h"

#include "..\..\..\buildNOs.h"
#include "..\..\..\game\game.h"
#include "..\..\..\game\gameConfig.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\game\persistence.h"
#include "..\..\..\library\gameDefinition.h"
#include "..\..\..\library\messageSet.h"
#include "..\..\..\pilgrimage\pilgrimage.h"

#include "..\..\..\..\core\buildVer.h"
#include "..\..\..\..\core\buildInformation.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\..\framework\debug\testConfig.h"
#include "..\..\..\..\framework\library\library.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

#include <time.h>

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define VERBOSE_RESTART
//#define VERBOSE_GAME_STATES

//

// screens
DEFINE_STATIC_NAME(hubBackground);
DEFINE_STATIC_NAME(pilgrimageSetupVersionInfo);
DEFINE_STATIC_NAME(pilgrimageSetupMainPanel_Main);
DEFINE_STATIC_NAME(pilgrimageSetupMainPanel_TestScenarios);
DEFINE_STATIC_NAME(pilgrimageSetupMainPanel_GameModifiers);
DEFINE_STATIC_NAME(pilgrimageSetupMainPanel_Unlocks);
DEFINE_STATIC_NAME(pilgrimageSetupMainPanel_Stats);
DEFINE_STATIC_NAME(pilgrimageSetupMenuBottom);
DEFINE_STATIC_NAME(pilgrimageSetupMenuBottom_Back);
DEFINE_STATIC_NAME(pilgrimageSetupMenuWhereToStart);
DEFINE_STATIC_NAME(pilgrimageSetupMenuWhereToStart_Left);
DEFINE_STATIC_NAME(pilgrimageSetupMenuWhereToStart_Right);
DEFINE_STATIC_NAME(pilgrimageSetupMenuWhereToTestStart);
DEFINE_STATIC_NAME(pilgrimageSetupMenuRight);
DEFINE_STATIC_NAME(pilgrimageSetupMenuLeft);
DEFINE_STATIC_NAME(pilgrimageSetupProfileGameSlot);
DEFINE_STATIC_NAME(pilgrimageSetupRealTime);
DEFINE_STATIC_NAME(manageProfile);

// ids
DEFINE_STATIC_NAME(idCalibrate);
DEFINE_STATIC_NAME(idGameStateList);
DEFINE_STATIC_NAME(idTestScenarioList);
DEFINE_STATIC_NAME(idRealTime);
DEFINE_STATIC_NAME(idProfileGameSlot);
DEFINE_STATIC_NAME(idStart);
DEFINE_STATIC_NAME(idBack);
DEFINE_STATIC_NAME(idChangeGameSlotProfile);

// localised strings
DEFINE_STATIC_NAME_STR(lsChooseLoadout, TXT("hub; common; choose loadout"));
DEFINE_STATIC_NAME_STR(lsStart, TXT("hub; game menu; start"));
DEFINE_STATIC_NAME_STR(lsMission, TXT("hub; game menu; mission"));
DEFINE_STATIC_NAME_STR(lsSwitchToMissions, TXT("hub; game menu; switch to missions"));
DEFINE_STATIC_NAME_STR(lsSwitchToStory, TXT("hub; game menu; switch to story"));
DEFINE_STATIC_NAME_STR(lsSwitchModeQuestion, TXT("hub; game menu; switch mode; question"));
DEFINE_STATIC_NAME_STR(lsEarlyUnlockMissionsQuestion, TXT("hub; game menu; switch mode; early unlock missions question"));
DEFINE_STATIC_NAME_STR(lsNewMission, TXT("hub; game menu; new mission"));
DEFINE_STATIC_NAME_STR(lsAbortMission, TXT("hub; game menu; abort mission"));
DEFINE_STATIC_NAME_STR(lsAbortMissionQuestion, TXT("hub; game menu; abort mission; question"));
DEFINE_STATIC_NAME_STR(lsBack, TXT("hub; common; back"));
DEFINE_STATIC_NAME_STR(lsCalibrate, TXT("hub; game menu; calibrate"));
DEFINE_STATIC_NAME_STR(lsOptions, TXT("hub; game menu; options"));
DEFINE_STATIC_NAME_STR(lsProfileGameSlot, TXT("hub; game menu; profile game slot"));

DEFINE_STATIC_NAME_STR(lsWhereToStartLastMoment, TXT("hub; game menu; where to start; last moment"));
DEFINE_STATIC_NAME_STR(lsWhereToStartLastMomentInterrupted, TXT("hub; game menu; where to start; last moment; interrupted"));
DEFINE_STATIC_NAME_STR(lsWhereToStartCheckpoint, TXT("hub; game menu; where to start; checkpoint"));
DEFINE_STATIC_NAME_STR(lsWhereToStartAtLeastHalfHealth, TXT("hub; game menu; where to start; at least half health"));
DEFINE_STATIC_NAME_STR(lsWhereToStartChapterStart, TXT("hub; game menu; where to start; chapter start"));
DEFINE_STATIC_NAME_STR(lsWhereToStartRestart, TXT("hub; game menu; where to start; restart"));

DEFINE_STATIC_NAME_STR(lsModifiers, TXT("hub; game menu; modifiers"));
DEFINE_STATIC_NAME_STR(lsUnlocks, TXT("hub; game menu; unlocks"));
DEFINE_STATIC_NAME_STR(lsStats, TXT("hub; game menu; stats"));

DEFINE_STATIC_NAME_STR(lsExperienceToSpend, TXT("hub; stats info; experience to spend"));
DEFINE_STATIC_NAME_STR(lsMeritPointsToSpend, TXT("hub; stats info; merit points to spend"));

DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));

DEFINE_STATIC_NAME_STR(lsQuitGame, TXT("hub; game menu; quit game"));
DEFINE_STATIC_NAME_STR(lsQuitGameQuestion, TXT("hub; question; quit game"));

DEFINE_STATIC_NAME_STR(lsShowHandGestures, TXT("hand gestures; show"));

DEFINE_STATIC_NAME_STR(lsGameStateExperience, TXT("hub; game menu; game state; experience"));
DEFINE_STATIC_NAME(experience);
DEFINE_STATIC_NAME_STR(lsGameStateTimeDistance, TXT("hub; game menu; game state; time distance"));
DEFINE_STATIC_NAME(time);
DEFINE_STATIC_NAME(distance);
DEFINE_STATIC_NAME_STR(lsGameStateEnergyLevels, TXT("hub; game menu; game state; energy levels"));
DEFINE_STATIC_NAME_STR(lsGameStateEnergyLevelsCommonAmmo, TXT("hub; game menu; game state; energy levels; common ammo"));
DEFINE_STATIC_NAME(health);
DEFINE_STATIC_NAME(handLeft);
DEFINE_STATIC_NAME(handRight);
DEFINE_STATIC_NAME(ammo);

DEFINE_STATIC_NAME_STR(lsRenameProfile, TXT("hub; pil.set; rename profile"));
DEFINE_STATIC_NAME_STR(lsRemoveProfile, TXT("hub; pil.set; remove profile"));
DEFINE_STATIC_NAME_STR(lsChangeGameSlotProfile, TXT("hub; pil.set; change game slot profile"));
DEFINE_STATIC_NAME_STR(lsProvideNonEmptyProfileName, TXT("hub; pil.set; provide non empty profile name"));
DEFINE_STATIC_NAME_STR(lsAlreadyExistsSuchProfile, TXT("hub; pil.set; already exist such profile"));
DEFINE_STATIC_NAME_STR(lsRemoveProfileQuestion, TXT("hub; pil.set; remove profile question"));
DEFINE_STATIC_NAME_STR(lsRemoveProfileQuestionSecond, TXT("hub; pil.set; remove profile question second"));
DEFINE_STATIC_NAME_STR(lsChangedProfileName, TXT("hub; pil.set; changed profile name"));
DEFINE_STATIC_NAME(profileName);
DEFINE_STATIC_NAME_STR(lsRestartTheAdventureDueToBuild, TXT("hub; pil.set; restart the adventure due to build"));

DEFINE_STATIC_NAME_STR(lsRemoveGameSlot, TXT("hub; pil.set; remove game slot"));
DEFINE_STATIC_NAME_STR(lsRemoveGameSlotQuestion, TXT("hub; pil.set; remove game slot question"));
DEFINE_STATIC_NAME_STR(lsRemoveGameSlotQuestionSecond, TXT("hub; pil.set; remove game slot question second"));

DEFINE_STATIC_NAME_STR(lsGameSlotInfo, TXT("hub; pil.set; game slot info"));
	DEFINE_STATIC_NAME(gameSlotID);
	DEFINE_STATIC_NAME(gameType);
	DEFINE_STATIC_NAME(gameSubType);

// language icon
DEFINE_STATIC_NAME(small);

// input
DEFINE_STATIC_NAME(tabLeft);
DEFINE_STATIC_NAME(tabRight);
DEFINE_STATIC_NAME(shift);

//

using namespace Loader;
using namespace HubScenes;

//

class GameStateOnList
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(GameStateOnList);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	TeaForGodEmperor::GameState* gameState = nullptr;
	Optional<TeaForGodEmperor::GameStateLevel::Type> startWhat;
	bool startAllOver = false;
	bool newMission = false;
	bool abortMission = false;
	Optional<TeaForGodEmperor::GameSlotMode::Type> switchGameSlotMode;

	List<String> lines; // for add new lines

	static void draw_element(float inElementMargin, HubWidgets::List* list, Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
	{
		list->draw_element_border(_display, _at, _element, false/*selectedGameStateElement == _element*/);
		if (auto* gsol = fast_cast<GameStateOnList>(_element->data.get()))
		{
			if (auto* font = _display->get_font())
			{
				Vector2 at(_at.x.min + inElementMargin, _at.y.max - inElementMargin);
				Colour textColour = HubColours::text();// selectedGameStateElement == _element ? HubColours::selected_highlighted() : HubColours::text();

				float inElementLineDistance = ((_at.y.max - _at.y.min) - inElementMargin * 2.0f) / max(1.0f, (float)max(4, gsol->lines.get_size()));
				for_every(l, gsol->lines)
				{
					float scale = 1.0f;
					float ptY = 0.0f;
					Optional<float> spacingScale;
					int i = for_everys_index(l);
					if (gsol->lines.get_size() == 1 ||
						(gsol->lines.get_size() == 2 && gsol->lines[1].is_empty()))
					{
						scale = 2.0f;
						spacingScale = 1.0f;
						ptY = 0.85f;
						inElementLineDistance = ((_at.y.max - _at.y.min) - inElementMargin * 2.0f); // recalculate for single line
					}
					else
					{
						if (i == 0) { scale = 0.75f; ptY = 1.0f; } // looks ok on given display
						if (i == 1) { scale = 1.5f; spacingScale = 1.25f; ptY = 0.6f; }
					}
					at.y -= inElementLineDistance * spacingScale.get(scale) * (1.0f - ptY);
					font->draw_text_at(::System::Video3D::get(), l->to_char(), textColour, at, Vector2::one * scale, Vector2(0.0f, ptY));
					at.y -= inElementLineDistance * spacingScale.get(scale) * ptY;
				}
			}
		}
	}
};
REGISTER_FOR_FAST_CAST(GameStateOnList);

//

class TestScenarioOnList
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(TestScenarioOnList);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	Framework::LibraryBasedParameters* scenario = nullptr;

	String info;
};
REGISTER_FOR_FAST_CAST(TestScenarioOnList);

//

PilgrimageSetup::ShowMainPanel PilgrimageSetup::showMainPanel = PilgrimageSetup::MP_GameModifiers;
bool PilgrimageSetup::s_gameModifiersRequestedOnNextSetup = false;
bool PilgrimageSetup::s_choosePilgrimageOnNextSetup = false;
bool PilgrimageSetup::s_reachedEnd = false;

REGISTER_FOR_FAST_CAST(PilgrimageSetup);

void PilgrimageSetup::request_game_modifiers_on_next_setup()
{
	s_gameModifiersRequestedOnNextSetup = true;
}

void PilgrimageSetup::request_choose_pilgrimage_on_next_setup()
{
	s_choosePilgrimageOnNextSetup = true;
}

void PilgrimageSetup::mark_reached_end(bool _mark)
{
	s_reachedEnd = _mark;
}

void PilgrimageSetup::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	if (!mainDir.is_set())
	{
		mainDir = get_hub()->get_background_dir_or_main_forward();
	}

	deactivateHub = false;

	runSetup.read_into_this();
	//
	auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
	if (auto* gs = ps.get_active_game_slot())
	{
		runSetup = gs->runSetup;
	}
	//
	runSetup.update_using_this();

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(pilgrimageSetupVersionInfo));
	screensToKeep.push_back(NAME(pilgrimageSetupMainPanel_Main));
	screensToKeep.push_back(NAME(pilgrimageSetupMainPanel_TestScenarios));
	screensToKeep.push_back(NAME(pilgrimageSetupMainPanel_GameModifiers));
	screensToKeep.push_back(NAME(pilgrimageSetupMainPanel_Unlocks));
	screensToKeep.push_back(NAME(pilgrimageSetupMainPanel_Stats));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuBottom));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuBottom_Back));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuRight));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuWhereToStart));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuWhereToStart_Left));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuWhereToStart_Right));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuWhereToTestStart));
	screensToKeep.push_back(NAME(pilgrimageSetupMenuLeft));
	screensToKeep.push_back(NAME(pilgrimageSetupProfileGameSlot));
	screensToKeep.push_back(NAME(pilgrimageSetupRealTime));
	screensToKeep.push_back(NAME(hubBackground));
	//
	get_hub()->keep_only_screens(screensToKeep);

	//HubScreens::SpirographMandala::show(get_hub(), NAME(hubBackground));

	selectedGameStateElement.clear();

	bool showMainPanelNow = true;

	if (auto* gs = ps.access_active_game_slot())
	{
		if (gs->buildNo < BUILD_NO__RESTART_GAME_SLOT_PROPOSAL &&
			!is_flag_set(gs->buildAcknowledgedChangesFlags, BUILD_NO__ACK_FLAG__RESTART_ADVENTURE))
		{
			set_flag(gs->buildAcknowledgedChangesFlags, BUILD_NO__ACK_FLAG__RESTART_ADVENTURE);
			bool proposeRestart = gs->has_any_game_states();
			if (!proposeRestart)
			{
				if (gs->restartAtPilgrimage.is_valid())
				{
					if (auto* gd = gs->gameDefinition.get())
					{
						if (!gd->get_pilgrimages().is_empty())
						{
							if (gs->restartAtPilgrimage != gd->get_pilgrimages().get_first()->get_name())
							{
								proposeRestart = true;
							}
						}
					}
				}
			}
			if (proposeRestart)
			{
				showMainPanelNow = false;

				HubScreens::Question::ask(get_hub(), NAME(lsRestartTheAdventureDueToBuild),
					[this, gs]()
					{
						gs->clear_game_states_and_mission_state_and_pilgrimages();

						show_main_panel(MP_MainMenu);

						allow_auto_tips();
					},
					[this]()
					{
						show_main_panel(MP_MainMenu);

						allow_auto_tips();
					}
					);
			}
		}
	}

	if (showMainPanelNow)
	{
		ShowMainPanel panel = MP_MainMenu;
		if (s_choosePilgrimageOnNextSetup) panel = MP_ChooseStart;
		if (s_gameModifiersRequestedOnNextSetup) panel = MP_GameModifiers;
		show_main_panel(panel);

		allow_auto_tips();
	}

	//
	s_gameModifiersRequestedOnNextSetup = false;
	s_choosePilgrimageOnNextSetup = false;
 }

void PilgrimageSetup::show_main_panel(Optional<ShowMainPanel> const& _showMainPanel)
{
	if (_showMainPanel.is_set())
	{
		showMainPanel = _showMainPanel.get();
	}

	bool isMainMenu = showMainPanel == MP_MainMenu;

	float radius = get_hub()->get_radius() * 0.6f;
	float mainWidth = 50.0f;
	float mainHeight = mainPanelHeight.get(50.0f);
	float bottomHeight = 10.0f;
	float mainPitch = 0.0f;
	float spacing = 3.0f;
	float menuSideWidth = 15.0f;
	float whereToStartWidth = mainWidth;//45.0f;
	float menuPPA = 24.0f;
	float smallMenuPPA = 30.0f;

	// main panel
	{
		// game modifiers (first to get the size)
		{
			Vector2 size(mainWidth, 50.0f); // force big/initial as we change the space every time and we could start getting smaller and smaller
			Vector2 ppa(30.0f, 30.0f);
			Rotator3 atDir = mainDir.get();
			//atDir.yaw -= 10.0f;
			atDir.pitch = mainPitch;

			if (!mainPanelHeight.is_set())
			{
				HubScreen* screen = new HubScreens::GameModifiers(get_hub(), false, NAME(pilgrimageSetupMainPanel_GameModifiers), runSetup, nullptr, NP, nullptr,
					atDir, size, radius, ppa, true /* for height calculation */);
				screen->blockTriggerHold = true;

				mainPanelHeight = screen->size.y;

				delete screen;
			}

			if (showMainPanel == MP_GameModifiers)
			{
				HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_GameModifiers));
				if (!screen)
				{
					screen = new HubScreens::GameModifiers(get_hub(), false, NAME(pilgrimageSetupMainPanel_GameModifiers), runSetup, nullptr, NP, nullptr,
						atDir, size, radius, ppa);
					screen->blockTriggerHold = true;

					screen->activate();
					get_hub()->add_screen(screen);
				}
			}
			else
			{
				if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_GameModifiers)))
				{
					screen->deactivate();
				}
			}
		}

		mainHeight = mainPanelHeight.get(50.0f);

		// main
		{
			HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_Main));

			if (!isMainMenu)
			{
				if (screen)
				{
					screen->deactivate();
				}
			}
			else if (!screen)
			{
				Vector2 size(mainWidth, mainHeight);
				Vector2 ppa(menuPPA, menuPPA);
				Rotator3 atDir = mainDir.get();
				atDir.pitch = mainPitch;

				screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMainPanel_Main), size, atDir, radius, true, NP, ppa);

				Vector2 fs = HubScreen::s_fontSizeInPixels;
				fs.y = fs.x = max(fs.x, fs.y);
				Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);
				Range2 availableSpaceTopRight = availableSpace;

				availableSpaceTopRight.x.min = round(availableSpaceTopRight.x.get_at(0.4f));
				availableSpaceTopRight.y.min = round(availableSpaceTopRight.y.get_at(0.4f));

				float buttonSpacing = fs.x;

				Range2 availableSpaceBottomLeft = availableSpace;
				availableSpaceBottomLeft.x.max = availableSpaceTopRight.x.min - buttonSpacing;
				availableSpaceBottomLeft.y.max = availableSpaceTopRight.y.min - buttonSpacing;

				Range2 availableSpaceBottomRight = availableSpaceBottomLeft;
				availableSpaceBottomRight.x = availableSpaceTopRight.x;

				Range2 availableSpaceTopLeft = availableSpaceTopRight;
				availableSpaceTopLeft.x = availableSpaceBottomLeft.x;

				Array<HubScreenButtonInfo> buttons;

				{
					buttons.clear();
					Name startLSId = NAME(lsStart);
					float scale = 4.0;
					ShowMainPanel showPanel = MP_ChooseStart;
					if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
					{
						if (gs->gameSlotMode == TeaForGodEmperor::GameSlotMode::Missions)
						{
							startLSId = NAME(lsMission);
							scale = 3.0f;
							showPanel = MP_ChooseMission;
						}
					}
					buttons.push_back(HubScreenButtonInfo(NAME(idStart), startLSId, [this, showPanel]() { show_main_panel(showPanel); }).activate_on_trigger_hold().with_scale(scale));
					screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
						availableSpaceTopRight, fs);
				}

				{
					buttons.clear();
					String caption = LOC_STR(NAME(lsModifiers));
					buttons.push_back(HubScreenButtonInfo(Name::invalid(), caption, [this]() { show_main_panel(MP_GameModifiers); }).with_scale(2.0f)
						.with_custom_draw(
							[](Framework::Display* _display, Range2 const& _at, HubWidgetUsedColours const& _useColours)	// custom draw
							{
								if (auto* font = _display->get_font())
								{
									float fontHeight = font->get_height();

									Range2 available = _at;

									{
										Range2 at = available;
										at.y.min = at.y.centre() - fontHeight * 4.0f;
										at.y.max = at.y.min;

										String caption = String::printf(TXT("x%S"), (TeaForGodEmperor::GameSettings::get().experienceModifier + TeaForGodEmperor::EnergyCoef::one()).as_string_percentage_auto_decimals().to_char());

										font->draw_text_at(::System::Video3D::get(), caption, _useColours.textColour, at.centre(), Vector2(1.0f), Vector2::half);
									}
								}
							})
					);
					screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
						availableSpaceTopLeft, fs);
				}

				{
					buttons.clear();
					buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsStats), [this]() { show_main_panel(MP_Stats); }).with_scale(2.0f));
					screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
						availableSpaceBottomRight, fs);
				}

				{
					buttons.clear();
					buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsUnlocks), [this]() { show_main_panel(MP_Unlocks); }).with_scale(2.0f));
					screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
						availableSpaceBottomLeft, fs);
				}

				screen->activate();
				get_hub()->add_screen(screen);
			}
		}

		// test scenarios
#ifndef BUILD_PUBLIC_RELEASE
		{
			HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_TestScenarios));

			if (!isMainMenu)
			{
				if (screen)
				{
					screen->deactivate();
				}
			}
			else if (!screen)
			{
				Vector2 size(mainWidth * 0.3f, mainHeight);
				Vector2 ppa(menuPPA, menuPPA);
				Rotator3 atDir = mainDir.get();
				atDir.yaw += mainWidth * 0.5f + spacing + size.x * 0.5f;
				atDir.pitch = mainPitch;

				screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMainPanel_TestScenarios), size, atDir, radius, true, NP, ppa);

				Vector2 fs = HubScreen::s_fontSizeInPixels;
				fs.y = fs.x = max(fs.x, fs.y);
				Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);
				Range2 availableSpaceTopRight = availableSpace;

				availableSpaceTopRight.x.min = round(availableSpaceTopRight.x.get_at(0.4f));
				availableSpaceTopRight.y.min = round(availableSpaceTopRight.y.get_at(0.4f));

				Array<HubScreenButtonInfo> buttons;

				{
					buttons.clear();
					buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("test~~sce~nar~ios")), [this]() { show_main_panel(MP_ChooseTestStart); }).with_scale(2.0f));
					screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
						availableSpace, fs);
				}

				screen->activate();
				get_hub()->add_screen(screen);
			}
		}
#endif

		// unlocks
		{
			if (showMainPanel == MP_Unlocks)
			{
				HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_Unlocks));
				if (!screen)
				{
					Vector2 size(mainWidth, mainHeight);
					Vector2 ppa(30.0f, 30.0f);
					Rotator3 atDir = mainDir.get();
					//atDir.yaw -= 10.0f;
					atDir.pitch = mainPitch;

					screen = new HubScreens::Unlocks(get_hub(), NAME(pilgrimageSetupMainPanel_Unlocks), atDir, size, radius, ppa);
					screen->blockTriggerHold = true;

					screen->activate();
					get_hub()->add_screen(screen);
				}
			}
			else
			{
				if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_Unlocks)))
				{
					screen->deactivate();
				}
			}
		}

		// stats
		{
			if (showMainPanel == MP_Stats)
			{
				HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_Stats));
				if (!screen)
				{
					Vector2 size(mainWidth, mainHeight);
					Vector2 ppa(30.0f, 30.0f);
					Rotator3 atDir = mainDir.get();
					//atDir.yaw -= 10.0f;
					atDir.pitch = mainPitch;

					screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMainPanel_Stats), size, atDir, radius, true, NP, ppa);
					screen->blockTriggerHold = true;

					Array<HubScreenOption> options;
					options.make_space_for(20);

					auto& ps = TeaForGodEmperor::PlayerSetup::get_current();

					//options.push_back(HubScreenOption(ps.get_profile_name()));
					if (auto* gs = ps.get_active_game_slot())
					{
						if (auto* gd = gs->get_game_definition())
						{
							options.push_back(HubScreenOption(gs->get_ui_name() + String::space() + gd->get_name_for_ui().get()));
						}
						else
						{
							options.push_back(HubScreenOption(gs->get_ui_name()));
						}
					}
					options.push_back(HubScreenOption());

					if (auto* gs = ps.get_active_game_slot())
					{
						Utils::add_options_from_player_stats(gs->stats, options, Utils::PlayerStatsFlags::Global);
					}

					options.push_back(HubScreenOption());
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsExperienceToSpend), nullptr, HubScreenOption::Text).with_text(TeaForGodEmperor::Persistence::get_current().get_experience_to_spend().as_string_auto_decimals()+LOC_STR(NAME(lsCharsExperience))).with_smaller_decimals(0.5f));
					if (TeaForGodEmperor::Persistence::get_current().get_merit_points_to_spend() != 0)
					{
						options.push_back(HubScreenOption(Name::invalid(), NAME(lsMeritPointsToSpend), nullptr, HubScreenOption::Text).with_text(String::printf(TXT("%i"), TeaForGodEmperor::Persistence::get_current().get_merit_points_to_spend()) + LOC_STR(NAME(lsCharsMeritPoints))).with_smaller_decimals(0.5f));
					}

					Vector2 fs = HubScreen::s_fontSizeInPixels;
					auto availableSpace = screen->mainResolutionInPixels;
					screen->add_option_widgets(options, availableSpace.expanded_by(-Vector2(max(fs.x, fs.y))),NP, fs.y * 0.6f, NP, NP, 2.0f);

					screen->activate();
					get_hub()->add_screen(screen);

				}
			}
			else
			{
				if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMainPanel_Stats)))
				{
					screen->deactivate();
				}
			}
		}
	}

	{	// top info
		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupVersionInfo));

		if (!screen)
		{
			Vector2 ppa = Vector2(24.0f, 24.0f);

			String text(get_build_version());

			Vector2 sizeInPixels = Vector2(text.get_length() + 0.0f, 1.0f) * HubScreen::s_fontSizeInPixels;

			auto* img = get_hub()->get_graphics(HubGraphics::LogoBigOneLine);

			Vector2 imgScale = Vector2(1.0f, 1.0f);
			Vector2 borderSize = Vector2(3.0f, 3.0f);
			float spacing = 3.0f;
			if (img)
			{
				Vector2 imgSize = img->calc_rect().length().to_vector2();
				sizeInPixels.x = max(sizeInPixels.x, (float)imgSize.x * imgScale.x);
				sizeInPixels.y += (float)imgSize.y * imgScale.y;
				sizeInPixels.y += spacing;
			}

			sizeInPixels += borderSize * 2.0f;

			Vector2 size = sizeInPixels / ppa;
			size.x = max(size.x, mainWidth);

			Rotator3 atDir = mainDir.get();
			atDir.pitch = mainHeight * 0.5f + mainPitch + spacing + size.y * 0.5f;

			screen = new HubScreen(get_hub(), NAME(pilgrimageSetupVersionInfo), size,
				atDir, get_hub()->get_radius() * 0.5f, true, NP, ppa);
			screen->blockTriggerHold = true;

			{
				Range2 at = screen->mainResolutionInPixels;
				at.y.min += borderSize.y;
				auto* w = new HubWidgets::Text(Name::invalid(), at, text);
				w->alignPt.y = 0.0f;
				screen->add_widget(w);
			}

			if (img)
			{
				Range2 at = screen->mainResolutionInPixels;
				at.y.max -= borderSize.y;
				auto* w = new HubWidgets::Image(Name::invalid(), at, img, imgScale, HubColours::text());
				w->alignPt.y = 1.0f;
				screen->add_widget(w);
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
	} 

	/*
	// left menu
	{
		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuLeft));

		if (!screen)
		{
			Vector2 size(menuSideWidth, mainHeight);
			Vector2 ppa(menuPPA, menuPPA);
			Rotator3 atDir = mainDir.get();
			atDir.yaw += -(mainWidth * 0.5f + spacing + size.x * 0.5f);
			atDir.pitch = mainPitch;

			screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMenuLeft), size, atDir, radius, true, NP, ppa);
			screen->blockTriggerHold = true;

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			fs.y = fs.x = max(fs.x, fs.y);			
			Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

			Array<HubScreenButtonInfo> buttons;

			{
				buttons.clear();
				buttons.push_back(HubScreenButtonInfo(NAME(lsModifiers), NAME(lsModifiers), [this]() { show_main_panel(MP_GameModifiers); }));
				buttons.push_back(HubScreenButtonInfo(NAME(lsUnlocks), NAME(lsUnlocks), [this]() { show_main_panel(MP_Unlocks); }));
				buttons.push_back(HubScreenButtonInfo(NAME(lsStats), NAME(lsStats), [this]() { show_main_panel(MP_Stats); }));
				screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
					availableSpace, fs);
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}
	*/

	float bottomAt = -mainHeight * 0.5f + mainPitch;

	// game slot
	{
		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupProfileGameSlot));

		if (!isMainMenu)
		{
			if (screen)
			{
				screen->deactivate();
			}
		}
		else if (!screen)
		{
			Vector2 size(menuSideWidth, bottomHeight);
			Vector2 ppa(smallMenuPPA, smallMenuPPA);
			Rotator3 atDir = mainDir.get();
			atDir.yaw += -mainWidth * 0.5f - spacing - size.x * 0.5f;
			atDir.pitch = bottomAt - spacing - size.y * 0.5f;

			screen = new HubScreen(get_hub(), NAME(pilgrimageSetupProfileGameSlot), size, atDir, radius, true, NP, ppa);
			screen->blockTriggerHold = true;

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			fs.y = fs.x = max(fs.x, fs.y);
			Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

			Array<HubScreenButtonInfo> buttons;

			{
				buttons.clear();
				buttons.push_back(HubScreenButtonInfo(NAME(idProfileGameSlot),
					Framework::StringFormatter::format_loc_str(NAME(lsProfileGameSlot), Framework::StringFormatterParams()
						.add(NAME(profileName), TeaForGodEmperor::PlayerSetup::access_current().get_profile_name())
						.add(NAME(gameSlotID), TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot() ? TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot()->id : 1)
						.add(NAME(gameType), TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot() &&
											 TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot()->gameDefinition.get()?
												TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot()->gameDefinition->get_name_for_ui().get() : String::empty())
						.add(NAME(gameSubType), TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot() &&
											 ! TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot()->gameSubDefinitions.is_empty() &&
											 TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot()->gameSubDefinitions[0].get()?
												TeaForGodEmperor::PlayerSetup::access_current().get_active_game_slot()->gameSubDefinitions[0]->get_name_for_ui().get() : String::empty())
					), [this]() { manage_profile(); }));
				screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
					availableSpace, fs);
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}

	// real time 
	{
		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupRealTime));

		if (!screen && TeaForGodEmperor::PlayerSetup::get_current().get_preferences().showRealTime &&
			showMainPanel != MP_ChooseStart &&
			showMainPanel != MP_ChooseMission &&
			showMainPanel != MP_ChooseTestStart)
		{
			Vector2 size(menuSideWidth, bottomHeight);
			Vector2 ppa(smallMenuPPA, smallMenuPPA);
			Rotator3 atDir = mainDir.get();
			atDir.yaw += mainWidth * 0.5f + spacing + size.x * 0.5f;
			atDir.pitch = bottomAt - spacing - size.y * 0.5f;

			screen = new HubScreen(get_hub(), NAME(pilgrimageSetupRealTime), size, atDir, radius, true, NP, ppa);
			
			Vector2 fs = HubScreen::s_fontSizeInPixels;
			fs.y = fs.x = max(fs.x, fs.y);
			Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

			{
				auto* w = new HubWidgets::Text(NAME(idRealTime), availableSpace, String::empty());
				w->scale = Vector2::one * 3.0f;
				w->forceFixedSize = true;
				screen->add_widget(w);
				realTimeSeconds = 0; // to force update
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
		else if (screen)
		{
			screen->deactivate();
		}
	}

	{
		// update before we start choosing
		allPilgrimagesUnlockedCheat = false;
		if (showMainPanel == MP_ChooseStart)
		{
			if (get_hub()->get_hand(Hand::Left).input.is_button_pressed(NAME(shift)) &&
				get_hub()->get_hand(Hand::Right).input.is_button_pressed(NAME(shift)))
			{
				allPilgrimagesUnlockedCheat = true;
#ifdef VERBOSE_RESTART
				output(TXT("all pilgrimages unlocked [cheat]"));
#endif
			}
			else
			{
#ifdef VERBOSE_RESTART
				output(TXT("don't use \"all pilgrimages unlocked\" cheat"));
#endif
			}
		}
	}

	// where to start
	{
		if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuWhereToStart_Left)))
		{
			screen->deactivate();
		}
		if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuWhereToStart_Right)))
		{
			screen->deactivate();
		}

		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuWhereToStart));

		struct AvailableGameState
		{
			TeaForGodEmperor::GameState* gameState = nullptr;
			Optional<TeaForGodEmperor::GameStateLevel::Type> startWhat; // if not set, restart the whole thing
			bool startAllOver = false;
			bool newMission = false;
			bool abortMission = false;
			Optional<TeaForGodEmperor::GameSlotMode::Type> switchGameSlotMode;

			AvailableGameState() {}
			AvailableGameState(TeaForGodEmperor::GameState* _gameState, Optional<TeaForGodEmperor::GameStateLevel::Type> const & _startWhat) : gameState(_gameState), startWhat(_startWhat), startAllOver(! _startWhat.is_set()) {}
			static AvailableGameState new_mission() { AvailableGameState a; a.newMission = true; return a; }
			static AvailableGameState abort_mission() { AvailableGameState a; a.abortMission = true; return a; }
			static AvailableGameState switch_game_slot_mode(TeaForGodEmperor::GameSlotMode::Type _to) { AvailableGameState a; a.switchGameSlotMode = _to; return a; }

			GameStateOnList* create_game_slot_on_list()
			{
				auto* gs = this;
				auto* gsol = new GameStateOnList();
				gsol->gameState = gs->gameState;
				gsol->startWhat = gs->startWhat;
				gsol->startAllOver = gs->startAllOver;
				if (gs->newMission)
				{
					gsol->newMission = true;
					gsol->lines.push_back(String::empty());
					gsol->lines.push_back(LOC_STR(NAME(lsNewMission)));
				}
				else if (gs->abortMission)
				{
					gsol->abortMission = true;
					gsol->lines.push_back(String::empty());
					gsol->lines.push_back(LOC_STR(NAME(lsAbortMission)));
				}
				else if (gs->switchGameSlotMode.is_set())
				{
					gsol->switchGameSlotMode = gs->switchGameSlotMode;
					gsol->lines.push_back(String::empty());
					if (gs->switchGameSlotMode.get() == TeaForGodEmperor::GameSlotMode::Story)
					{
						gsol->lines.push_back(LOC_STR(NAME(lsSwitchToStory)));
					}
					else if (gs->switchGameSlotMode.get() == TeaForGodEmperor::GameSlotMode::Missions)
					{
						gsol->lines.push_back(LOC_STR(NAME(lsSwitchToMissions)));
					}
					else
					{
						todo_implement;
					}
				}
				else
				{
					if (gs->startAllOver)
					{
						gsol->lines.push_back(LOC_STR(NAME(lsWhereToStartRestart)));
					}
					else if (gs->startWhat.get() == TeaForGodEmperor::GameStateLevel::LastMoment)
					{
						if (gs->gameState && gs->gameState->interrupted)
						{
							gsol->lines.push_back(LOC_STR(NAME(lsWhereToStartLastMomentInterrupted)));
						}
						else
						{
							gsol->lines.push_back(LOC_STR(NAME(lsWhereToStartLastMoment)));
						}
					}
					else if (gs->startWhat.get() == TeaForGodEmperor::GameStateLevel::Checkpoint)
					{
						gsol->lines.push_back(LOC_STR(NAME(lsWhereToStartCheckpoint)));
					}
					else if (gs->startWhat.get() == TeaForGodEmperor::GameStateLevel::AtLeastHalfHealth)
					{
						gsol->lines.push_back(LOC_STR(NAME(lsWhereToStartAtLeastHalfHealth)));
					}
					else if (gs->startWhat.get() == TeaForGodEmperor::GameStateLevel::ChapterStart)
					{
						gsol->lines.push_back(LOC_STR(NAME(lsWhereToStartChapterStart)));
					}
					if (gsol->gameState)
					{
						String pilgrimageName;
						if (auto* p = gsol->gameState->pilgrimage.get())
						{
							String connector = p->get_pilgrimage_name_pre_connector_ls().get().is_empty() ? String::space() : p->get_pilgrimage_name_pre_connector_ls().get();
							pilgrimageName = (p->get_pilgrimage_name_pre_short_ls().get() + connector + p->get_pilgrimage_name_ls().get()).trim();
						}
						MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
						String distanceString;
						if (ms == MeasurementSystem::Imperial)
						{
							distanceString = String::printf(TXT("%.0fyd"), MeasurementSystem::to_yards(gsol->gameState->gameStats.distance));
						}
						else
						{
							distanceString = String::printf(TXT("%.0fm"), gsol->gameState->gameStats.distance);
						}
						String gameStateCreationTime = gsol->gameState->when.get_string_wide_compact_date_time(Framework::DateTimeFormat(Framework::DateFormat::YMD, Framework::TimeFormat::H24),
							false // no seconds
						);
						//String experience = Framework::StringFormatter::format_loc_str(NAME(lsGameStateExperience), Framework::StringFormatterParams()
						//	.add(NAME(experience), gsol->gameState->gameStats.experience.as_string(0)));
						String timeDistance = Framework::StringFormatter::format_loc_str(NAME(lsGameStateTimeDistance), Framework::StringFormatterParams()
							.add(NAME(time), Framework::Time::from_seconds(gsol->gameState->gameStats.timeInSeconds).get_string_short_time())
							.add(NAME(distance), distanceString));
						String energyLevels;
						if (TeaForGodEmperor::GameSettings::get().difficulty.commonHandEnergyStorage)
						{
							energyLevels = Framework::StringFormatter::format_loc_str(NAME(lsGameStateEnergyLevelsCommonAmmo), Framework::StringFormatterParams()
								.add(NAME(health), gsol->gameState->gear->initialEnergyLevels.health.as_string(0))
								.add(NAME(ammo), gsol->gameState->gear->initialEnergyLevels.hand[Hand::Left].as_string(0)));
						}
						else
						{
							energyLevels = Framework::StringFormatter::format_loc_str(NAME(lsGameStateEnergyLevels), Framework::StringFormatterParams()
								.add(NAME(health), gsol->gameState->gear->initialEnergyLevels.health.as_string(0))
								.add(NAME(handLeft), gsol->gameState->gear->initialEnergyLevels.hand[Hand::Left].as_string(0))
								.add(NAME(handRight), gsol->gameState->gear->initialEnergyLevels.hand[Hand::Right].as_string(0)));
						}

						gsol->lines.push_back(pilgrimageName);
						gsol->lines.push_back(gameStateCreationTime);
						gsol->lines.push_back(timeDistance + TXT("   ") + energyLevels);
					}
					else
					{
						// this order, remember that change_pilgrimage_restart depends on this order
						gsol->lines.push_back(String::empty()); // placeholder for pilgrimage name
					}
				}
				return gsol;
			}
		};
		Array<AvailableGameState> availableGameStates;

#ifdef VERBOSE_GAME_STATES
		output(TXT("prepare available game states"));
#endif
		if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
		{
			if (gs->lastMoment.is_set()) // should be always available to load
			{
				availableGameStates.push_back(AvailableGameState(gs->lastMoment.get(), TeaForGodEmperor::GameStateLevel::LastMoment));
#ifdef VERBOSE_GAME_STATES
				output(TXT(" + %S"), TeaForGodEmperor::GameStateLevel::to_char((TeaForGodEmperor::GameStateLevel::Type)availableGameStates.get_last().startWhat.get()));
#endif
			}
			if (TeaForGodEmperor::GameSettings::get().difficulty.restartMode <= TeaForGodEmperor::GameSettings::RestartMode::AnyCheckpoint)
			{
				if (gs->checkpoint.is_set())
				{
					availableGameStates.push_back(AvailableGameState(gs->checkpoint.get(), TeaForGodEmperor::GameStateLevel::Checkpoint));
#ifdef VERBOSE_GAME_STATES
					output(TXT(" + %S"), TeaForGodEmperor::GameStateLevel::to_char((TeaForGodEmperor::GameStateLevel::Type)availableGameStates.get_last().startWhat.get()));
#endif
				}
				if (gs->atLeastHalfHealth.is_set())
				{
					availableGameStates.push_back(AvailableGameState(gs->atLeastHalfHealth.get(), TeaForGodEmperor::GameStateLevel::AtLeastHalfHealth));
#ifdef VERBOSE_GAME_STATES
					output(TXT(" + %S"), TeaForGodEmperor::GameStateLevel::to_char((TeaForGodEmperor::GameStateLevel::Type)availableGameStates.get_last().startWhat.get()));
#endif
				}
			}
			if (showMainPanel == MP_ChooseMission)
			{
				if (availableGameStates.is_empty())
				{
					availableGameStates.push_back(AvailableGameState::new_mission()); // to abort
				}
				else
				{
					availableGameStates.push_back(AvailableGameState::abort_mission()); // to abort
				}
			}
			if (showMainPanel == MP_ChooseStart)
			{
				if (TeaForGodEmperor::GameSettings::get().difficulty.restartMode <= TeaForGodEmperor::GameSettings::RestartMode::ReachedChapter && gs->chapterStart.is_set())
				{
					availableGameStates.push_back(AvailableGameState(gs->chapterStart.get(), TeaForGodEmperor::GameStateLevel::ChapterStart));
#ifdef VERBOSE_GAME_STATES
					output(TXT(" + %S"), TeaForGodEmperor::GameStateLevel::to_char((TeaForGodEmperor::GameStateLevel::Type)availableGameStates.get_last().startWhat.get()));
#endif
				}

				if (TeaForGodEmperor::GameSettings::get().difficulty.restartMode <= TeaForGodEmperor::GameSettings::RestartMode::UnlockedChapter)
				{
					if (auto* gd = TeaForGodEmperor::GameDefinition::get_chosen())
					{
						gs->update_unlocked_pilgrimages(TeaForGodEmperor::PlayerSetup::get_current(), gd->get_pilgrimages(), gd->get_conditional_pilgrimages(), true);
					}
#ifdef ALLOW_CHEAP_CHEATS
					// if we're actually allowed to (above)
					// and only if we can choose anything (below)
					if (!availableGameStates.is_empty()
						|| !gs->unlockedPilgrimages.is_empty() // this is empty if we haven't started yet
						|| allPilgrimagesUnlockedCheat
#ifdef BUILD_PREVIEW
#ifdef ALLOW_ALL_CHAPTERS_FOR_PREVIEW_BUILDS
						|| true // easy access to all chapters for preview
#endif
#endif				
						)
					{
						availableGameStates.push_back(AvailableGameState(nullptr, NP)); // restart
#ifdef VERBOSE_GAME_STATES
						output(TXT(" + RESTART AT ANY CHAPTER"));
#endif
					}
#endif
				}
			}
			if (! TeaForGodEmperor::is_demo())
			{
				Array<TeaForGodEmperor::GameSlotMode::Type> availableGameSlotModes;
				availableGameSlotModes = gs->gameSlotModesAvailable;
				availableGameSlotModes.push_back_unique(TeaForGodEmperor::GameSlotMode::Story); // make sure it is added
#ifdef ALL_GAME_MODES_AVAILABLE_SINCE_START
				// add all we know
				availableGameSlotModes.push_back_unique(TeaForGodEmperor::GameSlotMode::Missions);
#endif
				// now change modes into actually available entries
				for_every(gsm, availableGameSlotModes)
				{
					if (*gsm != gs->gameSlotMode)
					{
						availableGameStates.push_back(AvailableGameState::switch_game_slot_mode(*gsm));
					}
				}
			}
		}

		// remove doubled states
		{ 
#ifdef VERBOSE_GAME_STATES
			output(TXT("remove doubled game states"));
#endif
			for (int i = 0; i < availableGameStates.get_size() - 1; ++i)
			{
				auto& a = availableGameStates[i];
				auto& n = availableGameStates[i + 1];
				if (a.gameState && n.gameState &&
					a.gameState->when == n.gameState->when)
				{
					if (n.startWhat == TeaForGodEmperor::GameStateLevel::AtLeastHalfHealth)
					{
						// more eager to remove half-health
						availableGameStates.remove_at(i + 1);
					}
					else
					{
						availableGameStates.remove_at(i);
					}
					--i;
				}
			}
		}

#ifdef VERBOSE_GAME_STATES
		{
			output(TXT("available game states:"));
			for_every(ags, availableGameStates)
			{
				if (ags->startWhat.is_set())
				{
					output(TXT(" + %S"), TeaForGodEmperor::GameStateLevel::to_char((TeaForGodEmperor::GameStateLevel::Type)ags->startWhat.get()));
				}
				else if (ags->newMission)
				{
					output(TXT(" + NEW MISSION"));
				}
				else if (ags->abortMission)
				{
					output(TXT(" + ABORT MISSION"));
				}
				else if (ags->switchGameSlotMode.is_set())
				{
					output(TXT(" + SWITCH GAME SLOT MODE"));
				}
				else
				{
					output(TXT(" + RESTART AT ANY CHAPTER"));
				}
			}
		}
#endif

		if (screen)
		{
			screen->deactivate();
		}

		if (showMainPanel == MP_ChooseStart ||
			showMainPanel == MP_ChooseMission)
		{
			if (showMainPanel == MP_ChooseStart)
			{
				change_pilgrimage_restart(0); // before we added the screen - to get the number of available pilgrimages
			}
			if (availableGameStates.is_empty() &&
				(availablePilgrimages <= 1 || TeaForGodEmperor::GameSettings::get().difficulty.restartMode > TeaForGodEmperor::GameSettings::RestartMode::UnlockedChapter))
			{
				start_pilgrimage(); // start mission
			}
			else
			{
				Vector2 fs = HubScreen::s_fontSizeInPixels;
				fs.y = fs.x = max(fs.x, fs.y);

				float elementSpacing = fs.y;
				int lineCount = 4;
				float inElementLineDistance = fs.y * 1.5f;
				float inElementMargin = fs.y;

				float elementHeight = fs.y * (float)lineCount + (inElementLineDistance - fs.y) * (float)(lineCount - 1) + inElementMargin * 2.0f;
				Vector2 ppa(menuPPA, menuPPA);

				Vector2 size(whereToStartWidth, 0.0f);
				size.y = (fs.y * 2.0f + elementHeight * (float)(availableGameStates.get_size()) + elementSpacing * (float)(availableGameStates.get_size() - 1)) / ppa.y;

				Rotator3 atDir = mainDir.get();
				//atDir.yaw += mainWidth * 0.5f + spacing + /*menuSideWidth + spacing + */size.x * 0.5f;
				atDir.pitch = mainPitch;

				if (size.y > mainHeight)
				{
					// if wouldn't fit, make more space
					float excess = size.y - mainHeight;
					atDir.pitch -= excess * 0.5f;
				}

				screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMenuWhereToStart), size, atDir, radius, true, NP, ppa);
				screen->blockTriggerHold = true;

				Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

				float listBottomAt = atDir.pitch - size.y * 0.5f;

				{
					selectedGameStateElement.clear();
					auto* list = new HubWidgets::List(NAME(idGameStateList), availableSpace);
					list->scroll.scrollType = Loader::HubWidgets::InnerScroll::NoScroll;
					list->elementSize = Vector2(list->at.x.length(), elementHeight);
					list->elementSpacing = Vector2(elementSpacing, elementSpacing);
					list->on_select = [this](HubDraggable const* _element, bool _clicked)
					{
						selectedGameStateElement = _element;
						if (auto* gsol = fast_cast<GameStateOnList>(_element->data.get()))
						{
							startWhat = gsol->startWhat;
							if (gsol->newMission)
							{
								if (_clicked)
								{
									start_pilgrimage();
									return;
								}
							}
							if (gsol->abortMission)
							{
								if (_clicked)
								{
									abort_mission_question();
									return;
								}
							}
							if (gsol->switchGameSlotMode.is_set())
							{
								if (_clicked)
								{
									switch_game_slot_mode_question(gsol->switchGameSlotMode.get());
									return;
								}
							}
						}
						if (_clicked)
						{
							start_pilgrimage(startWhat);
						}
					};
					list->drawElementBorder = false;
					list->draw_element = [inElementMargin, list](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
					{
						GameStateOnList::draw_element(inElementMargin, list, _display, _at, _element);
					};

					HubDraggable* selectedDraggable = nullptr;
					{
						for_every(gs, availableGameStates)
						{
							auto* gsol = gs->create_game_slot_on_list();
							auto* addedDraggable = list->add(gsol);
							if (!selectedDraggable)
							{
								selectedDraggable = addedDraggable;
							}

						}
					}

					screen->add_widget(list);

					if (selectedDraggable)
					{
						get_hub()->select(list, selectedDraggable);
					}

					if (showMainPanel == MP_ChooseStart)
					{
						int trianglesAtIdx = NONE;
						for_every_ref(el, list->elements)
						{
							if (auto* gsol = fast_cast<GameStateOnList>(el->data.get()))
							{
								if (gsol->startAllOver)
								{
									trianglesAtIdx = for_everys_index(el);
								}
							}
						}
						int trianglesAtIdxFromBottom = list->elements.get_size() - 1 - trianglesAtIdx;
						listBottomAt += ((elementSpacing + elementHeight) * (float)trianglesAtIdxFromBottom) / ppa.y;
					}
				}

				screen->activate();
				get_hub()->add_screen(screen);

				bottomAt = min(bottomAt, atDir.pitch - size.y * 0.5f);

				// for MP_ChooseStart we have triangles to switch starting pilgrimage, for MP_ChooseMission we have only "abort"
				if (showMainPanel == MP_ChooseStart)
				{
					if (availablePilgrimages > 1)
					{
						for_count(int, sideTriangle, 2)
						{
							float side = sideTriangle == 0 ? -1.0f : 1.0f;
							Name id = sideTriangle == 0 ? NAME(pilgrimageSetupMenuWhereToStart_Left) : NAME(pilgrimageSetupMenuWhereToStart_Right);

							Vector2 size((fs.x * 2.0f + elementHeight) / ppa.y, (fs.y * 2.0f + elementHeight) / ppa.y);
							Rotator3 atDir = mainDir.get();
							atDir.yaw += side * (whereToStartWidth * 0.5f + spacing + /*menuSideWidth + spacing + */size.x * 0.5f);
							atDir.pitch = listBottomAt + size.y * 0.5f;

							auto* screen = new HubScreen(get_hub(), id, size, atDir, radius, true, NP, ppa);
							screen->blockTriggerHold = true;

							Range2 at = screen->mainResolutionInPixels;
							at.expand_by(-fs);
							{
								auto* w = new HubWidgets::Button(id, at, String::empty());
								w->custom_draw = [this, w, sideTriangle, side](Framework::Display* _display, Range2 const& _at, HubWidgetUsedColours const& _useColours)
								{
									// add triangles at the end to allow up/down
									Range2 triangleAt = _at;
									float triangleHeight = round((_at.y.length() * 0.9f) / 2.0f);
									triangleAt.y.max = round(_at.y.centre() + triangleHeight * 0.5f);
									triangleAt.y.min = triangleAt.y.max - triangleHeight;
									triangleAt.x.max = round(_at.x.centre() + triangleHeight * 0.5f);
									triangleAt.x.min = triangleAt.x.max - triangleHeight;

									Vector2 a, b, c;
									if (sideTriangle == 1)
									{
										a = Vector2(triangleAt.x.min, triangleAt.y.max);
										b = Vector2(triangleAt.x.max, triangleAt.y.centre());
										c = Vector2(triangleAt.x.min, triangleAt.y.min);
									}
									else
									{
										c = Vector2(triangleAt.x.max, triangleAt.y.max);
										b = Vector2(triangleAt.x.min, triangleAt.y.centre());
										a = Vector2(triangleAt.x.max, triangleAt.y.min);
									}
									float appearActive = w->active;
									float mayBeActive = 1.0f;
									if (sideTriangle == 0)
									{
										if (chosenPilgrimageRestart <= 0)
										{
											appearActive = 0.0f;
											mayBeActive = 0.0f;
										}
									}
									else
									{
										if (chosenPilgrimageRestart >= availablePilgrimages - 1)
										{
											appearActive = 0.0f;
											mayBeActive = 0.0f;
										}
									}
									if (mayBeActive > 0.0f)
									{
										::System::Video3DPrimitives::triangle_2d(Colour::lerp(appearActive, HubColours::text(), HubColours::selected_highlighted()), a, b, c, false);
									}
									{
										Vector2 offsets[] = { Vector2(0.0f,  0.0f)
															, Vector2(0.0f,  1.0f)
															, Vector2(0.0f, -1.0f)
															, Vector2(side,  0.0f)
										};
										for_count(int, o, 4)
										{
											Vector2 offset = offsets[o];
											Vector2 p[] = { a + offset, b + offset, c + offset, a + offset };
											::System::Video3DPrimitives::line_strip_2d(
												Colour::lerp(appearActive, Colour::lerp(0.5f + 0.5f * mayBeActive, HubColours::widget_background(), HubColours::text()), HubColours::selected_highlighted())
												, p, 4, false);
										}
									}
								};

								screen->add_widget(w);

								w->on_click = [this, sideTriangle](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { change_pilgrimage_restart(sideTriangle == 0 ? -1 : 1); };
							}

							screen->activate();
							get_hub()->add_screen(screen);
						}
					}
					change_pilgrimage_restart(0); // and after we added the screen - go fill the names
				}
			}
		}
		
		if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuWhereToTestStart)))
		{
			screen->deactivate();
		}

		if (showMainPanel == MP_ChooseTestStart)
		{
			Vector2 fs = HubScreen::s_fontSizeInPixels;
			fs.y = fs.x = max(fs.x, fs.y);

			Vector2 size(whereToStartWidth, 0.0f);
			size.y = mainHeight;
		
			Rotator3 atDir = mainDir.get();
			atDir.pitch = mainPitch;

			Vector2 ppa = Vector2::one * 16.0f;

			screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMenuWhereToTestStart), size, atDir, radius, true, NP, ppa);

			Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

			float elementHeight = fs.y * 1.5f;
			float elementSpacing = fs.y * 0.25f;

			{
				selectedGameStateElement.clear();
				auto* list = new HubWidgets::List(NAME(idTestScenarioList), availableSpace);
				list->scroll.scrollType = Loader::HubWidgets::InnerScroll::VerticalScroll;
				list->elementSize = Vector2(list->at.x.length(), elementHeight);
				list->elementSpacing = Vector2(elementSpacing, elementSpacing);
				list->on_select = [this](HubDraggable const* _element, bool _clicked)
				{
					selectedGameStateElement = _element;
					if (auto* tsol = fast_cast<TestScenarioOnList>(_element->data.get()))
					{
						startTestScenario = tsol->scenario;
					}
					if (_clicked)
					{
						start_test_scenario(startTestScenario);
					}
				};
				list->drawElementBorder = false;
				list->draw_element = [list, fs](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
				{
					list->draw_element_border(_display, _at, _element, false/*selectedGameStateElement == _element*/);
					if (auto* tsol = fast_cast<TestScenarioOnList>(_element->data.get()))
					{
						if (auto* font = _display->get_font())
						{
							Vector2 at(_at.x.min + fs.x, _at.y.centre());
							Colour textColour = HubColours::text();// selectedGameStateElement == _element ? HubColours::selected_highlighted() : HubColours::text();

							font->draw_text_at(::System::Video3D::get(), tsol->info.to_char(), textColour, at, Vector2::one, Vector2(0.0f, 0.5f));
						}
					}
				};

				Array<Framework::LibraryBasedParameters*> availableScenarios;
				Framework::Library::get_current()->get_library_based_parameters().do_for_every([&availableScenarios](Framework::LibraryStored* _libraryStored)
					{
						if (_libraryStored->get_name().get_group() == TXT("game test scenarios"))
						{
							if (auto* lbd = fast_cast<Framework::LibraryBasedParameters>(_libraryStored))
							{
								availableScenarios.push_back(lbd);
							}
						}
					});
				struct SortAvailableScenarios
				{
					static int compare(void const* _a, void const* _b)
					{
						Framework::LibraryBasedParameters* a = *plain_cast<Framework::LibraryBasedParameters*>(_a);
						Framework::LibraryBasedParameters* b = *plain_cast<Framework::LibraryBasedParameters*>(_b);
						tchar const * an = a->get_name().get_name().to_char();
						tchar const * bn = b->get_name().get_name().to_char();
						int result = String::compare_tchar_icase_sort(an, bn);
						return result;
					}
				};
				sort(availableScenarios, SortAvailableScenarios::compare);

				HubDraggable* selectedDraggable = nullptr;
				for_every_ptr(lbp, availableScenarios)
				{
					auto* tsol = new TestScenarioOnList();
					tsol->scenario = lbp;
					DEFINE_STATIC_NAME(gameTestScenarioName);
					tsol->info = lbp->get_parameters().get_value<Name>(NAME(gameTestScenarioName), Name::invalid()).to_string();
					auto* addedDraggable = list->add(tsol);
					if (!selectedDraggable)
					{
						selectedDraggable = addedDraggable;
					}
				}

				screen->add_widget(list);

				if (selectedDraggable)
				{
					get_hub()->select(list, selectedDraggable);
				}

				screen->activate();
				get_hub()->add_screen(screen);
			}
		}
	}

	{
		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuBottom));

		if (!isMainMenu)
		{
			if (screen)
			{
				screen->deactivate();
			}
		}
		else if (!screen)
		{
			Vector2 size(mainWidth, bottomHeight);
			Vector2 ppa(menuPPA, menuPPA);
			Rotator3 atDir = mainDir.get();
			//atDir.yaw += 30.0f;
			atDir.pitch = bottomAt - spacing - size.y * 0.5f;

			screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMenuBottom), size, atDir, radius, true, NP, ppa);
			screen->blockTriggerHold = true;

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			fs.y = fs.x = max(fs.x, fs.y);			
			Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

			Array<HubScreenButtonInfo> buttons;

			{
				buttons.clear();
				buttons.push_back(HubScreenButtonInfo(NAME(idCalibrate), NAME(lsCalibrate), [this]() { get_hub()->set_scene(new HubScenes::Calibrate()); }));
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOptions), [this]() { get_hub()->set_scene(new HubScenes::Options(HubScenes::Options::Main)); }));
#ifdef HAND_GESTURES_IN_MAIN_MENU
				if (auto* vr = VR::IVR::get())
				{
					if (vr->is_using_hand_tracking())
					{
						buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsShowHandGestures), [this]() { HubScreens::HandGestures::show(get_hub()); }));
					}
				}
#endif
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsQuitGame),
					[this]()
					{
						HubScreens::Question::ask(get_hub(), NAME(lsQuitGameQuestion),
							[this]()
							{
								deactivate_screens();

								if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
								{
									game->save_player_profile();
								}

								get_hub()->quit_game();
							},
							[]()
							{
								// we'll just get back
							}
							);
					}));
				screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1),
					availableSpace, fs);
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}

	{
		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuBottom_Back));

		if (isMainMenu)
		{
			if (screen)
			{
				screen->deactivate();
			}
		}
		else if (!screen)
		{
			Vector2 size(mainWidth, bottomHeight);
			Vector2 ppa(menuPPA, menuPPA);
			Rotator3 atDir = mainDir.get();
			//atDir.yaw += 30.0f;
			atDir.pitch = bottomAt - spacing - size.y * 0.5f;

			screen = new HubScreen(get_hub(), NAME(pilgrimageSetupMenuBottom_Back), size, atDir, radius, true, NP, ppa);
			screen->blockTriggerHold = true;

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			fs.y = fs.x = max(fs.x, fs.y);			
			Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

			Array<HubScreenButtonInfo> buttons;

			{
				buttons.clear();
				buttons.push_back(HubScreenButtonInfo(NAME(idBack), NAME(lsBack), [this]() { show_main_panel(MP_MainMenu); }).with_scale(2.0f));
				screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1),
					availableSpace, fs);
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}

	if (showMainPanel == MP_MainMenu)
	{
		if (s_reachedEnd)
		{
			get_hub()->reset_highlights();
			get_hub()->add_highlight_blink(NAME(pilgrimageSetupProfileGameSlot), NAME(idProfileGameSlot));
			get_hub()->add_highlight_infinite(NAME(pilgrimageSetupProfileGameSlot), NAME(idProfileGameSlot));
		}
		else
		{
			get_hub()->reset_highlights();
			get_hub()->add_highlight_blink(NAME(pilgrimageSetupMainPanel_Main), NAME(idStart));
			get_hub()->add_highlight_pause();
			get_hub()->add_highlight_blink(NAME(pilgrimageSetupProfileGameSlot), NAME(idProfileGameSlot));
			get_hub()->add_highlight_pause();
			get_hub()->add_highlight_infinite(NAME(pilgrimageSetupMainPanel_Main), NAME(idStart));
		}
	}
}

void PilgrimageSetup::change_pilgrimage_restart(int _by)
{
	Array<TeaForGodEmperor::Pilgrimage*> pilgrimages;
	bool noPilgrimages = false;

	String providePilgrimageName = String::empty();

	chosenPilgrimageRestart = 0;
	availablePilgrimages = 0;

	auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
	if (auto* gs = ps.access_active_game_slot())
	{
		if (auto* gd = gs->gameDefinition.get())
		{
			if (!gd->get_pilgrimages().is_empty())
			{
				// get them in the order they are defined in the game definition, no matter how we unlocked them
				for_every_ref(p, gd->get_pilgrimages())
				{
					if (gs->unlockedPilgrimages.does_contain(p->get_name())
						|| allPilgrimagesUnlockedCheat
#ifdef BUILD_PREVIEW
#ifdef ALLOW_ALL_CHAPTERS_FOR_PREVIEW_BUILDS
						|| true // all unlocked
#endif				
#endif
						)
					{
						pilgrimages.push_back(p);
					}
				}

				// default, first one
				if (pilgrimages.is_empty())
				{
					noPilgrimages = true;
					pilgrimages.push_back(gd->get_pilgrimages().get_first().get());
				}
			}
			if (!gd->get_conditional_pilgrimages().is_empty())
			{
				// get them in the order they are defined in the game definition, no matter how we unlocked them
				for_every(cp, gd->get_conditional_pilgrimages())
				{
					if (auto* p = cp->pilgrimage.get())
					{
						if (gs->unlockedPilgrimages.does_contain(p->get_name())
							|| allPilgrimagesUnlockedCheat
#ifdef BUILD_PREVIEW
#ifdef ALLOW_ALL_CHAPTERS_FOR_PREVIEW_BUILDS
							|| true // all unlocked
#endif				
#endif
							)
						{
							pilgrimages.push_back(p);
						}
					}
				}
			}
		}

		if (pilgrimages.is_empty())
		{
			error(TXT("no pilgrimages available"));
		}
		if (!pilgrimages.is_empty())
		{
			int atIdx = 0;
			for_every_ptr(p, pilgrimages)
			{
				if (p->get_name() == gs->restartAtPilgrimage)
				{
					atIdx = for_everys_index(p);
					break;
				}
			}

			atIdx += _by;
			atIdx = clamp(atIdx, 0, pilgrimages.get_size() - 1);

			auto* chosenPilgrimage = pilgrimages[atIdx];

#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("change pilgrimage restart, restartAtPilgrimage set to \"%S\""), chosenPilgrimage->get_name().to_string().to_char());
#endif

			gs->restartAtPilgrimage = chosenPilgrimage->get_name();

			if (noPilgrimages)
			{
				providePilgrimageName = String::empty();
			}
			else
			{
				String connector = chosenPilgrimage->get_pilgrimage_name_pre_connector_ls().get().is_empty() ? String::space() : String::space() + chosenPilgrimage->get_pilgrimage_name_pre_connector_ls().get() + String::space();
				providePilgrimageName = (chosenPilgrimage->get_pilgrimage_name_pre_short_ls().get() + connector + chosenPilgrimage->get_pilgrimage_name_ls().get()).trim();
			}

			chosenPilgrimageRestart = atIdx;
			availablePilgrimages = pilgrimages.get_size();
		}
	}

	if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuWhereToStart)))
	{
		if (auto* list = fast_cast<HubWidgets::List>(screen->get_widget(NAME(idGameStateList))))
		{
			for_every_ref(el, list->elements)
			{
				if (auto* gsol = fast_cast<GameStateOnList>(el->data.get()))
				{
					if (gsol->startAllOver)
					{
						an_assert(gsol->lines.get_size() >= 2);
						gsol->lines[1] = providePilgrimageName;
					}
				}
			}
		}
		screen->force_redraw();
	}

	if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuWhereToStart_Left)))
	{
		screen->force_redraw();
	}
	if (HubScreen* screen = get_hub()->get_screen(NAME(pilgrimageSetupMenuWhereToStart_Right)))
	{
		screen->force_redraw();
	}
}

void PilgrimageSetup::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

	auto* vr = VR::IVR::get();

	if (vr)
	{
		if (vr->get_controls().requestRecenter)
		{
			// as we need to store this dir but we also got updated with recenter request
			mainDir = get_hub()->get_background_dir_or_main_forward();
		}
	}

	if (!showRenameMessagePending.is_empty())
	{
		HubScreens::Message::show(get_hub(),
			Framework::StringFormatter::format_sentence_loc_str(NAME(lsChangedProfileName), Framework::StringFormatterParams()
				.add(NAME(profileName), showRenameMessagePending)),
			[this]()
			{
				show_main_panel(MP_MainMenu);
			});
		showRenameMessagePending = String::empty();
	}

#ifdef AN_STANDARD_INPUT
	if (System::Input::get()->has_key_been_pressed(System::Key::Space))
	{
		deactivateHub = true;
	}
#endif
	
	ArrayStatic<Name, 10> menuNames; SET_EXTRA_DEBUG_INFO(menuNames, TXT("PilgrimageSetup::on_update.menuNames"));
	menuNames.push_back(NAME(pilgrimageSetupMenuBottom));
	menuNames.push_back(NAME(pilgrimageSetupMenuRight));
	menuNames.push_back(NAME(pilgrimageSetupMenuLeft));
	ArrayStatic<Name, 10> selectionHighlight; SET_EXTRA_DEBUG_INFO(selectionHighlight, TXT("PilgrimageSetup::on_update.selectionHighlight"));
	selectionHighlight.set_size(MP_NUM);
	selectionHighlight[MP_GameModifiers] = NAME(lsModifiers);
	selectionHighlight[MP_Unlocks] = NAME(lsUnlocks);
	selectionHighlight[MP_Stats] = NAME(lsStats);

	for_every(menuName, menuNames)
	{
		if (auto* screen = get_hub()->get_screen(*menuName))
		{
			if (auto* w = fast_cast<HubWidgets::Button>(screen->get_widget(NAME(idCalibrate))))
			{
				float doorHeight = TeaForGodEmperor::PlayerPreferences::get_door_height_from_eye_level();

				w->set_highlighted(abs(TeaForGodEmperor::PlayerSetup::get_current().get_preferences().doorHeight - doorHeight) >= HubScenes::Calibrate::max_distance);
			}
			for_every(sh, selectionHighlight)
			{
				if (auto* w = fast_cast<HubWidgets::Button>(screen->get_widget(*sh)))
				{
					if (showMainPanel == for_everys_index(sh))
					{
						w->set_colour(HubColours::selected_highlighted());
					}
					else
					{
						w->set_colour(NP);
					}
				}
			}
		}
	}

	if (auto* screen = get_hub()->get_screen(NAME(pilgrimageSetupRealTime)))
	{
		time_t currentTime = time(0);
		tm currentTimeInfo;
		tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
		pcurrentTimeInfo = localtime(&currentTime);
#else
		localtime_s(&currentTimeInfo, &currentTime);
#endif
		int newRealTimeSeconds = (pcurrentTimeInfo->tm_hour * 60 + pcurrentTimeInfo->tm_min) * 60;

		if (realTimeSeconds != newRealTimeSeconds)
		{
			realTimeSeconds = newRealTimeSeconds;
			if (auto* w = fast_cast<HubWidgets::Text>(screen->get_widget(NAME(idRealTime))))
			{
				w->set(Framework::Time::from_seconds(realTimeSeconds).get_full_hour_minute_string());
			}
		}
	}

	if (changePilgrimageRestart != 0)
	{
		change_pilgrimage_restart(changePilgrimageRestart);
		changePilgrimageRestart = 0;
	}
}

void PilgrimageSetup::store_run_setup()
{
	runSetup.update_using_this();
	//
	auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
	if (auto* gs = ps.access_active_game_slot())
	{
		gs->runSetup = runSetup;
	}
}

void PilgrimageSetup::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);

	// if we come back here, we should update the direction
	mainDir.clear();
}

void PilgrimageSetup::on_loader_deactivate()
{
	base::on_loader_deactivate();
}

#define HIGHLIGHT_DIFFICULTY(diffName, diffWidget) \
	if (auto * w = fast_cast<HubWidgets::Button>(diffWidget.get())) \
	{ \
		w->set_highlighted(choseDifficulty == diffName); \
	}

void PilgrimageSetup::change_profile_game_slot()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		deactivate_screens();

		game->add_async_world_job_top_priority(TXT("exit to player profile selection"),
		[this, game]()
		{
			store_run_setup();
			game->save_player_profile();
#ifndef SINGLE_GAME_SLOT
			game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectGameSlot);
#else
			game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPlayerProfile);
#endif

			deactivateHub = true;
		});
	}
}

void PilgrimageSetup::abort_mission_question()
{
	HubScreens::Question::ask(get_hub(), NAME(lsAbortMissionQuestion),
		[this]()
		{
			abort_mission();
		},
		[]()
		{
			// we'll just get back
		}
		);
}

void PilgrimageSetup::abort_mission()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		deactivate_screens();
		game->add_async_world_job_top_priority(TXT("abort mission"),
			[this, game]()
			{
				TeaForGodEmperor::MissionState::async_abort();

				game->add_immediate_sync_world_job(TXT("show main menu"),
					[this]()
					{
						show_main_panel(MP_MainMenu);
					});
			});
	}
}

void PilgrimageSetup::switch_game_slot_mode_question(TeaForGodEmperor::GameSlotMode::Type _gameSlotMode)
{
	if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
	{
		if (!gs->has_any_game_state())
		{
			switch_game_slot_mode_unlock_question(_gameSlotMode);
			return;
		}
	}
	HubScreens::Question::ask(get_hub(), NAME(lsSwitchModeQuestion),
		[this, _gameSlotMode]()
		{
			switch_game_slot_mode_unlock_question(_gameSlotMode);
		},
		[]()
		{
			// we'll just get back
		}
		);
}

void PilgrimageSetup::switch_game_slot_mode_unlock_question(TeaForGodEmperor::GameSlotMode::Type _gameSlotMode)
{
	if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
	{
		if (!gs->is_game_slot_mode_available(_gameSlotMode))
		{
			Name lsQuestion;
			if (_gameSlotMode == TeaForGodEmperor::GameSlotMode::Missions)
			{
				lsQuestion = NAME(lsEarlyUnlockMissionsQuestion);
			}
			if (lsQuestion.is_valid())
			{
				HubScreens::Question::ask(get_hub(), lsQuestion,
					[this, _gameSlotMode]()
					{
						switch_game_slot_mode(_gameSlotMode);
					},
					[]()
					{
						// we'll just get back
					}
					);
				return;
			}
		}
	}
	switch_game_slot_mode(_gameSlotMode);
}

void PilgrimageSetup::switch_game_slot_mode(TeaForGodEmperor::GameSlotMode::Type _gameSlotMode)
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		deactivate_screens();
		game->add_async_world_job_top_priority(TXT("switch game slot mode"),
			[this, game, _gameSlotMode]()
			{
				if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
				{
					gs->make_game_slot_mode_available(_gameSlotMode); // make sure it is available
					gs->set_game_slot_mode(_gameSlotMode);
				}

				{
					game->save_config_file();
					game->save_player_profile(true); // save active game slot, to store mission state and game states
				}

				// do the long way, restart the whole thing
				deactivate_screens();
				deactivateHub = true;
			});
	}
}

void PilgrimageSetup::start_pilgrimage(Optional<TeaForGodEmperor::GameStateLevel::Type> const & _what)
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->use_test_scenario_for_pilgimage(nullptr);
		if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
		{
			Name question = Name::invalid();
			gs->startUsingGameState.clear();
			if (_what.is_set())
			{
				if (_what.get() == TeaForGodEmperor::GameStateLevel::LastMoment)
				{
					gs->startUsingGameState = gs->lastMoment;
				}
				else if (_what.get() == TeaForGodEmperor::GameStateLevel::Checkpoint)
				{
					gs->startUsingGameState = gs->checkpoint;
				}
				else if (_what.get() == TeaForGodEmperor::GameStateLevel::AtLeastHalfHealth)
				{
					gs->startUsingGameState = gs->atLeastHalfHealth;
				}
				else if (_what.get() == TeaForGodEmperor::GameStateLevel::ChapterStart)
				{
					gs->startUsingGameState = gs->chapterStart;
				}
			}

			if (question.is_valid())
			{
				HubScreens::Question::ask(get_hub(), question,
					[this]()
					{
						start_pilgrimage_now();
					},
					[]()
					{
						// we'll just get back
					}
					);
			}
			else
			{
				start_pilgrimage_now();
			}
		}
		else
		{
			start_pilgrimage_now();
		}
	}
}

void PilgrimageSetup::start_test_scenario(Framework::LibraryBasedParameters* _startTestScenario)
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->use_test_scenario_for_pilgimage(_startTestScenario);
		if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
		{
			gs->startUsingGameState.clear();
		}
		start_pilgrimage_now();
	}
}

void PilgrimageSetup::start_pilgrimage_now()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		deactivate_screens();

		game->add_async_world_job_top_priority(TXT("start pilgrimage"),
			[this, game]()
		{
			store_run_setup();
			game->save_player_profile();
			if (game->get_test_scenario_for_pilgimage())
			{
				game->set_meta_state(TeaForGodEmperor::GameMetaState::UseTestConfig);
			}
			else
			{
				game->set_meta_state(TeaForGodEmperor::GameMetaState::Pilgrimage); // both from game state or not
			}
			deactivateHub = true;
		});
	}
}

void PilgrimageSetup::manage_profile()
{
	HubScreen* screen = get_hub()->get_screen(NAME(manageProfile));

	if (!screen)
	{
		Vector2 size(50.0f, 30.0f);
		Vector2 ppa(20.0f, 20.0f);
		float radius = get_hub()->get_radius() * 0.55f;
		Rotator3 atDir = mainDir.get();
		Rotator3 verticalOffset(0.0f, 0.0f, 0.0f);

		screen = new HubScreen(get_hub(), NAME(manageProfile), size, atDir, radius, true, verticalOffset, ppa);
		screen->be_modal();
		get_hub()->add_screen(screen);

		Vector2 fs = HubScreen::s_fontSizeInPixels;
		fs.y = fs.x = max(fs.x, fs.y);
		Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-fs);

		Range2 profileSpace = availableSpace;
		profileSpace.y.min = profileSpace.y.max - fs.y * 5.0f;
		availableSpace.y.max = profileSpace.y.min - fs.y;

		{
			auto* w = new HubWidgets::Text(Name::invalid(), profileSpace, get_profile_name());
			w->scale = Vector2::one * 2.0f;
			w->alignPt = Vector2(0.5f, 1.0f);
			screen->add_widget(w);
		}
		{
			auto* w = new HubWidgets::Text(Name::invalid(), profileSpace, get_game_slot_info());
			w->alignPt = Vector2(0.5f, 0.0f);
			screen->add_widget(w);
		}

		Array<HubScreenButtonInfo> buttons;
		buttons.push_back(HubScreenButtonInfo(NAME(idChangeGameSlotProfile), NAME(lsChangeGameSlotProfile), [this]() { change_profile_game_slot(); }));
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsRenameProfile), [this]() { rename_profile(); }));
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsRemoveGameSlot), [this]() { remove_game_slot(); }));
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsRemoveProfile), [this]() { remove_profile(); }));
		int horCount = 2;

		while ((buttons.get_size() % horCount) != horCount - 1)
		{
			buttons.push_back(HubScreenButtonInfo()); // empty to have back on right
		}
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this, screen]() { screen->deactivate(); show_main_panel(NP);  }));

		screen->add_button_widgets_grid(buttons, VectorInt2(horCount, (buttons.get_size() + (horCount - 1)) / horCount),
			availableSpace,
			fs);

		if (s_reachedEnd)
		{
			get_hub()->reset_highlights();
			get_hub()->add_highlight_blink(NAME(manageProfile), NAME(idChangeGameSlotProfile));
			get_hub()->add_highlight_infinite(NAME(manageProfile), NAME(idChangeGameSlotProfile));
		}
		else
		{
			get_hub()->reset_highlights();
			get_hub()->add_highlight_blink(NAME(manageProfile), NAME(idChangeGameSlotProfile));
			get_hub()->add_highlight_pause();
		}
	}
}

String PilgrimageSetup::get_game_slot_info() const
{
	auto& ps = TeaForGodEmperor::PlayerSetup::get_current();
	if (auto* gs = ps.get_active_game_slot())
	{
		return Framework::StringFormatter::format_sentence_loc_str(NAME(lsGameSlotInfo), Framework::StringFormatterParams()
			.add(NAME(gameSlotID), gs->id)
			.add(NAME(gameType), gs->gameDefinition.get()? gs->gameDefinition->get_name_for_ui().get() : String::empty())
			.add(NAME(gameSubType), ! gs->gameSubDefinitions.is_empty() && gs->gameSubDefinitions[0].get()? gs->gameSubDefinitions[0]->get_name_for_ui().get() : String::empty())
		);
	}
	else
	{
		return String::empty();
	}
}

String PilgrimageSetup::get_profile_name() const
{
	auto& ps = TeaForGodEmperor::PlayerSetup::get_current();
	if (auto* gs = ps.get_active_game_slot())
	{
		return ps.get_profile_name();
	}
	else
	{
		return String::empty();
	}
}

void PilgrimageSetup::deactivate_screens()
{
	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));

	get_hub()->keep_only_screens(screensToKeep);
}

void PilgrimageSetup::rename_profile(Optional<String> const& _profileName)
{
	String profileName;
	String previousName;
	if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
	{
		previousName = c->get_player_profile_name();
	}
	if (_profileName.is_set())
	{
		profileName = _profileName.get();
	}
	else
	{
		profileName = previousName;
	}
	HubScreens::EnterText::show(get_hub(), LOC_STR(NAME(lsRenameProfile)), profileName,
		[this, previousName](String const& _text)
		{
			if (_text.is_empty())
			{
				HubScreens::Message::show(get_hub(), NAME(lsProvideNonEmptyProfileName), [] {});
				rename_profile();
			}
			else if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				if (game->get_player_profiles().does_contain(_text) && _text != previousName)
				{
					HubScreens::Message::show(get_hub(), NAME(lsAlreadyExistsSuchProfile), [] {});
					rename_profile();
				}
				else
				{
					deactivate_screens();
					game->add_async_world_job_top_priority(TXT("rename profile"),
					[game, this, previousName, _text]
					{
						game->rename_player_profile(previousName, _text);

						show_rename_message(_text);
					});
				}
			}
		},
		nullptr,
		false, true);
}

void PilgrimageSetup::remove_profile()
{
	HubScreens::Question::ask(get_hub(), NAME(lsRemoveProfileQuestion),
		[this]()
		{
			HubScreens::Question::ask(get_hub(), NAME(lsRemoveProfileQuestionSecond),
				[this]()
				{
					if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						deactivate_screens();
						game->add_async_world_job_top_priority(TXT("remove profile"),
						[this, game]()
						{
							game->remove_player_profile();
							game->supress_saving_config_file(false);
							game->save_config_file();
							game->save_player_profile();
							game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPlayerProfile);
							deactivateHub = true;
						});
					}
				},
				nullptr);
		},
		nullptr);
}

void PilgrimageSetup::remove_game_slot()
{
	HubScreens::Question::ask(get_hub(), NAME(lsRemoveGameSlotQuestion),
		[this]()
		{
			HubScreens::Question::ask(get_hub(), NAME(lsRemoveGameSlotQuestionSecond),
				[this]()
				{
					if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						deactivate_screens();
						game->add_async_world_job_top_priority(TXT("remove game slot"),
						[this, game]()
						{
							TeaForGodEmperor::PlayerSetup::access_current().remove_active_game_slot();
							game->supress_saving_config_file(false);
							game->save_config_file();
							game->save_player_profile();
							game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectGameSlot);
							deactivateHub = true;
						});
					}
				},
				nullptr);
		},
		nullptr);
}
