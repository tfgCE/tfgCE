#include "lhs_summary.h"

#include "lhs_pilgrimageSetup.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_list.h"
#include "..\widgets\lhw_rect.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameLog.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\game\gameStats.h"
#include "..\..\..\game\persistence.h"
#include "..\..\..\roomGenerators\roomGenerationInfo.h"

#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\time\time.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

REMOVE_AS_SOON_AS_POSSIBLE_
#define AN_DEBUG_SUMMARY

//

// screens
DEFINE_STATIC_NAME(summary);
DEFINE_STATIC_NAME(summary_gameModifiers);
DEFINE_STATIC_NAME(summary_gameSlots);

// execution tags
DEFINE_STATIC_NAME(showcase);

// localised strings
DEFINE_STATIC_NAME_STR(lsDied, TXT("hub; summary; died"));
DEFINE_STATIC_NAME_STR(lsRunOutOfTime, TXT("hub; summary; run out of time"));
DEFINE_STATIC_NAME_STR(lsInterrupted, TXT("hub; summary; interrupted"));
DEFINE_STATIC_NAME_STR(lsReachedEnd, TXT("hub; summary; reached end"));
DEFINE_STATIC_NAME_STR(lsReachedDemoEnd, TXT("hub; summary; reached demo end"));
DEFINE_STATIC_NAME_STR(lsSummary, TXT("hub; summary; summary"));
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
DEFINE_STATIC_NAME_STR(lsPleaseWait, TXT("hub; common; please wait"));
DEFINE_STATIC_NAME_STR(lsContinue, TXT("hub; summary; continue"));
DEFINE_STATIC_NAME_STR(lsExit, TXT("hub; common; exit"));
DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));
DEFINE_STATIC_NAME_STR(lsSummaryGameModifiersInfo, TXT("hub; summary; game modifiers; info"));
DEFINE_STATIC_NAME_STR(lsSummaryGameModifiersOpen, TXT("hub; summary; game modifiers; open"));
DEFINE_STATIC_NAME_STR(lsSummaryGameSlotsInfo, TXT("hub; summary; game slots; info"));
DEFINE_STATIC_NAME_STR(lsSummaryGameSlotsOpen, TXT("hub; summary; game slots; open"));

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(Summary);

Summary::Summary(TeaForGodEmperor::PostGameSummary::Type _summaryType)
:summaryType(_summaryType)
{
}

