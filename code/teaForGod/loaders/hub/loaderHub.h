#pragma once

#include "..\..\..\core\random\random.h"
#include "..\..\..\core\sound\playback.h"

#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\game\gameInput.h"
#include "..\..\..\framework\loaders\iLoader.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\sound\soundSample.h"

struct Vector2;
struct VectorInt2;
struct Colour;

namespace System
{
	class Video3D;
};

namespace Framework
{
	class Display;
	class EnvironmentType;
	class MeshGenerator;
	class TexturePart;
	class Sample;
};

namespace TeaForGodEmperor
{
	class Game;
};

namespace Loader
{
	struct HubDraggable;
	struct HubScreen;
	struct HubWidget;
	class Hub;
	class HubScene;

	namespace HubGraphics
	{
		enum Type
		{
			LogoBigOneLine,

			MAX
		};
	};

	struct HubHand
	{
		Framework::GameInput input;

		float timeSinceLastClick = 10.0f;
		float pressingFor = 0.0f;
		RefCountObjectPtr<HubDraggable> dragged;
		RefCountObjectPtr<HubDraggable> placeHolder; // if we drag something, we keep that place as a place holder
		RefCountObjectPtr<HubWidget> draggedFromWidget;
		RefCountObjectPtr<HubWidget> hoveringDraggedOver;

		bool isActive = false;
		Transform placement = Transform::identity;
		Transform lastPointerPlacement = Transform::identity;
		Meshes::Mesh3DInstance meshInstanceController;
		Meshes::Mesh3DInstance meshInstancePointHandController;
		Meshes::Mesh3DInstance meshInstanceHandTracking;
		Framework::UsedLibraryStored<Framework::Mesh> meshController;
		Framework::UsedLibraryStored<Framework::Mesh> meshPointHandController;
		Framework::UsedLibraryStored<Framework::Mesh> meshHandTracking;
		Framework::UsedLibraryStored<Framework::TexturePart> cursor;
		Framework::SocketID pointerSocket;
		Framework::SocketID pointHandSocket;
		Transform pointerMS = Transform::identity;
		Transform pointHandMS = Transform::identity; // point with whole hand

		RefCountObjectPtr<HubScreen> onScreen;
		float onScreenActive = 0.0f;
		Vector2 onScreenAt = Vector2::zero;
		float onScreenUpAngle = 0.0;

		RefCountObjectPtr<HubScreen> onScreenTarget;
		Vector2 onScreenTargetAt = Vector2::zero;
		float onScreenTargetUpAngle = 0.0f;

		RefCountObjectPtr<HubWidget> overWidget;
		RefCountObjectPtr<HubWidget> overWidgetOnPress; // when we started to press the button
		RefCountObjectPtr<HubWidget> prevOverWidget;
		RefCountObjectPtr<HubWidget> heldWidget;
		bool heldWidgetGripped = false;
		RefCountObjectPtr<HubScreen> heldWidgetOnScreen; // for mere reference
	};

