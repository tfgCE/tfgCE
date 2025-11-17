#pragma once

#include "displayTypes.h"

#include "displayType.h"

#include "..\library\libraryName.h"
#include "..\library\usedLibraryStored.h"
#include "..\postProcess\postProcess.h"
#include "..\postProcess\graph\postProcessGraphProcessingInputRenderTargetsProvider.h"
#include "..\postProcess\graph\postProcessGraphProcessingResolutionProvider.h"
#include "..\sound\soundSources.h"

#include "..\..\core\containers\list.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\random\random.h"
#include "..\..\core\system\video\videoFormat.h"

// lockless - will use gathering during advancement and will put that into actual rendering at some point
#define DISPLAY_LOCKLESS

namespace System
{
	class RenderTarget;
	class Texture;
};

namespace Framework
{
	interface_class IGameInput;
	class Font;
	class Library;
	class PostProcessChain;
	class PostProcessChainProcessElement;
	class PostProcessRenderTargetManager;
};

namespace Framework
{
	struct Display_DeleteRendering;
	class Display;
	class DisplayControl;
	class DisplayButton;
	class DisplayTable;
	class DisplayType;

	struct DisplayVRSceneContext
	{
		Vector3 displayInDir = Vector3::zero;
	};

	struct DisplayInitContext
	{
		Optional<::System::VideoFormat::Type> outputTexture;
		Optional<Colour> clearColour;
	};

