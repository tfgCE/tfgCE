#include "lhs_tutorialSelection.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\scenes\lhs_beAtRightPlace.h"
#include "..\scenes\lhs_options.h"
#include "..\screens\lhc_handGestures.h"
#include "..\screens\lhc_question.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_text.h"
#include "..\widgets\lhw_list.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\tutorials\tutorial.h"
#include "..\..\..\tutorials\tutorialTree.h"
#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\video\font.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsStart, TXT("tutorials; selection; start"));
DEFINE_STATIC_NAME_STR(lsRedo, TXT("tutorials; selection; redo"));
DEFINE_STATIC_NAME_STR(lsRedoQuestion, TXT("tutorials; selection; redo question"));
DEFINE_STATIC_NAME_STR(lsBack, TXT("hub; common; back"));
DEFINE_STATIC_NAME_STR(lsSectionSuffix, TXT("tutorials; sections suffix"));
DEFINE_STATIC_NAME_STR(lsKeepProgressInfo, TXT("tutorials; keep progress info"));

// screens
DEFINE_STATIC_NAME(tutorialSelection);
DEFINE_STATIC_NAME(tutorialKeepProgressInfo);

// widget id
DEFINE_STATIC_NAME(tutorialSelectionList);

// fade reasons
DEFINE_STATIC_NAME(backFromTutorialSelection);

// input
DEFINE_STATIC_NAME(move);

//

using namespace Loader;
using namespace HubScenes;

//

class TutorialDraggable
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(TutorialDraggable);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	TeaForGodEmperor::Tutorial* tutorial;
	String text;

	TutorialDraggable(TeaForGodEmperor::TutorialTreeElement* tte)
	{
		tutorial = fast_cast<TeaForGodEmperor::Tutorial>(tte);
		for_count(int, i, tte->get_tutorial_tree_element_indent())
		{
			text += TXT("  ");
		}
		text += tte->get_tutorial_tree_element_name().get();
		if (! tte->get_children().is_empty())
		{
			text += LOC_STR(NAME(lsSectionSuffix));
		}
	}
};
REGISTER_FOR_FAST_CAST(TutorialDraggable);

//

REGISTER_FOR_FAST_CAST(TutorialSelection);

void TutorialSelection::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	deactivateHub = false;
	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	show_list();
}

void TutorialSelection::show_list()
{
	bool isKeepingProgress = true;

	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		if (game->is_using_quick_game_player_profile())
		{
			isKeepingProgress = false;
		}
	}

	get_hub()->keep_only_screen(NAME(tutorialSelection));

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsxy = max(fs.x, fs.y);

	Vector2 mainSize(60.0f, 40.0f);
	float spacing = 2.0f;
	HubScreen* screen = get_hub()->get_screen(NAME(tutorialSelection));
	if (! screen)
	{
		Vector2 size = mainSize;
		Vector2 ppa(20.0f, 20.0f);
		Rotator3 atDir = get_hub()->get_current_forward();
		
		screen = new HubScreen(get_hub(), NAME(tutorialSelection), size, atDir, get_hub()->get_radius() * 0.7f, true, NP, ppa);
		screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

		screen->activate();
		get_hub()->add_screen(screen);

		Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-Vector2(fsxy));

		{
			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsStart), [this]() { start_tutorial(); }).activate_on_trigger_hold());
			if (isKeepingProgress)
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsRedo), [this]() { redo_all(); }));
			}
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { go_back(); }));

			Range2 at;
			float off = 0.4f;
			at.x.min = screen->mainResolutionInPixels.x.get_at(0.5f - off);
			at.x.max = screen->mainResolutionInPixels.x.get_at(0.5f + off);
			at.y.min = screen->mainResolutionInPixels.y.min + fsxy;
			at.y.max = at.y.min + fs.y * 2.0f;
			screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1), at, fs);

			availableSpace.y.min = at.y.max + fsxy;
		}

		{
			auto* w = new HubWidgets::List(NAME(tutorialSelectionList), availableSpace);
			w->allowScrollWithStick = false;
			w->elementSize.y = round(fs.y * 1.3f);
			w->drawElementBorder = false;
			w->drawSelectedColours = false;

			w->draw_element = [w, isKeepingProgress, fsxy](Framework::Display* _display, Range2 const& _at, Loader::HubDraggable const* _element)
			{
				if (auto* td = fast_cast<TutorialDraggable>(_element->data.get()))
				{
					if (auto* font = _display->get_font())
					{
						Colour colour = HubColours::text();
						if (td->tutorial && isKeepingProgress && TeaForGodEmperor::PlayerSetup::get_current().was_tutorial_done(td->tutorial->get_name()))
						{
							colour = Colour::lerp(0.7f, HubColours::widget_background(), Colour::lerp(0.7f, Colour::green, colour));
						}
						if (w->hub->is_selected(w, _element))
						{
							colour = HubColours::selected_highlighted();
						}
						if (!td->tutorial)
						{
							colour = Colour::lerp(0.75f, HubColours::screen_interior(), HubColours::text());
						}
						font->draw_text_at(::System::Video3D::get(), td->text, colour, Vector2(_at.x.min + fsxy, _at.y.centre()), Vector2::one, Vector2(0.0f, 0.5f));
					}
				}
			};
			w->on_double_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
			{
				start_tutorial();
			};
			listWidget = w;
			{
				bool selected = false;
				for_every_ptr(tte, TeaForGodEmperor::TutorialSystem::get()->get_flat_tree())
				{
					auto* nd = new TutorialDraggable(tte);
					w->add(nd);
					if (nd->tutorial &&
						nd->tutorial->get_name() == TeaForGodEmperor::PlayerSetup::get_current().get_last_tutorial_started())
					{
						get_hub()->select(w, w->elements.get_last().get());
						w->scroll_to_make_visible(w->elements.get_last().get());
						selected = true;
					}
				}
				if (!selected)
				{
					if (auto* tutorial = TeaForGodEmperor::TutorialSystem::get()->get_next_tutorial())
					{
						for_every_ref(e, w->elements)
						{
							if (auto* et = fast_cast<TutorialDraggable>(e->data.get()))
							{
								if (et->tutorial == tutorial)
								{
									get_hub()->select(w, e);
									w->scroll_to_make_visible(e);
									selected = true;
								}
							}
						}
					}
				}
			}
			screen->add_widget(w);
		}
	}

	if (!isKeepingProgress)
	{
		HubScreen* screen = get_hub()->get_screen(NAME(tutorialKeepProgressInfo));
		if (!screen)
		{
			Vector2 size(40.0f, 10.0f);
			Vector2 ppa(20.0f, 20.0f);
			Rotator3 atDir = get_hub()->get_current_forward();
			Rotator3 verticalOffset(mainSize.y * 0.5f + spacing + size.y * 0.5f, 0.0f, 0.0f);

			screen = new HubScreen(get_hub(), NAME(tutorialKeepProgressInfo), size, atDir, get_hub()->get_radius() * 0.7f, true, verticalOffset, ppa);
			screen->followScreen = NAME(tutorialSelection);

			screen->activate();
			get_hub()->add_screen(screen);

			Range2 availableSpace = screen->mainResolutionInPixels.expanded_by(-Vector2(fsxy));

			{
				auto* w = new HubWidgets::Text(Name::invalid(), availableSpace, LOC_STR(NAME(lsKeepProgressInfo)));
				screen->add_widget(w);
			}

			float compressedBy = screen->compress_vertically(NP, NP);
			screen->at.pitch -= compressedBy * 0.5f;
		}
	}
}