	struct HubColours
	{
		static Colour widget_background();
		static Colour widget_background_highlight();
		static Colour screen_border();
		static Colour screen_interior();
		static Colour border();
		static Colour mid();
		static Colour special_highlight();
		static Colour special_highlight_background();
		static Colour special_highlight_background_for(Optional<Colour> const & _colour);
		static Colour selected_highlighted();
		static Colour highlight();
		static Colour text();
		static Colour text_bright();
		static Colour text_inv();
		static Colour warning();
		static Colour match();
		static Colour unavailable();
		static Colour unlockable();
		static Colour ink();
		static Colour ink_highlighted();
		static Colour button_cant();
		static Colour button_not_yet();
	};
	/**
	 *	Hub manages fading in game
	 *
	 *	There are multiple ways to handle forward dir
	 *		get_current_forward						this is current forward dir, it is updated when user is looking horizontally
	 *		get_initial_forward						this is first forward dir since hub activation, resets when no hub is active
	 *		get_main_forward						this is forward stored when hub was opened and will go through multiple hubs, resets when no hub is active
	 *		get_background_dir_or_main_forward		looks for background screen and takes dir from it or if not available, from main forward
	 *		get_background_dir_or_current_forward	looks for background screen and takes dir from it or if not available, from current forward
	 *		get_snapped_current_forward				if current forward is close to background or main, it snaps there
	 */
	class Hub
	: public ILoader
	{
		typedef ILoader base;
	public:
		Hub(TeaForGodEmperor::Game* _game, tchar const* _loaderName = nullptr, Optional<Name> const& _sceneGlobalReferenceName = NP,
			Optional<Name> const& _backgroundGlobalReferenceName = NP, Optional<Name> const& _environmentGlobalReferenceName = NP, bool _forceRecreateSceneMesh = false);
		~Hub();

		void show_start_movement_centre(bool _showStartMovementCentre = true) { showStartMovementCentre = _showStartMovementCentre; }

		// do that before updating scene mesh
		void set_emissive_colour_override(Optional<Colour> const& _emissiveColourOverride) { emissiveColourOverride = _emissiveColourOverride; }
		void update_scene_mesh(); // in case vr size/placement has changed
		void update_background_mesh();

		Colour const& get_screen_interior_colour() const { return screenInteriorColour; }

		Hub& allow_to_deactivate_with_loader_immediately(bool _allowDeactivatingWithLoaderImmediately = true) { allowDeactivatingWithLoaderImmediately = _allowDeactivatingWithLoaderImmediately; return *this; }
		bool does_allow_deactivating_with_loader_immediately() const { return allowDeactivatingWithLoaderImmediately; }
		Hub& set_scene(HubScene* _scene);
		HubScene* get_scene() const { return currentScene.get(); }
		bool does_require_fade_out() const { return requiresToFadeOut; }
		void set_require_fade_out(bool _requiresToFadeOut = true) { requiresToFadeOut = _requiresToFadeOut; }
		void allow_fade_out(bool _allowFadeOut = true) { fadeOutAllowed = _allowFadeOut; }
		void disallow_fade_out(bool _dontFadeOut = true) { fadeOutAllowed = !_dontFadeOut; }
		bool does_allow_fade_out() const { return fadeOutAllowed; }

		void allow_input(bool _allowInput = true) { inputAllowed = _allowInput; }
		void disallow_input(bool _dontInput = true) { inputAllowed = !_dontInput; }

		void require_release_select() { requiresReleaseSelect = true; }

		HubHand& access_hand(::Hand::Type _hand) { return hands[_hand]; }
		HubHand const& get_hand(::Hand::Type _hand) const { return hands[_hand]; }

		Transform const& get_last_view_placement() const { return lastViewPlacement; }

		float get_use_scene_scale() const { return useSceneScale; }

		Array<RefCountObjectPtr<HubScreen>> const& get_all_screens() const { return screens; }
		HubScreen* get_screen(Name const& _id, bool _onlyActive = true) const;
		void add_screen(HubScreen* _screen);
		void remove_old_screens();
		void remove_all_screens();
		void remove_deactivated_screens();
		void deactivate_all_screens();
		void keep_only_screen(Name const& _id, bool _onlyActive = true);
		void keep_only_screens(Array<Name> const& _ids, bool _onlyActive = true);
		void rotate_screens(Rotator3 const& _by);
		void rotate_everything_by(float _yaw);

		static void reset_last_forward();
		void force_reset_and_update_forward();

		Rotator3 get_current_forward() const { return forward.get(Rotator3::zero); }
		Rotator3 get_initial_forward() const { return initialForward.get(Rotator3::zero); }
		Rotator3 get_main_forward() const;
		Rotator3 get_background_dir_or_main_forward() const;
		Rotator3 get_background_dir_or_current_forward() const;
		Rotator3 get_snapped_current_forward(Optional<float> const& _threshold = NP) const;
		float get_radius() const { return hubInfo.radius; }
		Optional<Transform> const& get_start_movement_centre() const { return startMovementCentre; }

		Rotator3 get_last_view_rotation() const { return lastViewRotation; }

		Framework::TexturePart* get_graphics(HubGraphics::Type _graphics) const { return graphics[_graphics].get(); }

		void quit_game(bool _unload = false); // fades out and exits

		Sound::Playback play(Framework::UsedLibraryStored<Framework::Sample> const& _sample) const;
		void play_move_sample();

		void select(HubWidget* _widget, HubDraggable const* _draggable = nullptr, bool _clicked = false); // draggable within widget
		void deselect();
		bool is_selected(HubWidget* _widget, HubDraggable const* _draggable = nullptr)  const;
		HubDraggable* get_selected_draggable() const { return selectedDraggable.get(); }

		bool is_hand_engaged(Hand::Type _hand) const;

		bool is_trigger_held_for_button_hold() const;

		void force_drop_dragged(); // useful when changing scenes

		void force_click(HubWidget* _widget, bool _shifted = false); // or queue

		void force_deactivate();

		void clear_special_highlights();
		void special_highlight(Name const& _screenId, Name const& _widgetId, bool _highlight = true);

		void set_beacon_active(bool _beaconActive, bool _beaconRActive = false, float _beaconZ = 1.7f) { beaconActive = _beaconActive; if (beaconActive) { beaconRActive = _beaconRActive; if (beaconRActive) { beaconZValue = _beaconZ; } } }

	public: // font
		void use_font(Framework::Font*);
		Framework::Font const* get_font() const;

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D* _v3d, bool _vr);
		implement_ bool activate();
		implement_ void deactivate();

