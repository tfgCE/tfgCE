#include "lhs_gameSlotSelection.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\scenes\lhs_quickGameMenu.h"
#include "..\screens\lhc_message.h"
#include "..\screens\lhc_question.h"
#include "..\screens\prayStations\lhc_spirographMandala.h"
#include "..\utils\lhu_playerStats.h"
#include "..\widgets\lhw_customDraw.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_list.h"
#include "..\widgets\lhw_rect.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameConfig.h"
#include "..\..\..\library\library.h"

#include "..\..\..\..\core\buildInformation.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// screens
DEFINE_STATIC_NAME(hubBackground);
DEFINE_STATIC_NAME(gameSlotList);
DEFINE_STATIC_NAME(gameSlotMenu);
DEFINE_STATIC_NAME(profileStats);

// ids
DEFINE_STATIC_NAME(startNewGameSlot);

// localised strings
DEFINE_STATIC_NAME_STR(lsAddNew, TXT("hub; game slot selection; add new"));

DEFINE_STATIC_NAME_STR(lsChangeProfile, TXT("hub; game slot selection; change profile"));

DEFINE_STATIC_NAME_STR(lsKills, TXT("hub; game slot selection; kills"));
DEFINE_STATIC_NAME_STR(lsDeaths, TXT("hub; game slot selection; deaths"));
DEFINE_STATIC_NAME_STR(lsExperience, TXT("hub; game slot selection; experience"));
DEFINE_STATIC_NAME_STR(lsAllEXMs, TXT("hub; game slot selection; all exms"));

// input
DEFINE_STATIC_NAME(move);

//

using namespace Loader;
using namespace HubScenes;

//

class GameSlotOnList
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(GameSlotOnList);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	int gameSlotIdx;
	int gameSlotId;
	String timeActive;
	String timeRun;
	String distance;
	String gameDefinitionName;
	String kills;
	String deaths;
	String experience;
	String experienceToSpend;
	String allEXMs;
	bool addNew = false;

	List<String> lines; // for add new lines
};
REGISTER_FOR_FAST_CAST(GameSlotOnList);

//

REGISTER_FOR_FAST_CAST(GameSlotSelection);

void GameSlotSelection::deactivate_screens()
{
	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));

	get_hub()->keep_only_screens(screensToKeep);
}

void GameSlotSelection::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);
	get_hub()->require_release_select();

	//
	
	deactivate_screens();

#ifdef SINGLE_GAME_SLOT
	start();
