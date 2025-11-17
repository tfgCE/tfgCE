#include "lhs_playerProfileSelection.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\scenes\lhs_quickGameMenu.h"
#include "..\screens\lhc_message.h"
#include "..\screens\lhc_question.h"
#include "..\screens\prayStations\lhc_spirographMandala.h"
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
DEFINE_STATIC_NAME(profilesList);

// ids
DEFINE_STATIC_NAME(startNewProfile);

// localised strings
DEFINE_STATIC_NAME_STR(lsAddNew, TXT("hub; player profile selection; add new"));

// input
DEFINE_STATIC_NAME(move);

//

using namespace Loader;
using namespace HubScenes;

//

void PlayerProfileSelection::add_background()
{
	//HubScreens::SpirographMandala::show(get_hub(), NAME(hubBackground));
}

class ProfileOnList
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(ProfileOnList);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	String name;
	String time;
	String distance;
	bool addNew = false;

	List<String> lines; // for add new lines
};
REGISTER_FOR_FAST_CAST(ProfileOnList);

//

REGISTER_FOR_FAST_CAST(PlayerProfileSelection);

void PlayerProfileSelection::deactivate_screens()
{
	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));

	get_hub()->keep_only_screens(screensToKeep);
}

void PlayerProfileSelection::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	//
	
	deactivate_screens();

	//

	add_background();

	float radius = get_hub()->get_radius() * 0.6f;

	{	// profiles list
		HubScreen* screen = get_hub()->get_screen(NAME(profilesList));

		if (!screen)
		{
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				game->update_player_profiles_list(false /* do not select */);
				auto const& profileNames = game->get_player_profiles();
				String selectedProfile;
				if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
				{
					selectedProfile = c->get_player_profile_name();
				}

				// change to actual profiles
				Array<RefCountObjectPtr<ProfileOnList>> profiles;
				for_every(profileName, profileNames)
				{
					RefCountObjectPtr<ProfileOnList> pol(new ProfileOnList());
					pol->name = *profileName;
					pol->time = TXT("--");
					pol->distance = TXT("--");
					{
						TeaForGodEmperor::PlayerSetup ps;
						if (game->load_player_profile_stats_and_preferences_into(ps, pol->name))
						{
							pol->time = ps.get_stats().timeRun.get_simple_compact_hour_minute_second_string();
							MeasurementSystem::Type ms = ps.get_preferences().get_measurement_system();
							if (ms == MeasurementSystem::Imperial)
							{
								pol->distance = String::printf(TXT("%.1f mi"), MeasurementSystem::to_miles(ps.get_stats().distance.as_meters()));
							}
							else
							{
								pol->distance = String::printf(TXT("%.1f km"), MeasurementSystem::to_kilometers(ps.get_stats().distance.as_meters()));
							}
						}
					}
					profiles.push_back(pol);
				}

				{
					RefCountObjectPtr<ProfileOnList> pol(new ProfileOnList());
					pol->addNew = true;
					profiles.push_back(pol);
				}

				// calculate how much space do we require
				Vector2 fs = HubScreen::s_fontSizeInPixels;

				// name
				// time
				// distance
				float inElementLineDistance = fs.y * 1.25f;
				float inElementMargin = fs.y * 0.75f;
				float elementHeight = inElementLineDistance * 3.0f // three lines (see above)
									- (inElementLineDistance - fs.y) // minus one space between
									+ inElementMargin * 2.0f; // margins
				float elementSpacing = fs.y;

				float spaceRequiredPixels = (float)profiles.get_size() * (elementHeight + elementSpacing) + elementSpacing; // we actually add extra space above and below the list

				Vector2 size(25.0f, 30.0f);
				Vector2 ppa(20.0f, 20.0f);
				Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();

				float sizeYRequired = spaceRequiredPixels / ppa.y;
				size.y = (3.0f * (elementHeight + elementSpacing) + elementSpacing) / ppa.y; // as above
				size.y = min(size.y, sizeYRequired);

				screen = new HubScreen(get_hub(), NAME(profilesList), size, atDir, radius, NP, NP, ppa);

				{
					int maxWidth = (int)(screen->mainResolutionInPixels.x.length() - fs.x * 2.0f - fs.y);
					int tempLineCount;
					int tempMaxWidth;
					{	// break "add new" into lines
						addNewLines.clear();
						HubWidgets::Text::measure(get_hub()->get_font(), NP, NP, LOC_STR(NAME(lsAddNew)), tempLineCount, tempMaxWidth, maxWidth, &addNewLines);
					}
					for_every_ref(pol, profiles)
					{
						if (pol->addNew)
						{
							pol->lines = addNewLines;
						}
					}
					{	// limit names
						List<String> nameBroken;
						for_every_ref(pol, profiles)
						{
							nameBroken.clear();
							HubWidgets::Text::measure(get_hub()->get_font(), NP, NP, pol->name, tempLineCount, tempMaxWidth, maxWidth, &nameBroken);
							if (!nameBroken.is_empty())
							{
								pol->name = nameBroken.get_first();
							}
						}
					}
				}

				Range2 listAt = screen->mainResolutionInPixels.expanded_by(Vector2(-max(fs.x, fs.y)));
				// fine tweaked by hand to have exactly three visible
				//listAt.y.max += 2.0f;
				//listAt.y.min -= 2.0f;

				auto* list = new HubWidgets::List(NAME(profilesList), listAt);
				list->allowScrollWithStick = false;
				list->scroll.scrollType = size.y < sizeYRequired ? Loader::HubWidgets::InnerScroll::VerticalScroll : Loader::HubWidgets::InnerScroll::NoScroll;
				list->elementSize = Vector2(list->at.x.length(), elementHeight);
				list->elementSpacing = Vector2(elementSpacing, elementSpacing);
				list->draw_element = [inElementLineDistance, inElementMargin, list](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
				{
					if (auto* pol = fast_cast<ProfileOnList>(_element->data.get()))
					{
						if (auto* font = _display->get_font())
						{
							Vector2 fs = HubScreen::s_fontSizeInPixels;
							Vector2 at(_at.x.centre(), _at.y.max - inElementMargin - fs.y * 0.5f);
							Colour textColour = list->hub->is_selected(list, _element) ? HubColours::selected_highlighted() : HubColours::text();

							if (pol->addNew)
							{
								at.y = _at.y.centre();
								at.y += (pol->lines.get_size() - 1) * 0.5f * inElementLineDistance;
								for_every(anl, pol->lines)
								{
									font->draw_text_at(::System::Video3D::get(), anl->to_char(), textColour, at, Vector2::one, Vector2::half);
									at.y -= inElementLineDistance;
								}
							}
							else
							{
								font->draw_text_at(::System::Video3D::get(), pol->name.to_char(), textColour, at, Vector2::one, Vector2::half);
								at.y -= inElementLineDistance;
								at.x = _at.x.max - max(fs.x, fs.y);
								font->draw_text_at(::System::Video3D::get(), pol->time.to_char(), textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								at.y -= inElementLineDistance;
								font->draw_text_at(::System::Video3D::get(), pol->distance.to_char(), textColour, at, Vector2::one, Vector2(1.0f, 0.5f));
								at.y -= inElementLineDistance;
							}
						}
					}
				};

				HubDraggable* selectedDraggable = nullptr;
				for_every_ref(profile, profiles)
				{
					auto* addedDraggable = list->add(profile);
					if (!profile->addNew &&
						!selectedProfile.is_empty() &&
						(!selectedDraggable || profile->name == selectedProfile)) // have first if none, it's still ok
					{
						selectedDraggable = addedDraggable;
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
					if (auto* pol = fast_cast<ProfileOnList>(_element->data.get()))
					{
						if (pol->addNew)
						{
							add_new_profile();
						}
						else
						{
							select_profile(pol->name);
						}
					}
				};

				screen->activate();
				get_hub()->add_screen(screen);
			}
		}
	}
}

void PlayerProfileSelection::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
}

