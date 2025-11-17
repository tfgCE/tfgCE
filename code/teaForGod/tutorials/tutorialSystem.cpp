#include "tutorialSystem.h"

#include "tutorial.h"

#include "..\game\game.h"
#include "..\loaders\hub\loaderHubScreen.h"
#include "..\loaders\hub\loaderHubWidget.h"
#include "..\loaders\hub\scenes\lhs_tutInGameMenu.h"
#include "..\loaders\hub\screens\lhc_tutInGameMenu.h"
#include "..\library\library.h"
#include "..\overlayInfo\overlayInfoElement.h"
#include "..\overlayInfo\overlayInfoSystem.h"

#include "..\..\framework\game\gameInput.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// game input definition
DEFINE_STATIC_NAME(tutorial);
DEFINE_STATIC_NAME(loaderHub);

// game input
DEFINE_STATIC_NAME(continue);
DEFINE_STATIC_NAME(useEquipment);

// loader hub input
DEFINE_STATIC_NAME(requestInGameMenu);
DEFINE_STATIC_NAME(requestInGameMenuHold);

// global references
DEFINE_STATIC_NAME_STR(highlightMaterial, TXT("tutorial; highlight material"));
DEFINE_STATIC_NAME_STR(tutorialStepSample, TXT("tutorial; step"));

//

using namespace TeaForGodEmperor;

TutorialSystem* TutorialSystem::s_tutorialSystem = nullptr;

TutorialSystem::TutorialSystem()
: gameSlot(new PlayerGameSlot())
, pilgrimSetup(new PilgrimSetup(&gameSlot->persistence))
{
	an_assert(! s_tutorialSystem);
	s_tutorialSystem = this;

	load_assets();
}

TutorialSystem::~TutorialSystem()
{
	an_assert(s_tutorialSystem == this);
	s_tutorialSystem = nullptr;

	delete_and_clear(pilgrimSetup);
	delete_and_clear(gameSlot);
}

void TutorialSystem::load_assets()
{
	/*
	highlightColour.parse_from_string(String(TXT("tutorial_highlight")));

	highlightMaterial = Library::get_current()->get_global_references().get<Framework::Material>(NAME(highlightMaterial));

	if (highlightMaterial.is_set())
	{
		for_count(int, materialUsageIdx, ::System::MaterialShaderUsage::Num)
		{
			highlightMaterialInstances[materialUsageIdx].set_material(highlightMaterial->get_material(), (::System::MaterialShaderUsage::Type)materialUsageIdx);
		}
	}

	tutorialStepSample = Library::get_current()->get_global_references().get<Framework::Sample>(NAME(tutorialStepSample));
	*/
}

void TutorialSystem::pause_tutorial()
{
	if (! paused)
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->supress();
		}
	}
	paused = true;
}

void TutorialSystem::unpause_tutorial()
{
	if (paused)
	{ 
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->resume();
		}
	}
	paused = false;
}

void TutorialSystem::repeat_tutorial()
{
	if (lastTutorial.is_set())
	{
		start_tutorial(lastTutorial.get());
	}
	else
	{
		end_tutorial();
	}
}

void TutorialSystem::start_tutorial(Tutorial const * _tutorial)
{
	an_assert(_tutorial);

	end_tutorial();

	tutorial = _tutorial;

	output(TXT("start tutorial \"%S\""), tutorial->get_name().to_string().to_char());

	PlayerSetup::access_current().set_last_tutorial_started(_tutorial->get_name());

	{	// store some stuff in case we modify it
		storedPlayerPreferences = PlayerSetup::get_current().get_preferences();
		storedGameSettingsSettings = GameSettings::get().settings;
		storedGameSettingsDifficulty = GameSettings::get().difficulty;
	}

	GameStats::use_tutorial();
	GameStats::get().reset(); // reset before setting any info
	GameStats::get().size = TXT("tutorial");
	GameStats::get().seed = tutorial->get_tutorial_tree_element_name().get();

	reset(); // back to default state

	if (auto* ois = OverlayInfo::System::get())
	{
		ois->mark_reset_anchor_placement();
	}

	tutorialInput.use(NAME(tutorial));
	tutorialInput.use(Framework::GameInputIncludeVR::All, true);
#ifdef AN_STANDARD_INPUT
	tutorialInputNoVR.use(NAME(tutorial));
	tutorialInputNoVR.use(Framework::GameInputIncludeVR::None);
	loaderHubInputNoVR.use(NAME(loaderHub));
	loaderHubInputNoVR.use(Framework::GameInputIncludeVR::None);
#endif

	// setup with tutorial
	gameSlot->persistence.copy_from(tutorial->get_persistence());
	pilgrimSetup->copy_from(tutorial->get_pilgrim_setup());

	scriptExecution.do_output_info(String(TXT("tutorial system")));
	scriptExecution.start(tutorial->get_game_script());

	if (auto* ois = OverlayInfo::System::get())
	{
		ois->force_clear();
	}
}