	/**
	 *	Display of computers.
	 *	This can be used for anything form hmd through in-game computers and interfaces to game menus.
	 */
	class Display
	: public RefCountObject
	{
	public:
		typedef std::function<void(Display* _display, float _deltaTime)> OnAdvanceDisplay;
		typedef std::function<void(Display* _display)> OnUpdateDisplay;
		typedef std::function<void(Display* _display)> OnRenderDisplay;
		typedef std::function<void(DisplayControl* _control, Vector2 const & _cursorAt, bool _clicked)> OnClick; // clicked or released

	public:
		Display();
		virtual ~Display();

	public: // setup and init
		void use_meshes(bool _useMeshes) { useMeshes = _useMeshes; }
		void ignore_game_frames() { ignoreGameFrames = true; }
		void setup_with(DisplayType * _displayType);
		void setup_output_size(VectorInt2 const & _outputSize);
		void setup_output_size_as_is();
		void setup_output_size_min(VectorInt2 const & _spaceAvailable, float _sizeMultiplier); // will choose the smaller out of two dimensions (x,y), means that
		void setup_output_size_max(VectorInt2 const & _spaceAvailable, float _maxSizeMultiplier); // will choose the bigger out of two dimensions (x,y), means that will fit into both
		void setup_output_size_limit(VectorInt2 const & _spaceAvailable);
		void setup_resolution_with_borders(VectorInt2 const & _screenResolution, VectorInt2 const & _border);
		void use_setup(DisplaySetup const & _displaySetup);
		void use_font(LibraryName const & _fontName);
		void use_post_process(LibraryName const & _postProcess);
		void use_render_target_manager(PostProcessRenderTargetManager* _postProcessRenderTargetManager);

		void set_use_coordinates(DisplayCoordinates::Type _useCoordinates) { useCoordinates = _useCoordinates; }
		DisplayCoordinates::Type get_use_coordinates() const { return useCoordinates; }

		void init(Library * _library, Optional<DisplayInitContext> const & _initContext = NP, LibraryName const & _meshNameForReusing = LibraryName::invalid(), DisplayHardwareMeshSetup* _overrideHardwareMeshSetup = nullptr);
		bool is_initialised() const { return initialised; }

		void loop_navigation(bool _loopedNavigation = true) { setup.loopedNavigation = _loopedNavigation; }
		bool is_navigation_looped() const { return setup.loopedNavigation; }

		float calc_device_mesh_3d_scale(Vector2 const & _desiredMinSize, Vector2 const & _desiredMaxSize = Vector2::zero);

		// this is better to be used during initialisation
		void set_retrace_at_pct(float _retraceAtPct);

	public:
		bool has_crashed() const { return hasCrashed; }
		int get_crashed_code() const { return crashedCode; }
		void clear_crashed() { hasCrashed = false; }
		void crash(int _code) { hasCrashed = true; crashedCode = _code; }

	public: // sounds
		Sample* find_sound_sample(Name const & _name) const;
		SoundSource* play_sound(Name const & _name);
		SoundSources & access_sounds() { return soundSources; }

	public: // running
		bool is_updated_continuously() const;
		void resume(); // if we were not advancing for a while

		void advance_sounds(float _deltaTime); // should be done before sounds are used for sound scene
		void advance(float _deltaTime); // can be done on any thread

		void set_rotate_display(int _rotateDisplay = 0);

		void set_on_advance_display(void const * _owner, OnAdvanceDisplay _on_advance_display);
		void set_on_update_display(void const * _owner, OnUpdateDisplay _on_update_display);
		void set_on_render_display(void const * _owner, OnRenderDisplay _on_render_display);
		void set_on_click(void const * _owner, OnClick _handle_click);

		void update_display(); // can be done on rendering thread only
		void ready_mesh_3d_instance_to_render(Meshes::Mesh3DInstance & _meshInstance); // update material/texture inside

		::System::RenderTarget* get_current_frame_render_target() const { return currentFrame.get(); }

		void render_2d(::System::Video3D* _v3d, Vector2 const & _centreAt, Optional<Vector2> const& _size, float _angle = 0.0f, ::System::BlendOp::Type _srcBlend = ::System::BlendOp::One, ::System::BlendOp::Type _destBlend = ::System::BlendOp::OneMinusSrcAlpha);
		void render_3d(::System::Video3D* _v3d, Transform const & _placement, ::System::BlendOp::Type _srcBlend = ::System::BlendOp::One, ::System::BlendOp::Type _destBlend = ::System::BlendOp::OneMinusSrcAlpha);
		void render_vr_scene(::System::Video3D* _v3d, DisplayVRSceneContext * _context = nullptr, ::System::BlendOp::Type _srcBlend = ::System::BlendOp::One, ::System::BlendOp::Type _destBlend = ::System::BlendOp::OneMinusSrcAlpha);
#ifdef USE_DISPLAY_MESH
		Meshes::Mesh3DInstance const & get_vr_mesh_instance() const { return deviceMesh3DInstance; }
		Transform const & get_vr_mesh_placement() const { return deviceMesh3DPlacement; }
#else
		Meshes::Mesh3DInstance get_vr_mesh_instance() const { error(TXT("enable USE_DISPLAY_MESH")); return Meshes::Mesh3DInstance(); }
		Transform const& get_vr_mesh_placement() const { error(TXT("enable USE_DISPLAY_MESH")); return Transform::identity; }
#endif

		::System::RenderTarget* get_output_render_target() { return is_visible()? outputRenderTarget.get() : nullptr; }

		bool is_on() const { return state == DisplayState::On; }
		void turn_on(bool _instant = false);
		void turn_off() { state = DisplayState::Off; }
		float get_visible_power_on() const { return clamp(visiblePowerOn, 0.0f, 1.0f); }
		bool is_visible() const { return visiblePowerOn > 0.005f; }

		void no_longer_visible(); // call it if we're sure, display won't be visible for some time

	public: // background
		void use_no_background(); // default
		void use_background(::System::Texture* _texture, float _backgroundColourBased = 0.0f);
		void use_background(::System::RenderTarget* _rt, int _index, float _backgroundColourBased = 0.0f);
		void set_background_fill_mode(DisplayBackgroundFillMode::Type _fillMode, VectorInt2 const & _atScreen = VectorInt2::zero, VectorInt2 const & _size = VectorInt2::one) { backgroundFillMode = _fillMode; backgroundAtScreenLocation = _atScreen; backgroundAtScreenSize = _size; }
		bool is_background_set() const { return backgroundTexture != ::System::texture_id_none(); }

	public: // cursor
		void use_cursor(Name const & _cursorName);
		void use_cursor_not_touched(Name const & _cursorName);
		void show_cursor_always_touching(bool _alwaysTouched) { showCursorAlwaysTouching = _alwaysTouched; }
		void use_default_cursor() { use_cursor(Name::invalid()); use_cursor_not_touched(Name::invalid()); }
		void lock_cursor_for_drawing(Name const & _cursorNameUntilDrawn, Name const & _cursorNameAfterDrawn = Name::invalid());
		void show_cursor(DisplayCursorVisible::Type _cursorRequestedVisibility = DisplayCursorVisible::IfActive) { cursorRequestedVisibility = _cursorRequestedVisibility; }
		void hide_cursor() { cursorRequestedVisibility = DisplayCursorVisible::No; }
		void move_cursor(Vector2 const & _by, bool _jump);
		inline bool can_cursor_be_seen() const { return cursorIsActive; }
		inline bool is_cursor_requested_to_be_visible() const { return cursorRequestedVisibility == DisplayCursorVisible::Always || (cursorRequestedVisibility == DisplayCursorVisible::IfActive && (cursorIsActive || cursorNameUntilDrawn.is_valid())); }
		inline bool is_cursor_visible() const { return is_cursor_requested_to_be_visible() && !cursorHiddenDueToDragging; }
		inline bool is_cursor_active() const { return cursorIsActive; }

		void set_cursor_ink(Colour const & _ink = Colour::none) { cursorInk = _ink; }
		void set_cursor_paper(Colour const & _paper = Colour::none) { cursorPaper = _paper; }
		void use_cursor_colours(Optional<bool> _useCursorColours = NP) { cursorUseColours = _useCursorColours; }

		Vector2 const & get_cursor_at() const { return cursorAt; }
		Vector2 const & get_cursor_moved_by() const { return cursorMovedBy; }
		Vector2 const & get_cursor_dragged_by() const { return cursorDraggedBy; }
		Vector2 get_cursor_normalised_at() const;
		Vector2 get_cursor_normalised_moved_by() const;
		Vector2 get_cursor_normalised_dragged_by() const;
		bool is_cursor_dragged_by() const { return cursorIsDraggedBy; }

	private:
		void set_cursor_active(bool _active); // this is used when auto activating/deactivating

	public: // touch as press
		static void allow_touch_as_press(bool _touchIsPress) { s_touchIsPress = _touchIsPress; }
		static bool should_allow_touch_as_press() { return s_touchIsPress; }
		void push_touch_as_press(bool _touchIsPress) { touchIsPressStack.push_back(touchIsPress); touchIsPress = _touchIsPress; }
		void pop_touch_as_press() { if (!touchIsPressStack.is_empty()) { touchIsPress = touchIsPressStack.get_last(); touchIsPressStack.pop_back(); } }

	private:
		bool should_allow_touch_as_press_internal() { return s_touchIsPress || touchIsPress; }

	public: // draw commands
		// we gather draw commands and send them to be drawn in advance - to avoid flickering when drawing happens during advancement
		DisplayDrawCommand* add(DisplayDrawCommand* _drawCommand); // returns prepared draw command
		void drop_all_draw_commands(bool _addHoldDrawing = true);
		void draw_all_commands_immediately() { doAllDrawCyclesGathered = true; }
		void always_draw_commands_immediately(bool _alwaysDrawCommandsImmediately = true) { alwaysDrawCommandsImmediately = _alwaysDrawCommandsImmediately; }

	public: // controls in general
		void add(DisplayControl* _control);
		void remove(DisplayControl* _control, bool _noRedrawNeeded = false, bool _dontSelectAntyhing = false);
		bool has_control(DisplayControl* _control) const;
		DisplayControl* find_control(Name const & _id) const;
		void remove_all_controls(bool _noRedrawNeeded = false);
		void select(DisplayControl* _control, Optional<VectorInt2> _cursor = NP, Optional<VectorInt2> _goingInDir = NP, bool _silent = false);
		void select_anything(DisplayControl* _exceptControl = nullptr);
		void select_by_id(Name const & _id);
		DisplayControl* get_selected() const { return selectedControl.get(); }

		void push_controls_lock(); // to avoid selecting controls
		void pop_controls_lock(); // to avoid selecting controls
		void pop_all_controls_lock(); // to avoid selecting controls

		void push_controls(); // to allow removal of only recently added controls
		void pop_controls(bool _noRedrawNeeded = false); // remove controls added after last push_controls
		void push_controls(int _keepThemAtLevel, bool _noRedrawNeeded = false);
		void pop_controls(int _fromLevel, bool _noRedrawNeeded = false);
		void pop_all_controls(bool _noRedrawNeeded = false) { pop_controls(0, _noRedrawNeeded); }
		void controls_for_level(int _level, bool _noRedrawNeeded = false) { pop_controls(_level, _noRedrawNeeded); push_controls(_level); }
		
		void redraw_controls(bool _clear = false);

	public: // buttons
		int place_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoord _innerPadding = 1, CharCoord _spacing = 2); // returns number of lines
		RangeCharCoord2 place_on_grid_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoords _buttonSize, CharCoords _spacing = CharCoords::zero, CharCoord _innerPadding = 0, VectorInt2 _limit = VectorInt2::zero); // returns rect occupied, if spacing is 0, it is evenly placed
		int place_vertically_and_add(RangeCharCoord2 const & _rect, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoord _innerPadding = 1, CharCoord _spacing = 1); // returns number of lines
		int place_on_band_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoord _innerPadding = 1, CharCoord _spacing = 2, CharCoord _vPadding = 0); // returns number of lines, easier version of placing buttons horizontally
		int place_horizontally_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, Array<DisplayButton*>& _buttons, DisplayVAlignment::Type _vAlignment = DisplayVAlignment::Centre, CharCoord _vPadding = NONE, CharCoord _maxButtonWidth = NONE, CharCoord _innerPadding = 1, CharCoord _spacing = 1, bool _keepPaddingAndSpacing = false); // _vPadding NONE means stretch, returns number of lines
		int place_horizontally(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, Array<DisplayButton*>& _buttons, DisplayVAlignment::Type _vAlignment = DisplayVAlignment::Centre, CharCoord _vPadding = NONE, CharCoord _maxButtonWidth = NONE, CharCoord _innerPadding = 1, CharCoord _spacing = 1, bool _keepPaddingAndSpacing = false); // _vPadding NONE means stretch, returns number of lines

	public: // input
		void process_input(IGameInput const & _input, float _deltaTime);

	public: // accessible information
		Vector2 const & get_left_bottom_of_screen() const { return leftBottomOfScreen; }
		Vector2 const & get_right_top_of_screen() const { return rightTopOfScreen; }
		Vector2 const & get_left_bottom_of_display() const { return leftBottomOfDisplay; }
		Vector2 const & get_right_top_of_display() const { return rightTopOfDisplay; }
		Vector2 const & get_char_size() const { return charSizeAsVector2; }
		CharCoords const & get_char_resolution() const { return charResolution; }
		RangeCharCoord2 get_char_screen_rect() const { return RangeCharCoord2(RangeCharCoord(0, charResolution.x - 1), RangeCharCoord(0, charResolution.y - 1)); }
		Font* get_font(Name const & _alphabet = Name::invalid()) const { return setup.get_font(_alphabet).get(); }

		DisplaySetup const & get_setup() const { return setup; }
		DisplayRegion const * get_region(Name const & _regionName) const { return setup.regions.get_existing(_regionName); }
		String const * get_crosshair(Name const & _crosshairName) const { return setup.crosshairs.get_existing(_crosshairName); }
		DisplayColourPair const * get_colour_pair(Name const & _colourPairName = Name::invalid()) const { return setup.colourPairs.get_existing(_colourPairName); }

		void use_colourise(bool _colourise) { colourise = _colourise; }
		bool get_colourise() const { return colourise; }

		void set_default_ink(Colour const & _ink) { setup.defaultInk = _ink; set_current_ink(_ink); }
		void set_default_paper(Colour const & _paper) { setup.defaultPaper = _paper; set_current_paper(_paper); }
		void set_current_ink(Colour const & _ink) { currentInk = _ink; }
		void set_current_paper(Colour const & _paper) { currentPaper = _paper; }
		Colour const & get_current_ink() const { return currentInk; }
		Colour const & get_current_paper() const { return currentPaper; }
		void use_colour_pair(Name const & _colourPairName = Name::invalid());
		
	public: // characters used to draw frame
		static inline tchar get_frame_lt() { return 0x250c; }
		static inline tchar get_frame_lc() { return 0x251c; }
		static inline tchar get_frame_lb() { return 0x2514; }
		static inline tchar get_frame_ct() { return 0x252c; }
		static inline tchar get_frame_cc() { return 0x253c; }
		static inline tchar get_frame_cb() { return 0x2534; }
		static inline tchar get_frame_rt() { return 0x2510; }
		static inline tchar get_frame_rc() { return 0x2524; }
		static inline tchar get_frame_rb() { return 0x2518; }
		static inline tchar get_frame_h() { return 0x2500; }
		static inline tchar get_frame_v() { return 0x2502; }

	private:
		static bool s_touchIsPress;
		Array<bool> touchIsPressStack;
		bool touchIsPress = true;

		// setup
		UsedLibraryStored<DisplayType> displayType;
		DisplaySetup setup;

		bool initialised = false;

		// calculated values
		CACHED_ VectorInt2 charSize;
		CACHED_ VectorInt2 originalOutputPreDistortionSize; // pre distortion - bigger to have 1:1 pixel in the middle
		CACHED_ VectorInt2 outputPreDistortionSize; // may be bigger due to modifications
		CACHED_ Vector2 screenSize;
		CACHED_ Vector2 leftBottomOfScreen;
		CACHED_ Vector2 rightTopOfScreen;
		CACHED_ Vector2 leftBottomOfDisplay;
		CACHED_ Vector2 rightTopOfDisplay;
		CACHED_ Vector2 charSizeAsVector2;
		CACHED_ CharCoords charResolution;

		// functions / callbacks to allow execution at proper time
		template <typename Func>
		struct FuncInfo
		{
			void const * owner;
			Func perform;
		};
		Array<FuncInfo<OnAdvanceDisplay>> on_advance_display;
		Array<FuncInfo<OnUpdateDisplay>> on_update_display;
		Array<FuncInfo<OnRenderDisplay>> on_render_display; // this is post draw commands
		Array<FuncInfo<OnClick>> on_click;

		// runtime
		int advancedOnFrame = NONE; // both to check if updated continuously
		int updatedDisplayOnFrame = NONE;
		bool cursorAbsolute01ProvidedInLastFrame = false;
		DisplayCoordinates::Type useCoordinates = DisplayCoordinates::Char;
		::System::RenderTargetPtr backgroundRenderTarget;
		::System::TextureID backgroundTexture = ::System::texture_id_none();
		Vector2 backgroundTextureSize = Vector2::one;
		DisplayBackgroundFillMode::Type backgroundFillMode = DisplayBackgroundFillMode::Screen;
		VectorInt2 backgroundAtScreenLocation = VectorInt2::zero;
		VectorInt2 backgroundAtScreenSize = VectorInt2::one;
		float backgroundColourBased = 0.0f;
		int rotateDisplay = 0; // only if square, ATM does not support cursor properly
		::System::RenderTargetPtr currentFrame;
		::System::RenderTargetPtr previousFrame;
		::System::RenderTargetPtr currentRotatedFrame;
		::System::RenderTargetPtr previousRotatedFrame;
		::System::RenderTargetPtr outputRenderTarget; // if we use post process instance, this will be render target of output node
		::System::RenderTargetPtr ownOutputRenderTarget;
		PostProcessRenderTargetPointer ppOutputRenderTarget;
		PostProcessRenderTargetPointer ppOutputForCasingRenderTarget;
		Map<Name, PostProcessRenderTargetPointer> ppLastFrameOutputRenderTargets;

		PostProcessInstance* postProcessInstance;