void Summary::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	deactivateHub = false;
	showContinue = false;

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsxy = max(fs.x, fs.y);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->keep_only_screen(NAME(summary));

	Rotator3 summaryAt = Rotator3::zero;
	float summaryWidth = 36.0f;
	float screenSpacing = 3.0f;
	float radius = get_hub()->get_radius() * 0.6f;
	float leftBottomAt = 0.0f;
	float rightBottomAt = 0.0f;

	{	// summary
		HubScreen* screen = get_hub()->get_screen(NAME(summary));

#ifdef AN_DEBUG_SUMMARY
		output(TXT("show summary"));
#endif

		if (!screen)
		{
			// we use prev stats to show how much did we gain when compared to the game save we started
			TeaForGodEmperor::GameStats const * prevStats = nullptr;
			TeaForGodEmperor::GameStats const * showStats = &TeaForGodEmperor::GameStats::get();
			if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
			{
				if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
				{
					if (gs->should_ignore_last_mission_result_for_summary())
					{
						showStats = nullptr;
					}
					else
					{
						if (auto* lmr = gs->lastMissionResult.get())
						{
							showStats = &lmr->get_game_stats();
						}
					}
				}
				else if (auto* g = gs->startedUsingGameState.get())
				{
					prevStats = &g->gameStats;
				}
			}

			screen = new HubScreen(get_hub(), NAME(summary), Vector2(summaryWidth, 120.0f), get_hub()->get_background_dir_or_main_forward(), radius, true);
			screen->activate();
			get_hub()->add_screen(screen);

			Range wholeWidth = screen->mainResolutionInPixels.x.expanded_by(-fsxy * 1.0f);

			float spacing = fsxy;
			float lineSize = fs.y * 1.2f;
			Vector2 at(screen->mainResolutionInPixels.x.centre(), screen->mainResolutionInPixels.y.max - spacing);

			{
				Vector2 wAt = at;

				auto* w = new HubWidgets::Text(Name::invalid(),
					Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
					(summaryType == TeaForGodEmperor::PostGameSummary::Interrupted ? NAME(lsInterrupted)
						: (summaryType == TeaForGodEmperor::PostGameSummary::Died ? (TeaForGodEmperor::GameSettings::get().difficulty.playerImmortal ? NAME(lsRunOutOfTime) : NAME(lsDied)) : (TeaForGodEmperor::is_demo()? NAME(lsReachedDemoEnd) : NAME(lsReachedEnd))))
				);
				screen->add_widget(w);
			}
			at.y -= lineSize;
			at.y -= spacing;

			// blank line
			at.y -= lineSize;

			{
				Vector2 wAt = at;

				auto* w = new HubWidgets::Text(Name::invalid(),
					Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
					NAME(lsSummary)
				);
				screen->add_widget(w);
			}
			at.y -= lineSize;
			at.y -= spacing;

			if (showStats)
			{
				// time
				{
					Vector2 wAt = at;

					auto* w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						NAME(lsTime));
					w->alignPt.x = 0.0f;
					screen->add_widget(w);

					String value = Framework::Time::from_seconds(showStats->timeInSeconds).get_string_short_time();
					if (prevStats)
					{
						if (showStats->timeInSeconds > prevStats->timeInSeconds)
						{
							value = String::printf(TXT("+%S (%S)"), Framework::Time::from_seconds(showStats->timeInSeconds - prevStats->timeInSeconds).get_string_short_time().to_char(), value.to_char());
						}
						else
						{
							value = String::printf(TXT("(%S)"), value.to_char());
						}
					}

					w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						value);
					w->alignPt.x = 1.0f;
					screen->add_widget(w);
				}
				at.y -= lineSize;
				at.y -= spacing;

				// distance
				{
					Vector2 wAt = at;

					auto* w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						NAME(lsDistance));
					w->alignPt.x = 0.0f;
					screen->add_widget(w);

					String value;
					MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
					if (ms == MeasurementSystem::Imperial)
					{
						value = String::printf(TXT("%.0fyd"), MeasurementSystem::to_yards(showStats->distance));
						if (prevStats)
						{
							if (showStats->distance > prevStats->distance)
							{
								value = String::printf(TXT("+%.0fyd (%S)"), MeasurementSystem::to_yards(showStats->distance - prevStats->distance), value.to_char());
							}
							else
							{
								value = String::printf(TXT("(%S)"), value.to_char());
							}
						}
					}
					else
					{
						value = String::printf(TXT("%.1fm"), showStats->distance);
						if (prevStats)
						{
							if (showStats->distance > prevStats->distance)
							{
								value = String::printf(TXT("+%.1fm (%S)"), (showStats->distance - prevStats->distance), value.to_char());
							}
							else
							{
								value = String::printf(TXT("(%S)"), value.to_char());
							}
						}
					}

					w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						value);
					w->alignPt.x = 1.0f;
					screen->add_widget(w);
				}
				at.y -= lineSize;
				at.y -= spacing;

				// kills
				{
					Vector2 wAt = at;

					auto* w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						NAME(lsKills));
					w->alignPt.x = 0.0f;
					screen->add_widget(w);

					String value = String::printf(TXT("%i"), showStats->kills);
					if (prevStats)
					{
						if (showStats->kills > prevStats->kills)
						{
							value = String::printf(TXT("+%i (%S)"), showStats->kills - prevStats->kills, value.to_char());
						}
						else
						{
							value = String::printf(TXT("(%S)"), value.to_char());
						}
					}

					w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						value);
					w->alignPt.x = 1.0f;
					screen->add_widget(w);
				}
				at.y -= lineSize;
				at.y -= spacing;
			}

			// blank line
			at.y -= lineSize;

			{
				Vector2 wAt = at;

				auto* w = new HubWidgets::Text(Name::invalid(),
					Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
					NAME(lsExperience)
				);
				w->alignPt.x = 0.0f;
				screen->add_widget(w);
			}
			at.y -= lineSize;
			//at.y -= spacing; // no spacing between - keep all experience related together

			if (showStats)
			{
				// experience gained
				{
					Vector2 wAt = at;

					auto* w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						NAME(lsExperienceGained));
					w->alignPt.x = 0.0f;
					w->with_smaller_decimals();
					screen->add_widget(w);

					String value;
					if (prevStats)
					{
						value = String::printf(TXT("+%S"), (showStats->experience - prevStats->experience).as_string_auto_decimals().to_char());
					}
					else
					{
						value = String::printf(TXT("+%S"), (showStats->experience).as_string_auto_decimals().to_char());
					}

					w = new HubWidgets::Text(Name::invalid(),
						Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
						value);
					w->alignPt.x = 1.0f;
					screen->add_widget(w);
				}
				at.y -= lineSize;
				// at.y -= spacing; // no spacing between
			
				// experience from mission
				if (auto* gs = TeaForGodEmperor::PlayerSetup::get_current().get_active_game_slot())
				{
					if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
					{
						{
							Vector2 wAt = at;

							auto* w = new HubWidgets::Text(Name::invalid(),
								Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
								NAME(lsExperienceMission));
							w->alignPt.x = 0.0f;
							w->with_smaller_decimals();
							screen->add_widget(w);

							String value;
							if (prevStats)
							{
								value = String::printf(TXT("+%S"), (showStats->experienceFromMission - prevStats->experienceFromMission).as_string_auto_decimals().to_char());
							}
							else
							{
								value = String::printf(TXT("+%S"), (showStats->experienceFromMission).as_string_auto_decimals().to_char());
							}

							w = new HubWidgets::Text(Name::invalid(),
								Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
								value);
							w->alignPt.x = 1.0f;
							screen->add_widget(w);
						}
						at.y -= lineSize;
						// at.y -= spacing; // no spacing between
					}
				}
			}

			// experience (total)
			{
				Vector2 wAt = at;

				auto* w = new HubWidgets::Text(Name::invalid(),
					Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
					NAME(lsExperienceTotal));
				w->alignPt.x = 0.0f;
				w->with_smaller_decimals();
				screen->add_widget(w);

				String value;
				if (auto* gs = TeaForGodEmperor::PlayerSetup::get_current().get_active_game_slot())
				{
					value = gs->stats.experience.as_string_auto_decimals();
				}

				w = new HubWidgets::Text(Name::invalid(),
					Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
					value);
				w->alignPt.x = 1.0f;
				screen->add_widget(w);
			}
			at.y -= lineSize;
			// at.y -= spacing; // no spacing between

			// experience to spend (total/current)
			{
				Vector2 wAt = at;

				auto* w = new HubWidgets::Text(Name::invalid(),
					Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
					NAME(lsExperienceToSpend));
				w->alignPt.x = 0.0f;
				w->with_smaller_decimals();
				screen->add_widget(w);

				String value = TeaForGodEmperor::Persistence::get_current().get_experience_to_spend().as_string_auto_decimals() + LOC_STR(NAME(lsCharsExperience));

				w = new HubWidgets::Text(Name::invalid(),
					Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
					value);
				w->alignPt.x = 1.0f;
				screen->add_widget(w);
			}
			at.y -= lineSize;
			at.y -= spacing;
			if (auto* gs = TeaForGodEmperor::PlayerSetup::get_current().get_active_game_slot())
			{
				if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
				{
					{
						Vector2 wAt = at;

						auto* w = new HubWidgets::Text(Name::invalid(),
							Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
							NAME(lsMeritPoints)
						);
						w->alignPt.x = 0.0f;
						screen->add_widget(w);
					}
					at.y -= lineSize;

					if (showStats)
					{
						{
							Vector2 wAt = at;

							auto* w = new HubWidgets::Text(Name::invalid(),
								Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
								NAME(lsMeritPointsGained));
							w->alignPt.x = 0.0f;
							w->with_smaller_decimals();
							screen->add_widget(w);

							String value;
							if (prevStats)
							{
								value = String::printf(TXT("+%i"), (showStats->meritPoints - prevStats->meritPoints)) + LOC_STR(NAME(lsCharsMeritPoints));
							}
							else
							{
								value = String::printf(TXT("+%i"), (showStats->meritPoints)) + LOC_STR(NAME(lsCharsMeritPoints));
							}

							w = new HubWidgets::Text(Name::invalid(),
								Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
								value);
							w->alignPt.x = 1.0f;
							screen->add_widget(w);
						}
						at.y -= lineSize;
					}
					{
						Vector2 wAt = at;

						auto* w = new HubWidgets::Text(Name::invalid(),
							Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
							NAME(lsMeritPointsToSpend));
						w->alignPt.x = 0.0f;
						w->with_smaller_decimals();
						screen->add_widget(w);

						String value = String::printf(TXT("%i"), TeaForGodEmperor::Persistence::get_current().get_merit_points_to_spend()) + LOC_STR(NAME(lsCharsMeritPoints));

						w = new HubWidgets::Text(Name::invalid(),
							Range2(wholeWidth, Range(wAt.y - lineSize, wAt.y)),
							value);
						w->alignPt.x = 1.0f;
						screen->add_widget(w);
					}
					at.y -= lineSize;
					at.y -= spacing;
				}
			}

			if (!TeaForGodEmperor::Game::get()->get_execution_tags().get_tag(NAME(showcase)))
			{
				Vector2 wAt = at;

				pleaseWaitWidget = new HubWidgets::Text(Name::invalid(),
					Range2(Range(wAt.x - fs.x * 8.0f, wAt.x + fs.x * 8.0f),
						Range(wAt.y - lineSize * 3.0f, wAt.y)),
					NAME(lsPleaseWait));
				screen->add_widget(pleaseWaitWidget.get());
			}

			screen->compress_vertically(fs.y * 1.0f, true);
		}

		summaryAt = screen->at;
		leftBottomAt = screen->at.pitch - screen->size.y * 0.5f;
		rightBottomAt = screen->at.pitch - screen->size.y * 0.5f;
	}


	{	// summary_gameModifiers
		HubScreen* screen = get_hub()->get_screen(NAME(summary_gameModifiers));

		float spacing = fsxy;
		float side = 1.0f;
		auto& bottomAt = rightBottomAt;

		if (!screen)
		{
			float width = 25.0f;
			Rotator3 at = summaryAt;
			at.yaw += side * (summaryWidth * 0.5f + screenSpacing + width * 0.5f);

			screen = new HubScreen(get_hub(), NAME(summary_gameModifiers), Vector2(width, 120.0f), at, radius, true);
			screen->activate();
			get_hub()->add_screen(screen);

			Range2 availableAt = screen->mainResolutionInPixels.expanded_by(-fsxy * Vector2::one);

			{
				auto* w = new HubWidgets::Text(Name::invalid(), availableAt, NAME(lsSummaryGameModifiersInfo));
				w->alignPt = Vector2(0.5f, 1.0f);
				screen->add_widget(w);
				float wSize = w->calculate_vertical_size();
				w->at.y.min = w->at.y.max - wSize;
				availableAt.y.max = w->at.y.min - spacing;
			}

			{
				Array<HubScreenButtonInfo> buttons;

				Range2 at = availableAt;
				at.y.min = at.y.max - fs.y * 2.0f;
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsSummaryGameModifiersOpen), [this]() { go_to_game_modifiers(); }));
				screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1), at, fs);
			}

			screen->compress_vertically(fsxy * 1.0f, true);
		}

		screen->at.pitch = bottomAt + screen->size.y * 0.5f;
		bottomAt = bottomAt + screen->size.y + screenSpacing;
	}

	{	// summary_gameSlots
		HubScreen* screen = get_hub()->get_screen(NAME(summary_gameSlots));

		float spacing = fsxy;
		float side = -1.0f;
		auto& bottomAt = leftBottomAt;

		if (!screen)
		{
			float width = 25.0f;
			Rotator3 at = summaryAt;
			at.yaw += side * (summaryWidth * 0.5f + screenSpacing + width * 0.5f);

			screen = new HubScreen(get_hub(), NAME(summary_gameSlots), Vector2(width, 120.0f), at, radius, true);
			screen->activate();
			get_hub()->add_screen(screen);

			Range2 availableAt = screen->mainResolutionInPixels.expanded_by(-fsxy * Vector2::one);

			{
				auto* w = new HubWidgets::Text(Name::invalid(), availableAt, NAME(lsSummaryGameSlotsInfo));
				w->alignPt = Vector2(0.5f, 1.0f);
				screen->add_widget(w);
				float wSize = w->calculate_vertical_size();
				w->at.y.min = w->at.y.max - wSize;
				availableAt.y.max = w->at.y.min - spacing;
			}

			{
				Array<HubScreenButtonInfo> buttons;

				Range2 at = availableAt;
				at.y.min = at.y.max - fs.y * 2.0f;
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsSummaryGameSlotsOpen), [this]() { go_to_game_slots(); }));
				screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1), at, fs);
			}

			screen->compress_vertically(fsxy * 1.0f, true);
		}

		screen->at.pitch = bottomAt + screen->size.y * 0.5f;
		bottomAt = bottomAt + screen->size.y + screenSpacing;
	}
}

