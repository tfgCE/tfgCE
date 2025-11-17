#include "lhs_startingPilgrimageSelection.h"

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

#include "..\..\..\..\framework\library\usedLibraryStored.inl"

#include "..\..\..\..\core\buildInformation.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// screens
DEFINE_STATIC_NAME(pilgrimagesList);

// localised strings
DEFINE_STATIC_NAME_STR(lsCustomiseFirst, TXT("hub; game definition selection; customise first"));

// input
DEFINE_STATIC_NAME(move);

//

using namespace Loader;
using namespace HubScenes;

//

class PilgrimageOnList
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(PilgrimageOnList);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	String name;
	Framework::UsedLibraryStored<TeaForGodEmperor::Pilgrimage> pilgrimage;
	bool customiseFirst = false;
};
REGISTER_FOR_FAST_CAST(PilgrimageOnList);

//

REGISTER_FOR_FAST_CAST(StartingPilgrimageSelection);

void StartingPilgrimageSelection::deactivate_screens()
{
	Array<Name> screensToKeep;

	get_hub()->keep_only_screens(screensToKeep);
}

void StartingPilgrimageSelection::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	//
	
	deactivate_screens();

	//

	float radius = get_hub()->get_radius() * 0.6f;

	{	// pilgrimages list
		HubScreen* screen = get_hub()->get_screen(NAME(pilgrimagesList));

		if (!screen)
		{
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
				if (auto* gs = ps.access_active_game_slot())
				{
					gameSlot = gs;
				}

				Array<RefCountObjectPtr<PilgrimageOnList>> pilgrimageList;

				if (auto* gs = gameSlot.get())
				{
					if (auto* gd = TeaForGodEmperor::GameDefinition::get_chosen())
					{
						gs->update_unlocked_pilgrimages(TeaForGodEmperor::PlayerSetup::get_current(), gd->get_pilgrimages(), gd->get_conditional_pilgrimages(), true);
					}
					if (!gs->unlockedPilgrimages.is_empty())
					{
						for_every(pn, gs->unlockedPilgrimages)
						{
							RefCountObjectPtr<PilgrimageOnList> pol;

							pol = new PilgrimageOnList();

							pol->pilgrimage.set_name(*pn);
							pol->pilgrimage.find(TeaForGodEmperor::Library::get_current());

							if (auto* p = pol->pilgrimage.get())
							{
								String connector = p->get_pilgrimage_name_pre_connector_ls().get().is_empty() ? String::space() : String::space() + p->get_pilgrimage_name_pre_connector_ls().get() + String::space();
								pol->name = (p->get_pilgrimage_name_pre_short_ls().get() + connector + p->get_pilgrimage_name_ls().get()).trim();
								pilgrimageList.push_back(pol);
							}
						}
					}
				}

				{
					RefCountObjectPtr<PilgrimageOnList> pol(new PilgrimageOnList());
					pol->customiseFirst = true;
					pol->name = LOC_STR(NAME(lsCustomiseFirst));
					pilgrimageList.push_back(pol);
				}

				// calculate how much space do we require
				Vector2 fs = HubScreen::s_fontSizeInPixels;

				float inElementMargin = fs.y * 0.75f;
				float elementHeight = fs.y
									+ inElementMargin * 2.0f; // margins
				float elementSpacing = fs.y;

				float spaceRequiredPixels = (float)pilgrimageList.get_size() * (elementHeight + elementSpacing) + elementSpacing; // we actually add extra space above and below the list

				Vector2 size(35.0f, 30.0f);
				Vector2 ppa(20.0f, 20.0f);
				Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();

				float sizeYRequired = spaceRequiredPixels / ppa.y;
				size.y = (3.0f * (elementHeight + elementSpacing) + elementSpacing) / ppa.y; // as above
				size.y = min(size.y, sizeYRequired);

				screen = new HubScreen(get_hub(), NAME(pilgrimagesList), size, atDir, radius, NP, NP, ppa);

				Range2 listAt = screen->mainResolutionInPixels.expanded_by(Vector2(-max(fs.x, fs.y)));
				// fine tweaked by hand to have exactly three visible
				//listAt.y.max += 2.0f;
				//listAt.y.min -= 2.0f;

				auto* list = new HubWidgets::List(NAME(pilgrimagesList), listAt);
				list->allowScrollWithStick = false;
				list->scroll.scrollType = size.y < sizeYRequired ? Loader::HubWidgets::InnerScroll::VerticalScroll : Loader::HubWidgets::InnerScroll::NoScroll;
				list->elementSize = Vector2(list->at.x.length(), elementHeight);
				list->elementSpacing = Vector2(elementSpacing, elementSpacing);
				list->draw_element = [inElementMargin, list](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
				{
					if (auto* pol = fast_cast<PilgrimageOnList>(_element->data.get()))
					{
						if (auto* font = _display->get_font())
						{
							Vector2 fs = HubScreen::s_fontSizeInPixels;
							Vector2 at(_at.x.centre(), _at.y.max - inElementMargin - fs.y * 0.5f);
							Colour textColour = list->hub->is_selected(list, _element) ? HubColours::selected_highlighted() : HubColours::text();

							at.y = _at.y.centre();
							font->draw_text_at(::System::Video3D::get(), pol->name.to_char(), textColour, at, Vector2::one, Vector2::half);
						}
					}
				};

				HubDraggable* selectedDraggable = nullptr;
				for_every_ref(pl, pilgrimageList)
				{
					auto* addedDraggable = list->add(pl);
					if (!selectedDraggable)
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
					if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						deactivate_screens();

						bool customiseFirst = false;
						if (auto* pol = fast_cast<PilgrimageOnList>(_element->data.get()))
						{
							if (pol->customiseFirst)
							{
								customiseFirst = true;
							}
							else
							{
								if (auto* gs = gameSlot.get())
								{
									gs->restartAtPilgrimage = pol->pilgrimage.get_name();
								}
							}
						}

						game->add_async_world_job_top_priority(TXT("done, setup pilgrimage"),
							[this, game, customiseFirst]()
							{
								game->save_config_file();
								game->save_player_profile();
								if (customiseFirst)
								{
									game->set_meta_state(TeaForGodEmperor::GameMetaState::PilgrimageSetup); // show the menu, allow to customise
								}
								else
								{
									game->set_meta_state(TeaForGodEmperor::GameMetaState::Pilgrimage); // play the game!
								}
								deactivateHub = true;
							});
					}
				};

				screen->activate();
				get_hub()->add_screen(screen);
			}
		}
	}
}

void StartingPilgrimageSelection::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
}

void StartingPilgrimageSelection::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
}

void StartingPilgrimageSelection::on_loader_deactivate()
{
	base::on_loader_deactivate();
}

void StartingPilgrimageSelection::process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime)
{
	base::process_input(_handIdx, _input, _deltaTime);

	Vector2 currMoveStick = _input.get_stick(NAME(move));
	currMoveStick.x = round(currMoveStick.x);
	currMoveStick.y = round(currMoveStick.y);

	VectorInt2 moveBy = moveStick[_handIdx].update(currMoveStick, _deltaTime);

	if (moveBy.y != 0)
	{
		if (auto* s = get_hub()->get_screen(NAME(pilgrimagesList)))
		{
			if (auto* w = fast_cast<HubWidgets::List>(s->get_widget(NAME(pilgrimagesList))))
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

