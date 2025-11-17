#pragma once

#include "..\library\library.h"
#include "..\video\material.h"

#include "..\..\core\types\name.h"
#include "..\..\core\containers\map.h"
#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	class World;

	/**
	 *	You should be able to load different bullshot setup while the game is running.
	 *	This belongs to Tea For God but with PgUp/Dn + Home you can select which game script trap to trigger to have some control over what's happening
	 * 
	 *	Custom params for objects
	 *		Name		bullshotEmissiveLayer
	 * 
	 *	Custom params for pilgrim
	 *		float		bullshotControlsMakeFistLeft
	 *		float		bullshotControlsMakeFistRight
	 *	
	 *	Custom variables (global)
	 *		LibraryName	useGearFromGameDefinition
	 *		int			useGearFromGameDefinitionIdx
	 *
	 *		int			bullshotEnvironmentCycleIdx
	 *		float		bullshotEnvironmentCyclePt
	 *		float		bullshotNoonYaw
	 *		float		bullshotNoonPitch
	 */

	struct BullshotObject
	{
		String name;

		UsedLibraryStored<ActorType> actorType;
		UsedLibraryStored<ItemType> itemType;
		UsedLibraryStored<SceneryType> sceneryType;
		UsedLibraryStored<Mesh> mesh;
		UsedLibraryStored<MeshGenerator> meshGenerator;

		TagCondition existingTagged;
		Tags tags; // tag newly created object

		TagCondition inRoom;
		Optional<Transform> placement;
		bool lockPlacement = false;

		SimpleVariableStorage parameters;

		struct BoneData
		{
			Name boneName;
			Optional<Vector3> translationLS;
			Optional<Rotator3> rotationLS;
			Optional<float> scaleLS;
		};
		Array<BoneData> poseLS;

		Array<MaterialSetup> materialSetups;

		Random::Generator rg;

		bool noAI = false;

		struct TriggerTemporaryObject
		{
			Name id;
			Optional<float> advanceOnlyFor;
		};
		Array<TriggerTemporaryObject> triggerTemporaryObjects;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
	};

	struct BullshotCell
	{
		// to force at specific location
		VectorInt2 at;
		UsedLibraryStored<RegionType> regionType;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
	};

	struct BullshotLogo
	{
		// to force at specific location
		Vector2 atPt = Vector2::half;
		float sizePt = 0.5f;
		UsedLibraryStored<TexturePart> texturePart;
		Colour colour = Colour::white;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
	};

	struct BullshotTitle
	{
		String text;
		float fontHeightPt = 0.1f;
		float finalFontScale = 1.0f;
		float lineSpacingPt = 0.2f; // between lines
		// relative to frame
		Vector2 atPt = Vector2::half; // at centre
		Vector2 textPt = Vector2::half; // centre
		Framework::UsedLibraryStored<Framework::Font> font;
		float stayTime = 1.0f;
		float comeTime = 0.5f;
		float awayTime = 1.0f;
		Optional<Range> perWordComeTimeOffset;
		Vector2 comeMask = Vector2::one;
		Vector2 awayMask = Vector2::one;
		Optional<Vector2> comeUnifiedDir;
		Optional<Vector2> awayUnifiedDir;
		Range comeDistPt = Range(0.2f, 0.3f); // t=0 beginning, p=0 at stay
		Range awayDistPt = Range(0.2f, 0.3f); // t=0 beginning, p=0 at stay
		BezierCurve<BezierCurveTBasedPoint<float>> comeCurve;
		BezierCurve<BezierCurveTBasedPoint<float>> awayCurve;

		Random::Generator rg = Random::Generator(1, 1);

		Colour ink = Colour::white;
		Colour paper = Colour::white.with_set_alpha(0.0f);

		CACHED_ Vector2 fontScale;
		CACHED_ Vector2 doneForSize;

		RUNTIME_ struct Element
		{
			float comeTime = 0.0f;
			float comeTimeOffset = 0.0f;
			String text;
			Vector2 at; // actual on screen
			Vector2 size;
			Vector2 comeLoc;
			Vector2 awayLoc;
		};
		Array<Element> elements;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		void break_into_elements();
	};

	class BullshotSystem
	{
	public:
		static BullshotSystem* get() { return s_bullshotSystem; }

		static void initialise_static();
		static void close_static();

		static Name const & get_id();

		static void log(LogInfoContext & _log);

		static bool is_active();
		static void set_active(bool _active);

		static bool does_want_to_draw();

		static float get_sub_step_details();

		static Concurrency::SpinLock & access_lock();
		static SimpleVariableStorage & access_variables();

		static bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		static void update_objects();
		static RegionType* get_cell_region_at(VectorInt2 const & _at);

		static void set_world(World* _world) { if (s_bullshotSystem) s_bullshotSystem->world = _world; }
		static void clear_world(World* _world) { if (s_bullshotSystem && s_bullshotSystem->world == _world) s_bullshotSystem->world = nullptr; }
		static void set_sub_world(SubWorld* _subWorld) { if (s_bullshotSystem) s_bullshotSystem->subWorld = _subWorld; }
		static void clear_sub_world(SubWorld* _subWorld) { if (s_bullshotSystem && s_bullshotSystem->subWorld == _subWorld) s_bullshotSystem->subWorld = nullptr; }

		static void hard_copy(IModulesOwner* _object);

		static void get_camera(OUT_ Room*& _inRoom, OUT_ Transform& _placement, OUT_ float& _fov, bool _force = false /* only if not consumed */);
		static void set_camera(Optional<Room*> const& _inRoom, Optional<Transform> const& _placement, Optional<float> const& _fov);

		static bool get_sound_camera(OUT_ Room*& _inRoom, OUT_ Transform& _placement); // returns true if actually used

		static void get_vr_placements(OUT_ Transform& headBoneOS, OUT_ Transform& handLeftBoneOS, OUT_ Transform& handRightBoneOS);

		static Optional<float> get_show_frame_aspect_ratio() { return s_bullshotSystem ? s_bullshotSystem->showFrameAspectRatio : NP; }
		static Optional<Vector2> get_show_frame_offset_pt() { return s_bullshotSystem ? s_bullshotSystem->showFrameOffsetPt : NP; }
		static Optional<Vector2> get_show_frame_render_centre_offset_pt() { return s_bullshotSystem ? s_bullshotSystem->showFrameRenderCentreOffsetPt : NP; }
		static RangeInt2 calculate_show_frame_for(VectorInt2 const& _size, Optional<VectorInt2> const& _centre = NP);
		static Name const & get_show_frame_template() { return s_bullshotSystem ? s_bullshotSystem->showFrameTemplate : Name::invalid(); }
		static Range2 get_show_frame_safe_area_pt() { return s_bullshotSystem ? s_bullshotSystem->showFrameSafeAreaPt : Range2(Range(0.0f, 1.0f), Range(0.0f, 1.0f)); }

		static Optional<float> const& get_near_z() { return s_bullshotSystem->nearZ; }
		
		static void advance(float _deltaTime, bool _skippable = false);
		static void render_logos_and_titles(::System::RenderTarget* _rt, Optional<VectorInt2> const& _centre = NP);

		static void change_title(int _changeBy);
		static void set_title_time(float _time);

		static bool is_setting_active(Name const& _what);

	private:
		static BullshotSystem* s_bullshotSystem;

		Name id;

		World* world = nullptr;
		SubWorld* subWorld = nullptr;

		bool active = false;
		bool wantsToDraw = false;

		float subStepDetails = 1.0f;

		Array<Name> settings;

		// offset placement of all objects and camera
		Name offsetPlacementRelativelyToParam;
		Transform offsetPlacement = Transform::identity;
		Transform soundOffsetPlacement = Transform::identity; // if different room

		Optional<float> showFrameAspectRatio; // will fill the screen
		Optional<Vector2> showFrameOffsetPt;
		Optional<Vector2> showFrameRenderCentreOffsetPt;
		Name showFrameTemplate;
		Range2 showFrameSafeAreaPt = Range2(Range(0.0f, 1.0f), Range(0.0f, 1.0f));

		Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("Framework.BullshotSystem.lock"));
		LogInfoContext history;
			
		SimpleVariableStorage variables;

		List<BullshotObject> objects;
		CACHED_ List<SafePtr<Framework::IModulesOwner>> createdObjects;

		List<BullshotCell> cells;
		
		List<BullshotLogo> logos;

		List<BullshotTitle> titles;
		int titleIdx = NONE; // change by key
		float titleTime = 0.0f;

		struct VRBullshot
		{
			Optional<Transform> headBoneOS;
			Optional<Transform> handLeftBoneOS;
			Optional<Transform> handRightBoneOS;
		} vr;

		struct ShowGameModifiers
		{
			bool show = false;
			Vector2 atPt = Vector2::half;
			float sizePt = 0.2f;
			float spacingPt = 0.3f;
			Vector2 dir = Vector2::yAxis;
			float speedPt = 0.5f;

			Colour backgroundColour = Colour::none;

			Array<UsedLibraryStored<TexturePart>> textures;
			Array<UsedLibraryStored<TexturePart>> available;
			struct Element
			{
				UsedLibraryStored<TexturePart> show;
				float alongDir = 0.0f;
			};
			Array<Element> elements;
		} showGameModifiers;

		CACHED_ SafePtr<Room> cameraInRoomPtr;
		TagCondition cameraInRoom;
		Optional<Transform> cameraPlacement;
		Optional<float> cameraFOV;
		bool cameraConsumed = false; // to load on init, later you have to reset

		CACHED_ SafePtr<Room> soundCameraInRoomPtr;
		TagCondition soundCameraInRoom;
		Optional<Transform> soundCameraPlacement;
		bool soundCamera = false;

		Optional<float> nearZ;

	};

};