void TutorialSystem::end_tutorial()
{
	if (! tutorial.is_set())
	{
		return;
	}

	output(TXT("end tutorial \"%S\""), tutorial->get_name().to_string().to_char());

	// to not mess up when loading screens etc
	clear_highlights();
	clear_hub_highlights();

	// store as last one
	lastTutorial = tutorial;

	tutorial.clear();

	if (auto* ois = OverlayInfo::System::get())
	{
		ois->force_clear();
	}

	{	// restore what was before we started a tutorial
		PlayerSetup::access_current().access_preferences() = storedPlayerPreferences;
		GameSettings::get().settings = storedGameSettingsSettings;
		GameSettings::get().difficulty = storedGameSettingsDifficulty;
		TeaForGodEmperor::GameSettings::get().inform_observers();
	}
}

void TutorialSystem::reset()
{
	scriptExecution.access_variables().clear();
	delete_and_clear(pilgrimSetup);
	delete_and_clear(gameSlot);
	gameSlot = new PlayerGameSlot();
	pilgrimSetup = new PilgrimSetup(&gameSlot->persistence);
	// highlight
	clear_highlights();
	clear_hub_highlights();
	// various
	forcedHandDisplaysState.clear();
	allowPickupOrbs = true;
	allowPhysicalViolence = true;
	allowEnergyTransfer = true;
	// input
	blockAllInput = false;
	blockedInputs.clear();
	allowedInputs.clear();
	// hub input
	blockHubInput = false;
	allowOverWidgetExplicit = false;
	// other
	inGameMenuShouldNotSupressOverlay = false;
	forcedSuccess = false;
	useNormalInGameMenu = false;
}

void TutorialSystem::advance()
{
	if (!is_active())
	{
		return;
	}

	scoped_call_stack_info(TXT("tutorial system advance"));
	scoped_call_stack_info(tutorial.get() ? tutorial->get_name().get_name().to_char() : TXT("<no tutorial>"));

	forceRedrawFor = max(0.0f, forceRedrawFor - ::System::Core::get_delta_time());

	float continueButtonState = tutorialInput.get_button_state(NAME(continue));
#ifdef AN_STANDARD_INPUT
	if (!VR::IVR::get())
	{
		continueButtonState = max(continueButtonState, tutorialInputNoVR.get_button_state(NAME(continue)));
	}
#endif

	if (blockInputForContinue)
	{
		stillBlockingInputForContinue = true;
	}
	else if (!blockInputForContinue && stillBlockingInputForContinue)
	{
		if (continueButtonState < hardcoded magic_number 0.2f)
		{
			stillBlockingInputForContinue = false;
		}
	}

	blockContinueForRestOfTheFrame = false;
	if (auto* game = Game::get_as<Game>())
	{
		if (game->get_fade_out() > 0.5f || paused)
		{
			playerWantedToContinue = false;
			playerWantsToContinue = false;
		}
		else
		{
			playerWantedToContinue = playerWantsToContinue;
			playerWantsToContinue = continueButtonState > hardcoded magic_number 0.5f;
		}
	}

	if (activeHub)
	{
		if (!hubInGameMenu.is_set())
		{
			bool requestsInGameMenu = false;
#ifdef AN_STANDARD_INPUT
			requestsInGameMenu |= loaderHubInputNoVR.has_button_been_pressed(NAME(requestInGameMenu));
			requestsInGameMenu |= loaderHubInputNoVR.has_button_been_pressed(NAME(requestInGameMenuHold));
#endif
			for_count(int, hIdx, Hand::MAX)
			{
				Hand::Type hand = (Hand::Type)hIdx;
				requestsInGameMenu |= activeHub->get_hand(hand).input.has_button_been_pressed(NAME(requestInGameMenu));
				requestsInGameMenu |= activeHub->get_hand(hand).input.has_button_been_pressed(NAME(requestInGameMenuHold));
			}

			if (requestsInGameMenu)
			{
				hubInGameMenu = Loader::HubScreens::TutInGameMenu::show_for_hub(activeHub);
				if (auto* ois = OverlayInfo::System::get())
				{
					OverlayInfo::System::get()->supress();
				}
			}
		}

		if (auto* tigm = fast_cast<Loader::HubScreens::TutInGameMenu>(hubInGameMenu.get()))
		{
			if (tigm->is_active())
			{
				return;
			}
			else
			{
				if (auto* ois = OverlayInfo::System::get())
				{
					OverlayInfo::System::get()->resume();
				}
				hubInGameMenu.clear();
			}
		}
	}

	if (paused)
	{
		return;
	}

	scriptExecution.execute();

	// in case someone has not cleared it
	clear_clicked_hub();
}

