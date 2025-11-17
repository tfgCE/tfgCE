#include "lhc_gameDefinitionSelection.h"

#include "..\lhc_message.h"
#include "..\lhc_question.h"

#include "..\..\loaderHub.h"
#include "..\..\scenes\lhs_calibrate.h"
#include "..\..\scenes\lhs_options.h"
#include "..\..\widgets\lhw_button.h"

#include "..\..\..\..\game\playerSetup.h"
#include "..\..\..\..\library\library.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define ONE_CLICK_NEXT

//

// localised strings
DEFINE_STATIC_NAME_STR(lsStart, TXT("hub; game definition selection; start"));
DEFINE_STATIC_NAME_STR(lsCustomiseFirst, TXT("hub; game definition selection; customise first"));
DEFINE_STATIC_NAME_STR(lsMoreInfo, TXT("hub; game definition selection; more info"));
DEFINE_STATIC_NAME_STR(lsGameDefinition, TXT("hub; game definition selection; game type"));
DEFINE_STATIC_NAME_STR(lsGameSubDefinition, TXT("hub; game definition selection; on fail"));
DEFINE_STATIC_NAME_STR(lsSelectionSummary, TXT("hub; game definition selection; selection summary"));
DEFINE_STATIC_NAME_STR(lsNext, TXT("hub; game definition selection; next"));
DEFINE_STATIC_NAME_STR(lsPrev, TXT("hub; game definition selection; prev"));

DEFINE_STATIC_NAME_STR(lsCalibrate, TXT("hub; game menu; calibrate"));
DEFINE_STATIC_NAME_STR(lsOptions, TXT("hub; game menu; options"));

// screen ids
DEFINE_STATIC_NAME(gameDefinitionSelection); // actual screen with logic, etc. all should go through it, it also locates other screens
DEFINE_STATIC_NAME(descriptionAndStart); // helping screen
DEFINE_STATIC_NAME(prevNextMenu); // helping screen
DEFINE_STATIC_NAME(bottomMenu); // helping screen

// ids
DEFINE_STATIC_NAME(idCalibrate);
DEFINE_STATIC_NAME(idGeneralDescription);
DEFINE_STATIC_NAME(idPrev);
DEFINE_STATIC_NAME(idNext);

//

using namespace Loader;
using namespace HubScreens;

//

REGISTER_FOR_FAST_CAST(GameDefinitionSelection);

