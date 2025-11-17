#pragma once

#include "tutorialHubId.h"

#include "..\game\gameSettings.h"
#include "..\game\playerSetup.h"
#include "..\pilgrim\pilgrimSetup.h"

#include "..\..\core\mesh\usage.h"
#include "..\..\core\system\video\materialInstance.h"
#include "..\..\core\tags\tag.h"
#include "..\..\framework\game\gameInput.h"
#include "..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\library\usedLibraryStored.h"

#define RETURN_IF_IN_TUTORIAL \
	if (TutorialSystem::get() && TutorialSystem::get()->is_active()) \
	{ \
		return; \
	}

namespace Framework
{
	class GameInput;
	class Material;
	class Sample;
};

namespace Loader
{
	class Hub;
	class HubScene;
	struct HubScreen;
	struct HubWidget;
	struct HubDraggable;
};

namespace TeaForGodEmperor
{
	class Tutorial;
	class TutorialTreeElement;

	namespace OverlayInfo
	{
		struct Element;
	};

	class TutorialSystem
	{
	public:
		static TutorialSystem* get() { return s_tutorialSystem; }

		TutorialSystem();
		~TutorialSystem();

		static bool check_active() { return s_tutorialSystem && s_tutorialSystem->is_active(); }

		static Colour const & highlight_colour();

		bool is_active() const { return tutorial.is_set(); }

		bool is_game_in_tutorial_mode() const { return gameInTutorialMode; }
		void set_game_in_tutorial_mode(bool _gameInTutorialMode) { gameInTutorialMode = _gameInTutorialMode; }

		void load_assets();

		Framework::GameScript::ScriptExecution& access_script_execution() { return scriptExecution; }

		void start_tutorial(Tutorial const* _tutorial); // this still requires setting meta state to game to launch actual tutorial
		void end_tutorial(); // after that tutorial game mode will request post game summary which will end game mode and will end up in tutorial summary
		void repeat_tutorial();
		Tutorial const* get_last_tutorial() const { return lastTutorial.get(); }
		void unset_last_tutorial() { lastTutorial.clear(); }

		void on_tutorial_step();

		Tutorial const* get_next_tutorial() const;

		void pause_tutorial();
		void unpause_tutorial();
		bool is_tutorial_paused() const { return paused; }

		Tutorial const* get_tutorial() const { return tutorial.get(); }

		PilgrimSetup& access_pilgrim_setup() { return *pilgrimSetup; }
		PilgrimSetup const& get_pilgrim_setup() const { return *pilgrimSetup; }

		PlayerGameSlot* access_game_slot() { return gameSlot; }
		PlayerGameSlot const* get_game_slot() const { return gameSlot; }

		void reset();

		void advance();

		Loader::HubScene* create_in_game_menu_scene() const;
		
		void in_game_menu_should_not_supress_overlay() { inGameMenuShouldNotSupressOverlay = true; }
		bool does_in_game_menu_supress_overlay() const { return ! inGameMenuShouldNotSupressOverlay; }
		
		bool should_open_normal_in_game_menu() const { return useNormalInGameMenu; }
		void open_normal_in_game_menu_if_requested(bool _useNormalInGameMenu) { useNormalInGameMenu = _useNormalInGameMenu; }

		void force_success() { forcedSuccess = true; }
		bool is_success_forced() const { return forcedSuccess; }

	// interaction with hub
	public:
		static void mark_clicked_hub(Loader::HubScreen const * _screen, Loader::HubWidget const * _widget);
		static void mark_clicked_hub(Name const& _screen, Name const& _widget);
		static bool has_clicked_hub(Name const& _screen, Name const& _widget);
		static void clear_clicked_hub();
		static void set_active_hub(Loader::Hub* _hub);
		static Loader::Hub* get_active_hub();

	public:
		struct HubScreenWidget
		{
			Name screen;
			Name widget;
			TutorialHubId draggable;
			bool noInteractions = false; // not compared
			bool alwaysAllowDrops = true;
			bool noPulse = false;
			HubScreenWidget() {}
			HubScreenWidget(Name const& _screen, Name const& _widget, TutorialHubId const& _draggable = TutorialHubId(), bool _noInteractions = false, bool _alwaysAllowDrops = false, bool _noPulse = false) : screen(_screen), widget(_widget), draggable(_draggable), noInteractions(_noInteractions), alwaysAllowDrops(_alwaysAllowDrops), noPulse(_noPulse) {}
			bool operator==(HubScreenWidget const& _o) const { return screen == _o.screen && widget == _o.widget && draggable == _o.draggable; }
		};

		bool hubClicked = false;
		HubScreenWidget hubClickedWhat;
		Loader::Hub* activeHub = nullptr;

	public:
		static void configure_oie_element(OverlayInfo::Element* _element, Rotator3 const& _offset);

	// tree
	public: 
		void build_tree();
		TutorialTreeElement* get_tree() const { return tree; }
		Array<TutorialTreeElement*> const & get_flat_tree() const { return flatTree; }

	// various
	public:
		Optional<bool> const & get_forced_hand_displays_state() const { return forcedHandDisplaysState; }
		Optional<bool> & access_forced_hand_displays_state() { return forcedHandDisplaysState; }

		bool should_allow_pickup_orbs() const { return allowPickupOrbs; }
		void set_allow_pickup_orbs(bool _allowPickupOrbs) { allowPickupOrbs = _allowPickupOrbs; }

		bool is_physical_violence_allowed() const { return allowPhysicalViolence; }
		void set_allow_physical_violence(bool _allowPhysicalViolence) { allowPhysicalViolence = _allowPhysicalViolence; }