//

float TutorialSystem::get_filtered_button_state(Framework::GameInput const& _controls, Name const& _button)
{
	float buttonState = _controls.get_button_state(_button);
	if (s_tutorialSystem && s_tutorialSystem->is_active() && ! s_tutorialSystem->is_tutorial_paused())
	{
		return s_tutorialSystem->filter_input(_button, buttonState, 0.0f);
	}
	else
	{
		return buttonState;
	}
}

Vector2 TutorialSystem::get_filtered_stick(Framework::GameInput const& _controls, Name const& _stick)
{
	Vector2 stickState = _controls.get_stick(_stick);
	if (s_tutorialSystem && s_tutorialSystem->is_active() && !s_tutorialSystem->is_tutorial_paused())
	{
		return s_tutorialSystem->filter_input(_stick, stickState, Vector2::zero);
	}
	else
	{
		return stickState;
	}
}

bool TutorialSystem::filter_input(Name const& _input, bool _value)
{
	if (s_tutorialSystem && s_tutorialSystem->is_active() && !s_tutorialSystem->is_tutorial_paused())
	{
		return s_tutorialSystem->filter_input(_input, _value, false);
	}
	else
	{
		return _value;
	}
}

template <typename InputType>
InputType const& TutorialSystem::filter_input(Name const& _input, InputType const& _inputValue, InputType const& _noInputValue) const
{
	bool blocked = blockAllInput;
	if (!blocked && blockedInputs.does_contain(_input))
	{
		blocked = true;
	}
	if (blocked && allowedInputs.does_contain(_input))
	{
		blocked = false;
	}
	// this is the most likely input that is going to interfere with continue gesture, it is hardcoded now
	if (stillBlockingInputForContinue && _input == hardcoded NAME(useEquipment))
	{
		blocked = true;
	}
	return blocked ? _noInputValue : _inputValue;
}

void TutorialSystem::block_all_input()
{
	blockAllInput = true;
	blockedInputs.clear();
	allowedInputs.clear();
}

void TutorialSystem::allow_all_input()
{
	blockAllInput = false;
	blockedInputs.clear();
	allowedInputs.clear();
}

void TutorialSystem::block_input(Name const& _input)
{
	blockedInputs.push_back_unique(_input);
	allowedInputs.remove_fast(_input);
}

void TutorialSystem::allow_input(Name const& _input)
{
	allowedInputs.push_back_unique(_input);
	blockedInputs.remove_fast(_input);
}

void TutorialSystem::block_input_for_continue()
{
	blockInputForContinue = true;
	stillBlockingInputForContinue = true;
}

void TutorialSystem::allow_input_after_continue()
{
	blockInputForContinue = false;
}

void TutorialSystem::configure_oie_element(OverlayInfo::Element* _element, Rotator3 const & _offset)
{
	_element->with_location(OverlayInfo::Element::Relativity::RelativeToAnchor);
	_element->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchor, _offset);
	_element->with_distance(10.0f);
}