#else

	//

	float radius = get_hub()->get_radius() * 0.6f;

	float listHeight = 25.0f;
	float listOffset = 5.0f;
	float spacing = 2.5f;

	{	// menu
		HubScreen* screen = get_hub()->get_screen(NAME(profileStats));

		if (!screen)
		{
			Vector2 size(35.0f, 25.0f);
			Vector2 ppa(20.0f, 20.0f);
			Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();

			screen = new HubScreen(get_hub(), NAME(profileStats), size, atDir, radius, true, Rotator3(listOffset + listHeight * 0.5f + spacing + size.y * 0.5f, 0.0f, 0.0f), ppa);

			Array<HubScreenOption> options;
			options.make_space_for(20);

			auto& ps = TeaForGodEmperor::PlayerSetup::get_current();

			options.push_back(HubScreenOption(ps.get_profile_name()));

			Utils::add_options_from_player_stats(ps.get_stats(), options, Utils::PlayerStatsFlags::Global);

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			auto availableSpace = screen->mainResolutionInPixels;
			screen->add_option_widgets(options, availableSpace.expanded_by(-Vector2(max(fs.x, fs.y))));

			float changedBy = screen->compress_vertically(NP, NP);

			screen->verticalOffset.pitch += changedBy * 0.5f; // reduced (negative) -> we should be lower
			screen->update_placement();

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}
	
	{	// menu
		HubScreen* screen = get_hub()->get_screen(NAME(gameSlotMenu));

		if (!screen)
		{
			Vector2 size(30.0f, 10.0f);
			Vector2 ppa(20.0f, 20.0f);
			Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();

			screen = new HubScreen(get_hub(), NAME(gameSlotMenu), size, atDir, radius, true, Rotator3(listOffset - listHeight * 0.5f - spacing - size.y * 0.5f, 0.0f, 0.0f), ppa);

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsChangeProfile), [this]() { exit_to_player_profile_selection(); }));

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1),
				screen->mainResolutionInPixels.expanded_by(-fs), fs);

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}

	{	// game slot list
		HubScreen* screen = get_hub()->get_screen(NAME(gameSlotList));

		if (!screen)
		{
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				auto& ps = TeaForGodEmperor::PlayerSetup::get_current();
				
				Array<RefCountObjectPtr<GameSlotOnList>> gameSlots;
				for_every_ref(gs, ps.get_game_slots())
				{
					RefCountObjectPtr<GameSlotOnList> gsol(new GameSlotOnList());
					gsol->gameSlotId = gs->id;
					gsol->gameSlotIdx = for_everys_index(gs);
					gsol->timeActive = gs->stats.timeActive.get_simple_compact_hour_minute_second_string();
					gsol->timeRun = gs->stats.timeRun.get_simple_compact_hour_minute_second_string();
					gsol->distance = TXT("--");
					{
						MeasurementSystem::Type ms = ps.get_preferences().get_measurement_system();
						if (ms == MeasurementSystem::Imperial)
						{
							gsol->distance = String::printf(TXT("%.1f mi"), MeasurementSystem::to_miles(gs->stats.distance.as_meters()));
						}
						else
						{
							gsol->distance = String::printf(TXT("%.1f km"), MeasurementSystem::to_kilometers(gs->stats.distance.as_meters()));
						}
					}
					gsol->gameDefinitionName = gs->gameDefinition.get()? gs->gameDefinition->get_name_for_ui().get() : String::empty();
					gsol->deaths = String::printf(TXT("%i"), gs->stats.deaths);
					gsol->kills = String::printf(TXT("%i"), gs->stats.kills);
					gsol->experience = gs->stats.experience.as_string_auto_decimals();
					gsol->allEXMs = String::printf(TXT("%i"), gs->persistence.get_all_exms().get_size());
					gameSlots.push_back(gsol);
				}

				{
					RefCountObjectPtr<GameSlotOnList> gsol(new GameSlotOnList());
					gsol->addNew = true;
					gameSlots.push_back(gsol);
				}

				// calculate how much space do we require
				Vector2 fs = HubScreen::s_fontSizeInPixels;

				// name
				// time active
				// time run
				// distance
				// data blocks
				float inElementLineDistance = fs.y * 1.25f;
				float inElementMargin = max(fs.x, fs.y) * 0.75f;
				float elementWidth = fs.x * 14.0f
									+ inElementMargin * 2.0f; // margins
				float elementSpacing = max(fs.x, fs.y);

				float spaceRequiredPixels = (float)gameSlots.get_size() * (elementWidth + elementSpacing) + elementSpacing; // we actually add extra space on the sides of the list

				Vector2 size(60.0f, listHeight);
				Vector2 ppa(20.0f, 20.0f);
				Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();

				float sizeXRequired = spaceRequiredPixels / ppa.x;
				size.x = (5.0f * (elementWidth + elementSpacing) + elementSpacing) / ppa.x; // as above
				size.x = min(size.x, sizeXRequired);

				screen = new HubScreen(get_hub(), NAME(gameSlotList), size, atDir, radius, true, Rotator3(listOffset, 0.0f, 0.0f), ppa);

				{
					int maxWidth = (int)(screen->mainResolutionInPixels.x.length() - fs.x * 2.0f - fs.y);
					int tempLineCount;
					int tempMaxWidth;
					{	// break "add new" into lines
						addNewLines.clear();
						HubWidgets::Text::measure(get_hub()->get_font(), NP, NP, LOC_STR(NAME(lsAddNew)), tempLineCount, tempMaxWidth, maxWidth, &addNewLines);
					}

					for_every_ref(gsol, gameSlots)
					{
						if (gsol->addNew)
						{
							gsol->lines = addNewLines;
						}
					}
				}

				Range2 listAt = screen->mainResolutionInPixels.expanded_by(Vector2((-max(fs.x, fs.y))));

				auto* list = new HubWidgets::List(NAME(gameSlotList), listAt);
				list->horizontal = true;
				list->allowScrollWithStick = false;
				list->scroll.scrollType = size.x < sizeXRequired ? Loader::HubWidgets::InnerScroll::HorizontalScroll : Loader::HubWidgets::InnerScroll::NoScroll;
				list->elementSize = Vector2(elementWidth, list->at.y.length());
				list->elementSpacing = Vector2(elementSpacing, elementSpacing);
				list->draw_element = [inElementLineDistance, inElementMargin, list](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
				{
					if (auto* gsol = fast_cast<GameSlotOnList>(_element->data.get()))
					{
						if (auto* font = _display->get_font())
						{
							Vector2 fs = HubScreen::s_fontSizeInPixels;
							Vector2 at(_at.x.centre(), _at.y.max - inElementMargin - fs.y * 0.5f);
							Colour textColour = list->hub->is_selected(list, _element) ? HubColours::selected_highlighted() : HubColours::text();
							Colour textColourLeft = Colour::lerp(0.7f, HubColours::screen_interior(), textColour);

							if (gsol->addNew)
							{
								at.y = _at.y.centre();
								at.y += (gsol->lines.get_size() - 1) * 0.5f * inElementLineDistance;
								for_every(anl, gsol->lines)
								{
									font->draw_text_at(::System::Video3D::get(), anl->to_char(), textColour, at, Vector2::one, Vector2::half);
									at.y -= inElementLineDistance;
								}
							}
							else
							{
								::System::FontDrawingContext fdcSmallerDecimals;
								fdcSmallerDecimals.smallerDecimals = 0.5f;

								float left = _at.x.min + max(fs.x, fs.y);
								float right = _at.x.max - max(fs.x, fs.y);
								at.x = left;
								font->draw_text_at(::System::Video3D::get(), String::printf(TXT("#%i"), gsol->gameSlotId), textColour, at, Vector2::one, Vector2(0.0f, 0.5f));
								at.x = right;
								font->draw_text_at(::System::Video3D::get(), gsol->gameDefinitionName, textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								at.y -= inElementLineDistance;
								at.x = right;
								//font->draw_text_at(::System::Video3D::get(), gsol->timeActive, textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								//at.y -= inElementLineDistance;
								at.y -= round(inElementLineDistance * 0.5f);
								font->draw_text_at(::System::Video3D::get(), gsol->timeRun, textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								at.y -= inElementLineDistance;
								font->draw_text_at(::System::Video3D::get(), gsol->distance, textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								at.y -= inElementLineDistance;
								at.y -= inElementLineDistance;
								font->draw_text_at(::System::Video3D::get(), LOC_STR(NAME(lsExperience)), textColourLeft, Vector2(left, at.y), Vector2::one, Vector2(0.0f, 0.5f));
								font->draw_text_at(::System::Video3D::get(), gsol->experience, textColour, at, Vector2::one, Vector2(1.0f, 0.5f), NP, fdcSmallerDecimals);
								at.y -= inElementLineDistance;
								font->draw_text_at(::System::Video3D::get(), LOC_STR(NAME(lsAllEXMs)), textColourLeft, Vector2(left, at.y), Vector2::one, Vector2(0.0f, 0.5f));
								font->draw_text_at(::System::Video3D::get(), gsol->allEXMs, textColour, at, Vector2::one, Vector2(1.0f, 0.5f), NP, fdcSmallerDecimals);
								at.y -= inElementLineDistance;
								font->draw_text_at(::System::Video3D::get(), LOC_STR(NAME(lsKills)), textColourLeft, Vector2(left, at.y), Vector2::one, Vector2(0.0f, 0.5f));
								font->draw_text_at(::System::Video3D::get(), gsol->kills, textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								at.y -= inElementLineDistance;
								font->draw_text_at(::System::Video3D::get(), LOC_STR(NAME(lsDeaths)), textColourLeft, Vector2(left, at.y), Vector2::one, Vector2(0.0f, 0.5f));
								font->draw_text_at(::System::Video3D::get(), gsol->deaths, textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								at.y -= inElementLineDistance;
							}
						}
					}
				};

				HubDraggable* selectedDraggable = nullptr;
				{
					int selectedGameSlotIdx = ps.get_active_game_slot_idx();
					for_every_ref(gs, gameSlots)
					{
						auto* addedDraggable = list->add(gs);
						if (!gs->addNew &&
							(gs->gameSlotIdx == selectedGameSlotIdx ||
							 (!selectedDraggable && selectedGameSlotIdx == NONE))) // have first if none, it's still ok
						{
							selectedDraggable = addedDraggable;
						}
					}
				}

				screen->add_widget(list);

				if (selectedDraggable)
				{
					get_hub()->select(list, selectedDraggable);
					list->scroll_to_centre(selectedDraggable);
				}

				// do it post our select, to prevent from calling this code
				list->on_select = [this](HubDraggable const* _element, bool _clicked)
				{
					if (stickSelection) return;
					if (auto* gsol = fast_cast<GameSlotOnList>(_element->data.get()))
					{
						if (gsol->addNew)
						{
							add_new_game_slot();
						}
						else
						{
							select_game_slot_idx(gsol->gameSlotIdx);
						}
					}
				};

				screen->activate();
				get_hub()->add_screen(screen);
			}
		}
	}
#endif
}

void GameSlotSelection::exit_to_player_profile_selection()
{
	// we will wait in the external waiting zone
	deactivateHub = true;

	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		deactivate_screens();
		// it is important here to do this now and wait for it to be done, switching between meta states issues waiting
		game->add_async_save_player_profile();
		game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPlayerProfile);
	}
}

void GameSlotSelection::start()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
		if (! ps.get_active_game_slot())
		{
			add_new_game_slot();
		}
		else
		{
#ifdef SINGLE_GAME_SLOT
			// select the last one
			select_game_slot_idx(ps.get_game_slots().get_size() - 1);
#else
			if (auto* selected = get_hub()->get_selected_draggable())
			{
				if (auto* gsol = fast_cast<GameSlotOnList>(selected->data.get()))
				{
					if (gsol->addNew)
					{
						add_new_game_slot();
					}
					else
					{
						select_game_slot_idx(gsol->gameSlotIdx);
					}
				}
			}
#endif
		}
	}
}