void PlayerProfileSelection::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
}

void PlayerProfileSelection::on_loader_deactivate()
{
	base::on_loader_deactivate();
}

void PlayerProfileSelection::start()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->update_player_profiles_list(false /* do not select */);
		if (game->get_player_profiles().is_empty())
		{
			add_new_profile();
		}
		else
		{
			if (auto* selected = get_hub()->get_selected_draggable())
			{
				if (auto* pol = fast_cast<ProfileOnList>(selected->data.get()))
				{
					if (pol->addNew)
					{
						add_new_profile();
					}
					else
					{
						select_profile(pol->name);
					}
				}
			}
		}
	}
}

void PlayerProfileSelection::select_profile(String const& _profileName)
{
	deactivate_screens();
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->add_async_world_job_top_priority(TXT("select profile"),
		[this, game, _profileName]()
		{
			game->use_quick_game_player_profile(false, _profileName);
			game->save_config_file(); // to store selected profile
			game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectGameSlot);
			deactivateHub = true;
		});
	}
}

void PlayerProfileSelection::add_new_profile()
{
	deactivateHub = true;
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->set_meta_state(TeaForGodEmperor::GameMetaState::SetupNewPlayerProfile);
	}
}

void PlayerProfileSelection::process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime)
{
	base::process_input(_handIdx, _input, _deltaTime);

	Vector2 currMoveStick = _input.get_stick(NAME(move));
	currMoveStick.x = round(currMoveStick.x);
	currMoveStick.y = round(currMoveStick.y);

	VectorInt2 moveBy = moveStick[_handIdx].update(currMoveStick, _deltaTime);

	if (moveBy.y != 0)
	{
		if (auto* s = get_hub()->get_screen(NAME(profilesList)))
		{
			if (auto* w = fast_cast<HubWidgets::List>(s->get_widget(NAME(profilesList))))
			{
				stickSelection = true;
				if (auto* newlySelected = w->change_selection_by(-moveBy.y))
				{
					w->scroll_to_make_visible(newlySelected);
				}
				stickSelection = false;
				get_hub()->play_move_sample();
			}
		}
	}
}