Loader::HubScene* TutorialSystem::create_in_game_menu_scene() const
{
	auto* scene = new Loader::HubScenes::TutInGameMenu();
	return scene;
}

bool TutorialSystem::is_hub_input_allowed()
{
	if (s_tutorialSystem && s_tutorialSystem->is_active())
	{
		if (auto* tigm = s_tutorialSystem->hubInGameMenu.get())
		{
			return tigm->is_active();
		}
		if (!s_tutorialSystem->is_tutorial_paused())
		{
			return !s_tutorialSystem->blockHubInput;
		}
	}
	return true;
}

bool TutorialSystem::is_hub_over_widget_allowed()
{
	if (s_tutorialSystem && s_tutorialSystem->is_active())
	{
		if (!s_tutorialSystem->is_tutorial_paused())
		{
			return s_tutorialSystem->allowOverWidgetExplicit;
		}
	}
	return true;
}

void TutorialSystem::block_hub_input()
{
	blockHubInput = true;
}

void TutorialSystem::allow_hub_input()
{
	blockHubInput = false;
}

void TutorialSystem::restore_over_widget_explicit()
{
	allowOverWidgetExplicit = false;
}

void TutorialSystem::allow_over_widget_explicit()
{
	allowOverWidgetExplicit = true;
}

Tutorial const* TutorialSystem::get_next_tutorial() const
{
	an_assert(!flatTree.is_empty());

	int found = NONE;
	for_every_ptr(tte, flatTree)
	{
		if (tte == lastTutorial.get())
		{
			found = for_everys_index(tte);
		}
	}
	if (found == NONE)
	{
		found = 0;
	}
	else
	{
		++found;
	}

	if (found >= flatTree.get_size())
	{
		found = 0;
	}

	while (!fast_cast<Tutorial>(flatTree[found]))
	{
		++found;
		if (found >= flatTree.get_size())
		{
			found = 0;
		}
	}

	an_assert(fast_cast<Tutorial>(flatTree[found]));
	return fast_cast<Tutorial>(flatTree[found]);
}

void TutorialSystem::build_tree()
{
	flatTree.clear();
	tree = TutorialTreeElement::build(TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>());
	if (tree)
	{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
#ifdef AN_DEVELOPMENT_OR_PROFILER
		output_colour(0, 1, 0, 0);
		output(TXT("tutorial system tree"));
		output_colour();
		tree->store_flat_tree(OUT_ flatTree);
#endif
#endif
	}
}

bool TutorialSystem::check_highlighted(Name const& _what)
{
	if (s_tutorialSystem && s_tutorialSystem->is_active())
	{
		return s_tutorialSystem->is_highlighted(_what);
	}
	return false;
}

void TutorialSystem::highlight(Name const& _what)
{
	Concurrency::ScopedMRSWLockWrite lock(highlightedLock);
	highlighted.set_tag(_what);
}

void TutorialSystem::clear_highlights()
{
	Concurrency::ScopedMRSWLockWrite lock(highlightedLock);
	highlighted.clear();
}

void TutorialSystem::clear_highlight(Name const& _what)
{
	Concurrency::ScopedMRSWLockWrite lock(highlightedLock);
	highlighted.remove_tag(_what);
}

bool TutorialSystem::is_highlighted(Name const& _what) const
{
	Concurrency::ScopedMRSWLockRead lock(highlightedLock);
	return highlighted.get_tag(_what) != 0.0f;;
}

bool TutorialSystem::should_be_highlighted_now()
{
	if (s_tutorialSystem && s_tutorialSystem->is_active() && !s_tutorialSystem->highlighted.is_empty())
	{
		float period = 1.0f;
		return ::System::Core::get_timer_mod(period) < period * 0.3f;
	}
	else
	{
		return false;
	}
}

::System::MaterialInstance const& TutorialSystem::get_highlight_material_instance(::System::MaterialShaderUsage::Type _materialUsage) const
{
	return highlightMaterialInstances[_materialUsage];
}

::System::MaterialInstance const& TutorialSystem::get_highlight_material_instance(::Meshes::Usage::Type _meshUsage) const
{
	return get_highlight_material_instance(::Meshes::Usage::usage_to_material_usage(_meshUsage));
}