	private: friend struct HubScreen;
		   int allowedCreateDisplayAndMesh = 1;
		   bool screen_wants_to_create_display_and_mesh() { if (allowedCreateDisplayAndMesh > 0) { --allowedCreateDisplayAndMesh; return true; } else { return false; } }

	private:
		static Optional<Rotator3> s_lastForward;
		static System::TimeStamp s_lastForwardTimeStamp;
		static bool s_autoPointWithWholeHands[Hand::MAX];

		struct HubInfo
		{
			Vector3 sphereCentre = Vector3(0.0f, 0.0f, 1.4f);
			float radius = 6.0f;
			float floorWidth = 2.0f;
			float floorLength = 1.5f;
		} hubInfo;

		TeaForGodEmperor::Game* game = nullptr;

		Framework::UsedLibraryStored<Framework::Font> font;

		bool allowDeactivatingWithLoaderImmediately = true; // when loader deactivated, will immediately deactivate (won't wait for scene)
		bool requiresToFadeOut = false; // if this is set, game is arranged to fade out
		bool fadeOutAllowed = true; // if true, won't fade out on its own
		bool wantsToExit = false; // if wants to exit, will fade out or deactivate immediately
		bool inputAllowed = true;

		// we shouldn't force clicks during screen advancement when we're going through arrays of screens and widgets
		struct QueuedForcedClicks
		{
			RefCountObjectPtr<HubWidget> widget;
			bool shifted = false;
			QueuedForcedClicks();
			QueuedForcedClicks(HubWidget* _widget, bool _shifted = false);
		};
		ArrayStatic<QueuedForcedClicks, 8> queuedForcedClicks;
		bool queueForcedClicks = false;

		RefCountObjectPtr<HubScene> currentScene;
		RefCountObjectPtr<HubScene> nextScene;

		Colour fadeColour = Colour::black;
		Colour screenInteriorColour = Colour::black;

		Optional<Colour> emissiveColourOverride;

		bool allowDeactivating = false;
		Random::Generator randomGenerator;

		String loaderName;
		bool forceRecreateSceneMesh = false;
		Framework::UsedLibraryStored<Framework::MeshGenerator> sceneMeshGenerator;
		Framework::UsedLibraryStored<Framework::Mesh> sceneMesh; // will create on fly using reference "loader hub scene mesh generator"
		Framework::UsedLibraryStored<Framework::MeshGenerator> backgroundMeshGenerator;
		Framework::UsedLibraryStored<Framework::Mesh> backgroundMesh; // will create on fly using reference "loader hub scene mesh generator"
		Framework::UsedLibraryStored<Framework::EnvironmentType> environment; // will create on fly using reference "loader hub scene mesh generator"
		Framework::UsedLibraryStored<Framework::Mesh> startHeadMarkerMesh; // feet/cross
		Framework::UsedLibraryStored<Framework::Mesh> startHeadMarkerCloseMesh; // around actual centre
		Framework::UsedLibraryStored<Framework::Mesh> startHeadMarkerFarMesh; // on ground, covers, wider area