void Summary::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

#ifdef AN_STANDARD_INPUT
	if (showContinue)
	{
		if (System::Input::get()->has_key_been_pressed(System::Key::Space))
		{
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPilgrimageSetup);
			}
			deactivateHub = true;
		}
	}
#endif
}

void Summary::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
}

void Summary::on_loader_deactivate()
{
	base::on_loader_deactivate();

	if (!showContinue)
	{
		showContinue = true;
		show_continue();
	}
}

void Summary::show_continue()
{
	if (auto* screen = get_hub()->get_screen(NAME(summary)))
	{
		if (pleaseWaitWidget.is_set())
		{
			Range2 exitAt = pleaseWaitWidget->at;

			screen->remove_widget(pleaseWaitWidget.get());
			pleaseWaitWidget.clear();

			RefCountObjectPtr<TeaForGodEmperor::GameState> continueFromGameState;
			if (summaryType == TeaForGodEmperor::PostGameSummary::Died)
			{
				if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
				{
					continueFromGameState = gs->get_game_state_to_continue();
				}
			}

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			float fsxy = max(fs.x, fs.y);

			Range2 continueAt = exitAt;
			if (continueFromGameState.is_set())
			{
				float moveBy = (continueAt.x.length() + fsxy) * 0.5f;
				exitAt.x.min -= moveBy;
				exitAt.x.max -= moveBy;
				continueAt.x.min += moveBy;
				continueAt.x.max += moveBy;
			}

			Range2 availableAt = screen->mainResolutionInPixels;
			availableAt.expand_by(-Vector2::one * fsxy);
			exitAt.x.min = max(exitAt.x.min, availableAt.x.min);
			continueAt.x.max = min(continueAt.x.max, availableAt.x.max);

			{
				exitWidget = new HubWidgets::Button(Name::invalid(), exitAt,
					NAME(lsExit));
				exitWidget->on_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
				{
					if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPilgrimageSetup);
					}
					deactivateHub = true;
				};
				screen->add_widget(exitWidget.get());
			}

			if (continueFromGameState.is_set())
			{
				continueFromCheckpointWidget = new HubWidgets::Button(Name::invalid(), continueAt,
					NAME(lsContinue));
				continueFromCheckpointWidget->on_click = [this, continueFromGameState](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
				{
					if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
						{
							gs->startUsingGameState = continueFromGameState;
						}
						game->set_meta_state(TeaForGodEmperor::GameMetaState::Pilgrimage);
					}
					deactivateHub = true;
				};
				screen->add_widget(continueFromCheckpointWidget.get());
			}
		}
	}
	else
	{
		if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
		{
			game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPilgrimageSetup);
		}
		deactivateHub = true;
	}
}

void Summary::go_to_game_modifiers()
{
	PilgrimageSetup::request_game_modifiers_on_next_setup();
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPilgrimageSetup);
	}
	deactivateHub = true;
}

void Summary::go_to_game_slots()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
		ps.add_game_slot_and_make_it_active();
		game->set_meta_state(TeaForGodEmperor::GameMetaState::SetupNewGameSlot);
	}
	deactivateHub = true;
}