#ifdef USE_DISPLAY_CASING_POST_PROCESS
		PostProcessInstance* postProcessForCasingInstance;
#endif
		PostProcessRenderTargetManager* postProcessRenderTargetManager;
		PostProcessRenderTargetManager* ownPostProcessRenderTargetManager; // created if render target manager was not provided (or couldn't get it from game)
		PostProcessGraphProcessingInputRenderTargetsProvider postProcessRenderTargetsProvider;
		PostProcessGraphProcessingInputRenderTargetsProvider postProcessRenderTargetsProviderForCasing;
		PostProcessGraphProcessingResolutionProvider postProcessResolutionProvider;

		::Random::Generator randomGenerator;

		Vector2 noiseTextureCurrentOffset;
		Vector2 noiseTexturePreviousOffset;

		Name cursorName;
		Name cursorNameNotTouching;
		Name cursorNameUntilDrawn;
		Name cursorNameAfterDrawn;

		DisplayCursor const * currentCursor = nullptr;
		DisplayCursor const * currentCursorNotTouching = nullptr;
		bool showCursorAlwaysTouching = false; // to override_ not touching when choosing cursor to show
		Vector2 cursorAt; // 0,0 is lower screen corner
		Vector2 cursorMovedBy; // any movement does it
		Vector2 cursorDraggedBy; // only if touching or with cursor pressed
		bool cursorIsDraggedBy;
		DisplayCursorVisible::Type cursorRequestedVisibility = DisplayCursorVisible::No; // as requested by other systems
		bool cursorHiddenDueToDragging = false;
		bool cursorNotTouching = false;
		bool cursorIsActive; // can cursor be seen mouse was moved, not gamepad, this is switched off on any gamepad and keyboard input (with exception for Escape, as it is treated as special key)
		bool cursorDependantOnInput = false; // change it in future if we have full screen displays - or change whole system
		bool cursorIsPressed; // is cursor physically pressed (not touched, tapped, has to be pressed!)
		bool cursorWasPressed; // was cursor pressed
		bool cursorIsTouched; // is cursor physically touched (not fake touched!)
		bool cursorWasTouched; // was cursor pressed
		static bool s_cursorIsActive; // global one to preserve state when switching between displays

		Vector2 cursorMovementSinceLongerInactive = Vector2::zero; // this is used to avoid activation of mouse when my desk is trembling
		float inputCursorInactiveTime = 0.0f;
		bool wasInputProcessed = false;
		
		Colour cursorInk = Colour::none; // if none, current ink is used
		Colour cursorPaper = Colour::none; // if none, current paper is used
		Optional<bool> cursorUseColours; // overrides cursor if set

		float retraceAtPct;
		float retraceVisibleAtPct;
		bool ignoreGameFrames; // if doesn't ignore game frames, will do certain things depending on whether game frames were advanced or not (and when that happened)
		int lastAdvanceGameFrameNo;
		int lastUpdateDisplayGameFrameNo;
		bool doNextFrame;
		bool doAllDrawCycles;
		bool doAllDrawCyclesGathered;
		bool alwaysDrawCommandsImmediately = false;
		int accumulatedDrawCycles;
		int drawCyclesUsedByCurrentDrawCommand;

		DisplayState::Type state = DisplayState::Off;
		float powerOn = 0.0f;
		float visiblePowerOn = 0.0f; // this value is not clamped!
		float powerOnLightenUpCoef = 1.0f;
		Vector2 scaleDisplay = Vector2::zero;

		::Concurrency::SpinLock drawCommandsLock = Concurrency::SpinLock(TXT("Framework.Display.drawCommandsLock"));
		List<DisplayDrawCommandPtr> drawCommands;
		::Concurrency::SpinLock drawCommandsGatherLock = Concurrency::SpinLock(TXT("Framework.Display.drawCommandsGatherLock"));
		List<DisplayDrawCommandPtr> drawCommandsGather;
		bool gatheredDropAllDrawCommands = false; // we don't want to drop actual draw commands somewhere during frame as we could drop them before they are drawed

		// crash check now happens only during process input, as only then user code is called
		bool hasCrashed = false;
		int crashedCode = 0;