void TutorialSelection::redo_all()
{
	HubScreens::Question::ask(get_hub(), NAME(lsRedoQuestion),
	[]()
	{
		TeaForGodEmperor::PlayerSetup::access_current().redo_tutorials();
	},
	[]()
	{
		// we'll just get back
	}
	);
}

void TutorialSelection::start_tutorial()
{
	if (auto * tutorial = get_selected_tutorial())
	{
		if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
		{
			game->set_meta_state(TeaForGodEmperor::GameMetaState::Tutorial);
			game->set_post_run_tutorial(false);
			TeaForGodEmperor::TutorialSystem::get()->start_tutorial(tutorial);
			TeaForGodEmperor::TutorialSystem::get()->pause_tutorial();
			get_hub()->deactivate_all_screens();
			deactivateHub = true;
		}
	}
}

void TutorialSelection::go_back()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->set_meta_state(game->get_post_tutorial_selection_meta_state());
		get_hub()->deactivate_all_screens();
		deactivateHub = true;
	}
}

TeaForGodEmperor::Tutorial* TutorialSelection::get_selected_tutorial() const
{
	if (auto* sd = get_hub()->get_selected_draggable())
	{
		if (auto* td = fast_cast<TutorialDraggable>(sd->data.get()))
		{
			return td->tutorial;
		}
	}
	return nullptr;
}

void TutorialSelection::process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime)
{
	base::process_input(_handIdx, _input, _deltaTime);

	Vector2 currMoveStick = _input.get_stick(NAME(move));
	currMoveStick.x = round(currMoveStick.x);
	currMoveStick.y = round(currMoveStick.y);

	VectorInt2 moveBy = moveStick[_handIdx].update(currMoveStick, _deltaTime);

	if (moveBy.y != 0)
	{
		if (auto* s = get_hub()->get_screen(NAME(tutorialSelection)))
		{
			if (auto* w = fast_cast<HubWidgets::List>(s->get_widget(NAME(tutorialSelectionList))))
			{
				// continue until we run into one with tutorial
				auto* prevSelected = get_hub()->get_selected_draggable();
				while (auto* newlySelected = w->change_selection_by(-moveBy.y))
				{
					if (prevSelected == newlySelected)
					{
						break;
					}
					if (auto* td = fast_cast<TutorialDraggable>(newlySelected->data.get()))
					{
						if (td->tutorial)
						{
							break;
						}
					}
					prevSelected = newlySelected;
				}
				w->scroll_to_make_visible(get_hub()->get_selected_draggable());
				get_hub()->play_move_sample();
			}
		}
	}
}