HubScreen* GameDefinitionSelection::show(Hub* _hub, Name const& _id, TeaForGodEmperor::PlayerGameSlot* _gameSlot, WhatToShow _whatToShow, std::function<void()> _onPrev, std::function<void(bool _startImmediately)> _onNextSelect)
{
	Array<TeaForGodEmperor::GameDefinition*> gameDefinitions;
	Array<TeaForGodEmperor::GameSubDefinition*> gameSubDefinitions;
	if (auto* lib = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>())
	{
		lib->get_game_definitions().do_for_every([&gameDefinitions](Framework::LibraryStored* _libraryStored)
			{
				if (auto* gd = fast_cast<TeaForGodEmperor::GameDefinition>(_libraryStored))
				{
					if (gd->get_player_profile_requirements().check(TeaForGodEmperor::PlayerSetup::get_current().get_game_unlocks()))
					{
						gameDefinitions.push_back(gd);
					}
				}
			});
		sort(gameDefinitions, [](void const* _a, void const* _b)
			{
				TeaForGodEmperor::GameDefinition const* gda = *(plain_cast<TeaForGodEmperor::GameDefinition*>(_a));
				TeaForGodEmperor::GameDefinition const* gdb = *(plain_cast<TeaForGodEmperor::GameDefinition*>(_b));
				float spDiff = gda->get_selection_placement() - gdb->get_selection_placement();
				if (spDiff < 0.0f) return A_BEFORE_B;
				if (spDiff > 0.0f) return B_BEFORE_A;
				return A_AS_B;
			});
		lib->get_game_sub_definitions().do_for_every([&gameSubDefinitions](Framework::LibraryStored* _libraryStored)
			{
				if (auto* gsd = fast_cast<TeaForGodEmperor::GameSubDefinition>(_libraryStored))
				{
					if (gsd->get_player_profile_requirements().check(TeaForGodEmperor::PlayerSetup::get_current().get_game_unlocks()))
					{
						gameSubDefinitions.push_back(gsd);
					}
				}
			});
		sort(gameSubDefinitions, [](void const* _a, void const* _b)
			{
				TeaForGodEmperor::GameSubDefinition const* gsda = *(plain_cast<TeaForGodEmperor::GameSubDefinition*>(_a));
				TeaForGodEmperor::GameSubDefinition const* gsdb = *(plain_cast<TeaForGodEmperor::GameSubDefinition*>(_b));
				float spDiff = gsda->get_selection_placement() - gsdb->get_selection_placement();
				if (spDiff < 0.0f) return A_BEFORE_B;
				if (spDiff > 0.0f) return B_BEFORE_A;
				return A_AS_B;
			});
	}

	if (gameDefinitions.is_empty())
	{
		return nullptr;
	}

	Rotator3 mainAtDir = _hub->get_current_forward();

	GameDefinitionSelection* gdsScreen = nullptr;

	Vector2 ppa = Vector2::one * 20.0f;

	Optional<float> topAt;
	float spacingY = 2.5f;
	if (_whatToShow == WhatToShow::ShowAll ||
		_whatToShow == WhatToShow::SelectGameDefinition ||
		_whatToShow == WhatToShow::SelectGameSubDefinition)
	{
		Rotator3 atDir = mainAtDir;
		Vector2 size(65.0f, _whatToShow == WhatToShow::ShowAll? 32.0f : (_whatToShow == WhatToShow::SelectGameDefinition? 22.0f : 16.0f));
		if (topAt.is_set())
		{
			atDir.pitch = topAt.get() - size.y * 0.5f;
		}
		else
		{
			atDir.pitch += size.y * 0.5f - 5.0f;
		}

		GameDefinitionSelection* s = new GameDefinitionSelection(_hub, NAME(gameDefinitionSelection), _gameSlot, _whatToShow, _onNextSelect, gameDefinitions, gameSubDefinitions, atDir, size, _hub->get_radius() * 0.6f, ppa);

		s->activate();
		_hub->add_screen(s);

		gdsScreen = s;
		topAt = atDir.pitch - size.y * 0.5f - spacingY;
	}
	if (_whatToShow == WhatToShow::ShowAll ||
		_whatToShow == WhatToShow::ShowSummary)
	{
		Rotator3 atDir = mainAtDir;
		Vector2 size(65.0f, 17.0f);
		if (TeaForGodEmperor::is_demo())
		{
			size.y = 22.0f;
		}
		if (topAt.is_set())
		{
			atDir.pitch = topAt.get() - size.y * 0.5f;
		}
		else
		{
			atDir.pitch += size.y * 0.5f - 5.0f;
		}

		HubScreen* s = new HubScreen(_hub, NAME(descriptionAndStart), size, atDir, _hub->get_radius() * 0.6f, true, NP, ppa);

		s->activate();
		_hub->add_screen(s);
		topAt = atDir.pitch - size.y * 0.5f - spacingY;

		{
			Vector2 fs = HubScreen::s_fontSizeInPixels;

			float spacing = max(fs.x, fs.y);
			Range2 available = s->mainResolutionInPixels;
			available.expand_by(-Vector2::one * spacing);

			{
				Range2 availableOnSide = available;
				availableOnSide.x.min = round(availableOnSide.x.get_at(0.667f));

				{
					auto at = availableOnSide;
					at.y.max = at.y.min + fs.y * 2.5f;
					auto* b = new HubWidgets::Button(NAME(lsCustomiseFirst), at, LOC_STR(NAME(lsCustomiseFirst)));
					b->on_click = [_onNextSelect](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
						{
							_onNextSelect(false);
						};

					s->add_widget(b);
					availableOnSide.y.min = at.y.max + spacing;
				}

				{
					auto at = availableOnSide;
					auto* b = new HubWidgets::Button(NAME(lsStart), at, LOC_STR(NAME(lsStart)));
					b->scale = Vector2::one * 3.0f;
					b->on_click = [_onNextSelect](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
						{
							_onNextSelect(true);
						};

					s->add_widget(b);
				}

				available.x.max = availableOnSide.x.min - spacing;
			}

			{
				Range2 availableSummary = available;
				if (! TeaForGodEmperor::is_demo())
				{
					Range2 at = availableSummary;
					auto* e = new HubWidgets::Text(Name::invalid(), at, LOC_STR(NAME(lsSelectionSummary)));
					e->alignPt.x = 0.0f;
					e->alignPt.y = 1.0f;
					s->add_widget(e);
					e->at.y.min = e->at.y.max - e->calculate_vertical_size();
					availableSummary.y.max = e->at.y.min - spacing;
				}
				{
					Range2 at = availableSummary;
					auto* e = new HubWidgets::Text(NAME(idGeneralDescription), at, String::empty());
					e->alignPt.x = 0.0f;
					e->alignPt.y = 1.0f;
					s->add_widget(e);

					if (gdsScreen)
					{
						gdsScreen->generalDescriptionWidget = e;
					}
					else
					{
						String desc = String::empty();

						if (auto* gd = _gameSlot->get_game_definition())
						{
							desc = gd->get_general_desc_for_ui(_gameSlot->get_game_sub_definitions());
						}
						e->set(desc);
					}
				}
			}

			_hub->reset_highlights();
			_hub->add_highlight_blink(NAME(descriptionAndStart), NAME(lsStart));
			_hub->add_highlight_pause();
			_hub->add_highlight_blink(NAME(descriptionAndStart), NAME(lsCustomiseFirst));
			_hub->add_highlight_pause();
			_hub->add_highlight_infinite(NAME(descriptionAndStart), NAME(lsStart));
		}
	}
#ifndef ONE_CLICK_NEXT
	if (_whatToShow != WhatToShow::ShowAll)
	{
		Array<HubScreenButtonInfo> buttons;
		{
			if (_whatToShow > WhatToShow::SelectGameDefinition)
			{
				buttons.push_back(HubScreenButtonInfo(NAME(idPrev), NAME(lsPrev), [_onPrev]() { _onPrev(); }));
			}
			else
			{
				buttons.push_back(HubScreenButtonInfo());
			}
#ifndef ONE_CLICK_NEXT
			if (_whatToShow < WhatToShow::ShowSummary)
			{
				buttons.push_back(HubScreenButtonInfo(NAME(idNext), NAME(lsNext), [_onNextSelect]() { _onNextSelect(false); }));
			}
			else
			{
				buttons.push_back(HubScreenButtonInfo());
				//buttons.push_back(HubScreenButtonInfo(NAME(idNext), NAME(lsStart), [_onNextSelect]() { _onNextSelect(true); }));
			}
#endif
		}

		Rotator3 atDir = mainAtDir;
		Vector2 size((float)buttons.get_size() * 15.0f, 10.0f);
		if (topAt.is_set())
		{
			atDir.pitch = topAt.get() - size.y * 0.5f;
		}
		else
		{
			atDir.pitch += size.y * 0.5f;
		}

		HubScreen* s = new HubScreen(_hub, NAME(prevNextMenu), size, atDir, _hub->get_radius() * 0.6f, true, NP, ppa);

		Vector2 fs = HubScreen::s_fontSizeInPixels;

		float spacing = max(fs.x, fs.y);

		Range2 available = s->mainResolutionInPixels;
		available.expand_by(-Vector2::one * spacing);

		{
			Range2 at = available;
			//at.x.expand_by(-spacing * 4.0f);

			s->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1), at, Vector2::one * spacing);

			available.y.min = at.y.max + spacing;
		}

		s->activate();
		_hub->add_screen(s);

		topAt = atDir.pitch - size.y * 0.5f - spacingY;

		if (_whatToShow != WhatToShow::ShowSummary)
		{
			_hub->reset_highlights();
			_hub->add_highlight_blink(NAME(prevNextMenu), NAME(idNext));
			_hub->add_highlight_pause();
			_hub->add_highlight_infinite(NAME(prevNextMenu), NAME(idNext));
		}
	}
#endif

#ifndef ONE_CLICK_NEXT
	if (_whatToShow == WhatToShow::ShowAll ||
		_whatToShow == WhatToShow::ShowSummary)
#endif
	{
		Rotator3 atDir = mainAtDir;
		Vector2 size(30.0f, 10.0f);
#ifdef ONE_CLICK_NEXT
		{
			size.x += 15.0f;
		}
#endif
		if (topAt.is_set())
		{
			atDir.pitch = topAt.get() - size.y * 0.5f;
		}
		else
		{
			atDir.pitch += size.y * 0.5f;
		}

		HubScreen* s = new HubScreen(_hub, NAME(bottomMenu), size, atDir, _hub->get_radius() * 0.6f, true, NP, ppa);

		Vector2 fs = HubScreen::s_fontSizeInPixels;

		float spacing = max(fs.x, fs.y);

		Range2 available = s->mainResolutionInPixels;
		available.expand_by(-Vector2::one * spacing);

		{
			Array<HubScreenButtonInfo> buttons;
			Range2 at = available;
			//at.x.expand_by(-spacing * 4.0f);

#ifdef ONE_CLICK_NEXT
			bool allowBackExit = false;
			if (auto* ps = TeaForGodEmperor::PlayerSetup::access_current_if_exists())
			{
				if (ps->get_game_slots().get_size() > 1)
				{
					// we can remove this game slot and choose a different one
					allowBackExit = true;
				}
			}
			if ((_whatToShow > WhatToShow::SelectGameDefinition && ! TeaForGodEmperor::is_demo()) ||
				allowBackExit)
			{
				buttons.push_back(HubScreenButtonInfo(NAME(idPrev), NAME(lsPrev), [_onPrev]() { _onPrev(); }));
			}
			else
			{
				buttons.push_back(HubScreenButtonInfo());
			}
#endif
			buttons.push_back(HubScreenButtonInfo(NAME(idCalibrate), NAME(lsCalibrate), [s]() { s->hub->set_scene(new HubScenes::Calibrate()); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOptions), [s]() { s->hub->set_scene(new HubScenes::Options(HubScenes::Options::Main)); }));

			s->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1), at, Vector2::one * spacing);

			available.y.min = at.y.max + spacing;
		}

		s->activate();
		_hub->add_screen(s);

		topAt = atDir.pitch - size.y * 0.5f - spacingY;
	}

	if (gdsScreen)
	{
		gdsScreen->update_selection();
	}

	return gdsScreen;
}