Colour const& TutorialSystem::highlight_colour()
{
	return s_tutorialSystem? s_tutorialSystem->highlightColour : Colour::white;
}

bool TutorialSystem::does_allow_interaction(Loader::HubScreen const* _screen, Loader::HubWidget const* _widget, Loader::HubDraggable const* _dragging)
{
	if (s_tutorialSystem && s_tutorialSystem->is_active() && !s_tutorialSystem->is_tutorial_paused())
	{
		if (auto* tigm = s_tutorialSystem->hubInGameMenu.get())
		{
			return tigm == _screen;
		}
		Concurrency::ScopedMRSWLockRead lock(s_tutorialSystem->hubHighlightedLock);
		if (s_tutorialSystem->hubHighlighted.is_empty())
		{
			if (! s_tutorialSystem->forceHubHighlights)
			{
				return true;
			}
		}
		for_every(h, s_tutorialSystem->hubHighlighted)
		{
			if ((!h->noInteractions || (h->alwaysAllowDrops && _dragging)) &&
				(!h->screen.is_valid() || !_screen || _screen->id == h->screen) &&
				(!h->widget.is_valid() || !_widget || _widget->id == h->widget))
			{
				return true;
			}
		}
		return false;
	}
	return true;
}

bool TutorialSystem::can_be_dragged(Loader::HubScreen const* _screen, Loader::HubWidget const* _widget, Loader::HubDraggable const* _draggable)
{
	if (s_tutorialSystem && s_tutorialSystem->is_active() && !s_tutorialSystem->is_tutorial_paused())
	{
		TutorialHubId draggable;
		if (_draggable)
		{
			if (auto* d = _draggable->data.get())
			{
				draggable = d->tutorialHubId;
			}
		}
		Concurrency::ScopedMRSWLockRead lock(s_tutorialSystem->hubHighlightedLock);
		if (s_tutorialSystem->hubHighlighted.is_empty())
		{
			if (!s_tutorialSystem->forceHubHighlights)
			{
				return true;
			}
		}
		for_every(h, s_tutorialSystem->hubHighlighted)
		{
			if (!h->noInteractions &&
				(!h->screen.is_valid() || !_screen || _screen->id == h->screen) &&
				(!h->widget.is_valid() || !_widget || _widget->id == h->widget) &&
				(!h->draggable.is_set() || draggable == h->draggable))
			{
				return true;
			}
		}
		return false;
	}
	return true;
}

bool TutorialSystem::should_redraw_hub_for(Loader::HubScreen const* _screen, Loader::HubWidget const* _widget)
{
	if (s_tutorialSystem && s_tutorialSystem->is_active() && !s_tutorialSystem->is_tutorial_paused())
	{
		if (s_tutorialSystem->forceRedrawFor > 0.0f)
		{
			return true;
		}
		Concurrency::ScopedMRSWLockRead lock(s_tutorialSystem->hubHighlightedLock);
		if (s_tutorialSystem->hubHighlighted.is_empty())
		{
			if (!s_tutorialSystem->forceHubHighlights)
			{
				return false;
			}
		}
		for_every(h, s_tutorialSystem->hubHighlighted)
		{
			if ((!h->screen.is_valid() || _screen->id == h->screen) &&
				(!h->widget.is_valid() || _widget->id == h->widget))
			{
				return true;
			}
		}
		return false;
	}
	return false;
}