void GameSlotSelection::select_game_slot_idx(int _gameSlotIdx)
{
	deactivateHub = true;
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
		ps.set_active_game_slot(_gameSlotIdx);
		if (auto* gs = ps.access_active_game_slot())
		{
			gs->prepare_missions_definition();
			gs->update_mission_state();
		}
		game->set_meta_state(TeaForGodEmperor::GameMetaState::PilgrimageSetup);
		Loader::Hub::reset_last_forward(); // we want to be where we face
	}
}

void GameSlotSelection::add_new_game_slot()
{
	deactivate_screens();

	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->add_async_world_job_top_priority(TXT("add new game slot"),
		[game, this]()
		{
			auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
			ps.add_game_slot_and_make_it_active();
			game->set_meta_state(TeaForGodEmperor::GameMetaState::SetupNewGameSlot);
			deactivateHub = true;
		});
	}
}

void GameSlotSelection::process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime)
{
	base::process_input(_handIdx, _input, _deltaTime);

	Vector2 currMoveStick = _input.get_stick(NAME(move));
	currMoveStick.x = round(currMoveStick.x);
	currMoveStick.y = round(currMoveStick.y);

	VectorInt2 moveBy = moveStick[_handIdx].update(currMoveStick, _deltaTime);
	
	if (moveBy.x != 0)
	{
		if (auto* s = get_hub()->get_screen(NAME(gameSlotList)))
		{
			if (auto* w = fast_cast<HubWidgets::List>(s->get_widget(NAME(gameSlotList))))
			{
				stickSelection = true;
				if (auto* newlySelected = w->change_selection_by(moveBy.x))
				{
					w->scroll_to_make_visible(newlySelected);
				}
				stickSelection = false;
				get_hub()->play_move_sample();
			}
		}
	}
}