#ifdef AN_DEVELOPMENT
		volatile bool __advancing = false;
		volatile bool __rendering = false;
#endif

		// controls
		int controlsLockLevel = NONE;
		int controlsStackLevel = NONE;
		RefCountObjectPtr<DisplayControl> selectedControl;
		DisplaySelectorDir::Type lastSelectorDir = DisplaySelectorDir::None;
		float lastSelectorTime = 0.0f;
		int repeatSelectorCount = 0;
		DisplaySelectorDir::Type lastCursorHoverDir = DisplaySelectorDir::None;
		float lastCursorHoverTime = 0.0f;
		int repeatCursorHoverCount = 0;
		DisplayCursorState::Type cursorState = DisplayCursorState::None;
		bool touchingIsActive = false;
		bool usingCursorTouch = false;
		float cursorActivateTime = 0.0f; // how long it was activatedy
		float cursorDragTime = 0.0f;
		RefCountObjectPtr<DisplayControl> cursorActiveControl;
		RefCountObjectPtr<DisplayControl> cursorHoverControl;
		RefCountObjectPtr<DisplayControl> preShortcutControl;

		VectorInt2 clickCursorAtAsVectorInt2;
		float const cursorClickThreshold = 0.1f; // threshold for activation, if exceeded, dragging is enabled - if dragging is impossible, we repeat cursor process/activate
		float const cursorDoubleClickThreshold = 0.3f; // threshold for activation, if exceeded, dragging is enabled - if dragging is impossible, we repeat cursor process/activate
		float const touchThresholdTimeCoef = 2.5f;
		float const repeatActionFirstTime = 0.35f;
		float const repeatActionTime = 0.1f;

		enum ActiveDisplayControlFlags
		{
			ADPF_None = 0,
			ADPF_Shortcut = 1,
			ADPF_CursorPress = 2,
			ADPF_SelectorPress = 4,
		};
		struct ActiveDisplayControl
		{
			DisplayControl* control;
			int flags;
			float activeTime;
		};
		Array<ActiveDisplayControl> activeControls;
		// these don't have to be ref counted as we clear them when we remove controls
		DisplayControl* activeControlDueToCursorPress = nullptr;
		DisplayControl* activeControlDueToSelectorPress = nullptr;

		// controls
		mutable ::Concurrency::SpinLock controlsLock = Concurrency::SpinLock(TXT("Framework.Display.controlsLock"));
		Array<RefCountObjectPtr<DisplayControl>> controls;

		bool colourise = false; // will override_ colouring - instead of using just ink and/or paper, will turn white->ink, black->paper allowing for using more complex graphics and just changing them to fit colours

		Colour currentInk;
		Colour currentPaper;