		bool is_energy_transfer_allowed() const { return allowEnergyTransfer; }
		void set_allow_energy_transfer(bool _allowEnergyTransfer) { allowEnergyTransfer = _allowEnergyTransfer; }

	private:
		// remember about reset()
		Optional<bool> forcedHandDisplaysState;
		bool allowPickupOrbs = true;
		bool allowPhysicalViolence = true;
		bool allowEnergyTransfer = true;

	// hub highlighting
	public:
		static bool does_allow_interaction(Loader::HubScreen const* _screen, Loader::HubWidget const* _widget, Loader::HubDraggable const* _dragging = nullptr);
		static bool can_be_dragged(Loader::HubScreen const* _screen, Loader::HubWidget const* _widget, Loader::HubDraggable const* _draggable);
		static Colour adjust_colour_for_hub(Colour const& _colour, Loader::HubScreen const* _screen, Loader::HubWidget const* _widget, Loader::HubDraggable const* _draggable = nullptr);
		static bool should_redraw_hub_for(Loader::HubScreen const* _screen, Loader::HubWidget const* _widget);
		void clear_hub_highlights();
		void hub_highlight(Name const & _screen, Name const & _widget, TutorialHubId const & _draggable = TutorialHubId(), bool _noInteractions = false, bool _alwaysAllowDrops = false, bool _noPulse = false);
		void clear_hub_highlight(Name const& _screen, Name const& _widget, TutorialHubId const& _draggable = TutorialHubId());
		void force_hub_highlights(bool _force) { forceHubHighlights = _force; }

	private:
		mutable Concurrency::MRSWLock hubHighlightedLock;
		Array<HubScreenWidget> hubHighlighted;
		bool forceHubHighlights = false; // this will mean that highlighting is on, even if nothing is to be highlighted
		float forceRedrawFor = 0.0f;

	// highlighting
	public:
		static bool check_highlighted(Name const& _what);
		void highlight(Name const& _what);
		void clear_highlights();
		void clear_highlight(Name const& _what);
		bool is_highlighted(Name const& _what) const;
		static bool should_be_highlighted_now();
		::System::MaterialInstance const & get_highlight_material_instance(::System::MaterialShaderUsage::Type _materialUsage) const;
		::System::MaterialInstance const & get_highlight_material_instance(::Meshes::Usage::Type _meshUsage) const;

	private:
		mutable Concurrency::MRSWLock highlightedLock;
		Tags highlighted;
		Framework::UsedLibraryStored<Framework::Material> highlightMaterial;
		::System::MaterialInstance highlightMaterialInstances[::System::MaterialShaderUsage::Num];
		Colour highlightColour = Colour::white;;
			
	// input
	public:
		static float get_filtered_button_state(Framework::GameInput const& _controls, Name const& _button);
		static Vector2 get_filtered_stick(Framework::GameInput const& _controls, Name const& _stick);
		static bool filter_input(Name const& _input, bool _value);

		void block_all_input();
		void allow_all_input();
		void block_input(Name const & _input);
		void allow_input(Name const & _input);

		// continue part of input
		void block_input_for_continue();
		void allow_input_after_continue();
		bool has_continue_been_triggered() const { return playerWantsToContinue && !playerWantedToContinue && !blockContinueForRestOfTheFrame; }
		void consume_continue() { blockContinueForRestOfTheFrame = true; }

	private:
		bool blockInputForContinue = false;
		bool stillBlockingInputForContinue = false; // when input is cleared, this is cleared
		bool blockAllInput = false;
		Array<Name> blockedInputs;
		Array<Name> allowedInputs;

		bool playerWantsToContinue = false;
		bool playerWantedToContinue = false;
		bool blockContinueForRestOfTheFrame = false;

		template <typename InputType>
		InputType const & filter_input(Name const& _input, InputType const & _inputValue, InputType const & _noInputValue) const;

	// hub input
	public:
		static bool is_hub_input_allowed();
		static bool is_hub_over_widget_allowed(); // if input not allowed, we may still want to allow "over widget"

		void block_hub_input();
		void allow_hub_input();

		void restore_over_widget_explicit();
		void allow_over_widget_explicit();

	private:
		bool blockHubInput = false;
		bool allowOverWidgetExplicit = false; // if hub input not allowed

	private:
		static TutorialSystem* s_tutorialSystem;

		Framework::GameInput tutorialInput;
#ifdef AN_STANDARD_INPUT
		Framework::GameInput tutorialInputNoVR;
		Framework::GameInput loaderHubInputNoVR;
#endif
		bool paused = false;
		bool gameInTutorialMode = false;

		RefCountObjectPtr<Loader::HubScreen> hubInGameMenu;

		Framework::UsedLibraryStored<Tutorial> tutorial;
		Framework::UsedLibraryStored<Tutorial> lastTutorial;
		Framework::UsedLibraryStored<Framework::Sample> tutorialStepSample;

		Framework::GameScript::ScriptExecution scriptExecution;

		PlayerGameSlot* gameSlot = nullptr;
		PilgrimSetup* pilgrimSetup = nullptr;
		PlayerPreferences storedPlayerPreferences; // stored for tutorial as those can be modified
		GameSettings::Settings storedGameSettingsSettings;
		GameSettings::Difficulty storedGameSettingsDifficulty;

		TutorialTreeElement* tree = nullptr;
		Array<TutorialTreeElement*> flatTree;

		bool inGameMenuShouldNotSupressOverlay = false;
		bool forcedSuccess = false;
		bool useNormalInGameMenu = false;
	};
};