		Framework::UsedLibraryStored<Framework::TexturePart> graphics[HubGraphics::MAX];
		Framework::UsedLibraryStored<Framework::Sample> clickSample; // will create on fly using reference "loader hub scene mesh generator"
		Framework::UsedLibraryStored<Framework::Sample> moveSample; // will create on fly using reference "loader hub scene mesh generator"

		Framework::UsedLibraryStored<Framework::TexturePart> helpIcon;
		Framework::UsedLibraryStored<Framework::TexturePart> restartAtAnyChapterInfo;

		HubHand hands[2];

		Meshes::Mesh3DInstance sceneMeshInstance;
		Meshes::Mesh3DInstance backgroundMeshInstance;
		Meshes::Mesh3DInstance startHeadMarkerMeshInstance;
		Meshes::Mesh3DInstance startHeadMarkerCloseMeshInstance;
		Meshes::Mesh3DInstance startHeadMarkerFarMeshInstance;

		Rotator3 lastViewRotation = Rotator3::zero;
		Transform lastViewPlacement = Transform::identity;
		Transform lastMovementCentrePlacement = Transform::identity; // offset to view

		Optional<Vector3> referencePoint;

		Vector3 mousePretendLoc = Vector3(0.0f, 0.0f, 1.65f);
		Rotator3 followMouseRotation = Rotator3::zero;
		Rotator3 mouseRotation = Rotator3::zero;
		HubHand mousePretendHand;

		Optional<Rotator3> forward;
		Optional<Rotator3> viewForward; // always updated
		Optional<Rotator3> initialForward;
		Optional<Transform> startMovementCentre;
		bool showStartMovementCentre = false;

		int lazyScreenRenderIdx = 0;
		int lazyScreenRenderCount = 2;
		Array<RefCountObjectPtr<HubScreen>> screens;

		::System::TimeStamp timeSinceLastActive;

		bool requiresReleaseSelect = false;
		RefCountObjectPtr<HubWidget> selectedWidget;
		RefCountObjectPtr<HubDraggable> selectedDraggable;

		float beaconZValue = 1.5f; // alt
		float beaconActiveValue = 0.0f; // total
		float beaconRActiveValue = 0.0f;
		float beaconFarActiveValue = 0.0f; // if set to 1, only wide is visible, at 0.5 none, at 0.0 close
		bool beaconActive = false; // total / at all
		bool beaconRActive = false;

		void deactivate_now_as_loader();

		void activate_next_scene();

		void update_graphics();

		void update_forward(bool _initialToo = false);
		void store_start_movement_centre();

		Optional<Transform> calculate_start_movement_centre_marker() const;

		void drop_dragged_for(int handIdx);

		static bool should_use_point_with_whole_hand(Hand::Type _hand);

		bool should_lazy_render_screen_display(HubScreen* screen, int screenIdx) const;

		friend struct HubScreen;
		friend struct HubWidget;

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		bool allowDebugMovement = true;
#endif

	private:
		float useSceneScale = 1.0f;

		float adjust_screen_radius(float _radius, HubScreen* _forScreen) const;
		Transform adjust_screen_placement(Transform _placement, HubScreen* _forScreen) const;
		Vector3 adjust_pointer_placement(Vector3 _pointer, HubScreen* _forScreen) const;

		bool does_allow_updating_forward() const { return useSceneScale >= 0.5f; }

	private:
		struct Highlight
		{
			Name screen;
			Name widget;
			float timeLeft = 0.0f;
			bool active = false;

			Highlight() {}
			Highlight(float _timeLeft) : timeLeft(_timeLeft) {}
			Highlight(Name const& _screen, Name const& _widget, float _timeLeft) : screen(_screen), widget(_widget), timeLeft(_timeLeft) {}
		};
		Array<Highlight> highlights;

	public:
		void reset_highlights();
		void add_highlight_blink(Name const& _screen, Name const& _widget);
		void add_highlight_long(Name const& _screen, Name const& _widget);
		void add_highlight_infinite(Name const& _screen, Name const& _widget);
		void add_highlight_pause();
	};
};