Colour TutorialSystem::adjust_colour_for_hub(Colour const& _colour, Loader::HubScreen const * _screen, Loader::HubWidget const* _widget, Loader::HubDraggable const* _draggable)
{
	if (s_tutorialSystem)
	{
		bool dim = false;
		if (s_tutorialSystem->is_active() && !s_tutorialSystem->is_tutorial_paused())
		{
			dim = true;
			if (auto* tigm = s_tutorialSystem->hubInGameMenu.get())
			{
				if (tigm == _screen)
				{
					return _colour;
				}
			}
			else
			{
				TutorialHubId draggable;
				if (_draggable)
				{
					if (auto* d = _draggable->data.get())
					{
						draggable = d->tutorialHubId;
					}
				}
				Concurrency::ScopedMRSWLockRead lock(s_tutorialSystem->hubHighlightedLock);
				if (s_tutorialSystem->hubHighlighted.is_empty())
				{
					if (!s_tutorialSystem->forceHubHighlights)
					{
						return _colour;
					}
				}
				for_every(h, s_tutorialSystem->hubHighlighted)
				{
					if ((!h->screen.is_valid() || _screen->id == h->screen) &&
						(!h->widget.is_valid() || _widget->id == h->widget) &&
						(!h->draggable.is_set() || h->draggable == draggable))
					{
						if (h->widget.is_valid() && !h->noPulse)
						{
							float period = 1.0f;
							return ::System::Core::get_timer_mod(period) < period * 0.3f ? Colour::lerp(0.8f, _colour, s_tutorialSystem->highlightColour) : _colour;
						}
						else
						{
							// highlight whole screen, do not pulse
							return _colour;
						}
					}
				}
			}
		}
		else if (!s_tutorialSystem->is_active() && s_tutorialSystem->is_game_in_tutorial_mode())
		{
			// we just ended tutorial, dim everything
			dim = true;
		}
		if (dim)
		{
			return Colour::lerp(0.5f, Loader::HubColours::screen_interior(), _colour);
		}
	}
	return _colour;
}

void TutorialSystem::clear_hub_highlights()
{
	Concurrency::ScopedMRSWLockWrite lock(hubHighlightedLock);
	hubHighlighted.clear();
	forceRedrawFor = 0.2f;
}

void TutorialSystem::hub_highlight(Name const& _screen, Name const& _widget, TutorialHubId const & _draggable, bool _noInteractions, bool _alwaysAllowDrops, bool _noPulse)
{
	Concurrency::ScopedMRSWLockWrite lock(hubHighlightedLock);
	hubHighlighted.push_back_unique(HubScreenWidget(_screen, _widget, _draggable, _noInteractions, _alwaysAllowDrops, _noPulse));
	forceRedrawFor = 0.2f;
}

void TutorialSystem::clear_hub_highlight(Name const& _screen, Name const& _widget, TutorialHubId const& _draggable)
{
	Concurrency::ScopedMRSWLockWrite lock(hubHighlightedLock);
	hubHighlighted.remove(HubScreenWidget(_screen, _widget, _draggable));
	forceRedrawFor = 0.2f;
}

void TutorialSystem::mark_clicked_hub(Loader::HubScreen const* _screen, Loader::HubWidget const* _widget)
{
	mark_clicked_hub(_screen ? _screen->id : Name::invalid(), _widget ? _widget->id : Name::invalid());
}

void TutorialSystem::mark_clicked_hub(Name const& _screen, Name const& _widget)
{
	if (s_tutorialSystem)
	{
		s_tutorialSystem->hubClicked = true;
		s_tutorialSystem->hubClickedWhat = HubScreenWidget(_screen, _widget);
	}
}

bool TutorialSystem::has_clicked_hub(Name const& _screen, Name const& _widget)
{
	if (s_tutorialSystem)
	{
		return s_tutorialSystem->hubClicked &&
			   (!_screen.is_valid() || s_tutorialSystem->hubClickedWhat.screen == _screen) &&
			   (!_widget.is_valid() || s_tutorialSystem->hubClickedWhat.widget == _widget);
	}
	return false;
}

void TutorialSystem::clear_clicked_hub()
{
	if (s_tutorialSystem)
	{
		s_tutorialSystem->hubClicked = false;
		s_tutorialSystem->hubClickedWhat = HubScreenWidget();
	}
}

void TutorialSystem::set_active_hub(Loader::Hub* _hub)
{
	if (s_tutorialSystem)
	{
		s_tutorialSystem->activeHub = _hub;
	}
}

Loader::Hub* TutorialSystem::get_active_hub()
{
	if (s_tutorialSystem)
	{
		return s_tutorialSystem->activeHub;
	}
	return nullptr;
}

void TutorialSystem::on_tutorial_step()
{
	if (auto* s = tutorialStepSample.get())
	{
		if (auto* ss = s->get_sample())
		{
			ss->play();
		}
	}
}