GameDefinitionSelection::GameDefinitionSelection(Hub* _hub, Name const& _id, TeaForGodEmperor::PlayerGameSlot* _gameSlot,
	WhatToShow _whatToShow,
	std::function<void(bool _startImmediately)> _onNextSelect,
	Array<TeaForGodEmperor::GameDefinition*> const& _gameDefinitions, Array<TeaForGodEmperor::GameSubDefinition*> const& _gameSubDefinitions,
	Rotator3 const & _atDir, Vector2 const& _size, float _radius, Vector2 const& _ppa)
: HubScreen(_hub, Name::invalid(), _size, _atDir, _radius, true, NP, _ppa)
, gameSlot(_gameSlot)
, gameDefinitions(_gameDefinitions)
, gameSubDefinitions(_gameSubDefinitions)
{
	Vector2 fs = HubScreen::s_fontSizeInPixels;

	float spacing = max(fs.x, fs.y);

	Range2 available = mainResolutionInPixels;
	available.expand_by(-Vector2::one * spacing);

	gameDefinitionButtons.set_size(gameDefinitions.get_size());
	gameDefinitionMoreInfoButtons.set_size(gameDefinitions.get_size());
	gameSubDefinitionButtons.set_size(gameSubDefinitions.get_size());

	if (_whatToShow == WhatToShow::ShowAll ||
		_whatToShow == WhatToShow::SelectGameDefinition)
	{
		Range2 availableNow = available;
		if (_whatToShow == WhatToShow::ShowAll)
		{
			availableNow.y.min = availableNow.y.get_at(0.4f) + spacing;
			available.y.max = availableNow.y.min - spacing * 2.0f;
		}

		// top
		{
			Range2 eat = availableNow;
			auto* e = new HubWidgets::Text(Name::invalid(), eat, LOC_STR(NAME(lsGameDefinition)));
			e->alignPt.x = 0.0f;
			e->alignPt.y = 1.0f;
			add_widget(e);
			e->at.y.min = e->at.y.max - e->calculate_vertical_size();

			availableNow.y.max = e->at.y.min - spacing;
		}
		// bottom
		{
			Array<HubScreenButtonInfo> buttons;
			Range2 at = availableNow;
			at.y.max = at.y.min + fs.y * 1.5f;

			for_every_ptr(gd, gameDefinitions)
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), LOC_STR(NAME(lsMoreInfo)),
					[this, gd]() // on click
					{
						MessageSetup ms;
						ms.alignXPt = 0.0f;
						Message::show(hub, gd->get_desc_for_ui().get(), []() {}, ms);
					})
					.store_widget_in(&gameDefinitionMoreInfoButtons[for_everys_index(gd)])
					);
			}
			add_button_widgets_grid(buttons, VectorInt2(gameDefinitions.get_size(), 1), at, Vector2::one * spacing);

			// more info buttons should be a bit smaller
			for_every_ref(b, gameDefinitionMoreInfoButtons)
			{
				b->at.x.expand_by(-fs.x * 2.0f);
			}

			availableNow.y.min = at.y.max + spacing;
		}

		// middle
		{
			Array<HubScreenButtonInfo> buttons;
			Range2 at = availableNow;

			for_every_ptr(gd, gameDefinitions)
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), gd->get_desc_for_selection_ui().get(),
					[this, gd
#ifdef ONE_CLICK_NEXT
					, _onNextSelect
#endif
					]() // on click
					{
						select_game_definition(gd);
#ifdef ONE_CLICK_NEXT
						_onNextSelect(false /*irrelevant*/);
#endif
					})
					.store_widget_in(&gameDefinitionButtons[for_everys_index(gd)])
					);
			}
			add_button_widgets_grid(buttons, VectorInt2(gameDefinitions.get_size(), 1), at, Vector2::one * spacing);
		}
	}

	if (_whatToShow == WhatToShow::ShowAll ||
		_whatToShow == WhatToShow::SelectGameSubDefinition)
	{
		Range2 availableNow = available;

		{
			Range2 eat = availableNow;
			auto* e = new HubWidgets::Text(Name::invalid(), eat, LOC_STR(NAME(lsGameSubDefinition)));
			e->alignPt.x = 0.0f;
			e->alignPt.y = 1.0f;
			add_widget(e);
			e->at.y.min = e->at.y.max - e->calculate_vertical_size();

			availableNow.y.max = e->at.y.min - spacing;
		}

		{
			Array<HubScreenButtonInfo> buttons;
			Range2 at = availableNow;

			for_every_ptr(gsd, gameSubDefinitions)
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), gsd->get_desc_for_selection_ui().get(),
					[this, gsd
#ifdef ONE_CLICK_NEXT
					, _onNextSelect
#endif
					]() // on click
					{
						select_game_sub_definition(gsd);
#ifdef ONE_CLICK_NEXT
						_onNextSelect(false /*irrelevant*/);
#endif
					})
					.store_widget_in(&gameSubDefinitionButtons[for_everys_index(gsd)])
					);
			}
			add_button_widgets_grid(buttons, VectorInt2(gameSubDefinitions.get_size(), 1), at, Vector2::one* spacing);

			// this is not required, what for? availableNow.y.max = at.y.min - spacing;
		}
	}

	// store what's set in gameslot, so if we change one thing, the other stays as it was
	if (gameSlot.get())
	{
		selectedGameDefinition = cast_to_nonconst(gameSlot->get_game_definition());
		selectedGameSubDefinitions.clear();
		for_every_ref(gsd, gameSlot->get_game_sub_definitions())
		{
			selectedGameSubDefinitions.push_back(gsd);
		}
	}

	update_selection();
}