#ifdef USE_DISPLAY_MESH
		bool useMeshes = true; // in some cases we may not want to create/use meshes, we're only interested in display output 
#else
		bool useMeshes = false; // explicitly provide meshes
#endif

		// presentation using mesh
#ifdef USE_DISPLAY_MESH
		Mesh* deviceMesh2D; // mesh2d has nominal size y = 1,  x = anything else
		bool deviceMesh2DFromLibrary; // for reusing
		Meshes::Mesh3DInstance deviceMesh2DInstance;
		Vector2 deviceMesh2DSize;

		Mesh* deviceMesh3D;
		bool deviceMesh3DFromLibrary; // for reusing
		CACHED_ Transform deviceMesh3DPlacement;
		Meshes::Mesh3DInstance deviceMesh3DInstance;
		Meshes::Mesh3DInstance deviceMesh3DInstanceTempForRendering; // temporary for rendering, to have hard copy, we should use it on one thread only!
		Vector2 deviceMesh3DSize;
#endif

#ifdef USE_DISPLAY_VR_SCENE
		Mesh* vrSceneMesh3D;
		bool vrSceneMesh3DFromLibrary;
		Meshes::Mesh3DInstance vrSceneMesh3DInstance;
#endif

		SoundSources soundSources;

		void next_frame(); // swap and copy what was in previous "current frame" to our current "current frame"

		void defer_rendering_delete();

		DisplaySelectorDir::Type choose_dir(Vector2 const & _dir) const;
		void clear_active_controls();
		void add_active_control(DisplayControl* _control, int _flags);
		bool remove_active_control(DisplayControl* _control, int _flags = 0xffffffff);
		bool has_active_control(int _flags) const;
		DisplayControl* get_active_control(int _flags) const;

		void process_input_internal(IGameInput const & _input, float _deltaTime);

		friend struct Display_DeleteRendering;

		void move_gathered_draw_commands_to_drawing(); // done at the end of advancement

		void ready_device_mesh_3d_instance_to_render();

		template <typename Func>
		void set_func_info(Array<FuncInfo<Func>> & _array, Func _perform, void const * _owner);

		template <typename Func>
		void perform_func_info(Array<FuncInfo<Func>> & _array, typename std::function<void(AN_NOT_CLANG_TYPENAME Func _perform)> _performFunc);

		// util functions to check if rendering and advancement happen at the same time - for development only
		// they only should be used to catch situations when not all draw commands were issued but were started to be rendered
		inline void __doing_advancement_now();
		inline void __done_advancement_now();
		inline void __doing_rendering_now();
		inline void __done_rendering_now();

		inline void update_post_process_frames();

#ifdef AN_DEVELOPMENT
	public:
		bool debugOutput = false;
#endif
	};
};