void GameDefinitionSelection::advance(float _deltaTime, bool _beyondModal)
{
	base::advance(_deltaTime, _beyondModal);

	if (auto* w = fast_cast<HubWidgets::Button>(get_widget(NAME(idCalibrate))))
	{
		float doorHeight = TeaForGodEmperor::PlayerPreferences::get_door_height_from_eye_level();

		w->set_highlighted(abs(TeaForGodEmperor::PlayerSetup::get_current().get_preferences().doorHeight - doorHeight) >= HubScenes::Calibrate::max_distance);
	}
}

void GameDefinitionSelection::select_game_definition(TeaForGodEmperor::GameDefinition* gd)
{
	selectedGameDefinition = gd;
	update_chosen_game_definition();
}

void GameDefinitionSelection::select_game_sub_definition(TeaForGodEmperor::GameSubDefinition* gsd)
{
	selectedGameSubDefinitions.clear();
	selectedGameSubDefinitions.push_back(gsd);
	update_chosen_game_definition();
}

void GameDefinitionSelection::update_chosen_game_definition()
{
	gameSlot->set_game_definition(selectedGameDefinition, selectedGameSubDefinitions, true /* if we're here, we most likely create a new game slot (or reset existing) */, true /* make it current */);
	update_selection();
}

void GameDefinitionSelection::update_selection()
{
	for_every_ref(w, gameDefinitionButtons)
	{
		if (auto* b = fast_cast<HubWidgets::Button>(w))
		{
			bool highlight = (gameSlot->get_game_definition() == gameDefinitions[for_everys_index(w)]);
			b->set_highlighted(highlight);
		}
	}
	for_every_ref(w, gameSubDefinitionButtons)
	{
		if (auto* b = fast_cast<HubWidgets::Button>(w))
		{
			bool highlight = false;
			auto* gsdButton = gameSubDefinitions[for_everys_index(w)];
			for_every_ref(gds, gameSlot->get_game_sub_definitions())
			{
				if (gds == gsdButton)
				{
					highlight = true;
					break;
				}
			}			
			b->set_highlighted(highlight);
		}
	}

	if (auto* w = fast_cast<HubWidgets::Text>(generalDescriptionWidget.get()))
	{
		String desc = String::empty();

		if (auto* gd = gameSlot->get_game_definition())
		{
			desc = gd->get_general_desc_for_ui(gameSlot->get_game_sub_definitions());
		}
		w->set(desc);
	}
}
