#pragma once

#include "..\..\library\customLibraryStoredData.h"
#include "..\..\library\libraryLoadingContext.h"
#include "..\..\library\libraryPrepareContext.h"
#include "..\..\meshGen\meshGenParam.h"
#include "..\..\presence\relativeToPresencePlacement.h"
#include "..\..\sound\soundSource.h"

#include "..\..\..\core\containers\map.h"
#include "..\..\..\core\debug\extendedDebug.h"
#include "..\..\..\core\math\math.h"
#include "..\..\..\core\memory\refCountObject.h"
#include "..\..\..\core\random\random.h"

namespace Meshes
{
	struct Pose;
};

namespace Framework
{
	class ModuleAppearance;
	class PhysicalMaterial;

	namespace AppearanceControllersLib
	{
		/**
		 *	Various classes for Walker functionality
		 */
		namespace Walkers
		{
			struct Gait;
			struct GaitLeg;
			struct GaitOption;
			struct GaitPlayback;
			struct Instance;
			struct InstanceContext;
			class Lib;

			struct Event
			{
				enum Flags
				{
					E_Trigerred = 1,
					E_Started = 2,
					E_Active = 4, // inside
					E_Ended = 8,
				};
				float atPt = 0.0f;
				float length = 0.0f; // to stop
				Name name;

				bool load_from_xml(IO::XML::Node const * _node, tchar const * _nameAttr = TXT("id"));
				void be_full() { atPt = 0.0f; length = 1.0f; }

				int/*Flags*/ get_state(float _prevPt, float _newPt) const;
			};

			struct EventSound
			: public Event
			{
				typedef Event base;

				bool detach = false;

				bool load_from_xml(IO::XML::Node const * _node, tchar const * _nameAttr = TXT("id"));
			};

			struct Events
			{
				Array<EventSound> sounds;
				Array<EventSound> footStepSounds; // they are just played, should not be looped, always detached, not using sound socket

				bool load_from_xml(IO::XML::Node const * _node);
			};

			struct EventSoundInstance
			{
				EventSound const * source = nullptr;
				SoundSourcePtr sound;

				EventSoundInstance() {}
				EventSoundInstance(EventSound const * _source, SoundSource* _sound) : source(_source), sound(_sound) {}
			};

			struct EventsInstance
			{
				Events const * events = nullptr;
				Array<EventSoundInstance> sounds;

				void do_foot_step(Instance & _instance, InstanceContext const & _context, Events const * _events, RelativeToPresencePlacement const & _whereToPlay, PhysicalMaterial const * _hitMaterial);
				void advance(Instance & _instance, InstanceContext const & _context, Events const * _events, float _prevPt, float _currPt, bool _justSet, Optional<Name> _soundSocket);
				void stop_sounds();
			};

			struct CustomGaitVar
			{
				Name varID;
				float value = 0.0f;
				void const * source = nullptr; // to check if sources have changed
				float blendTimeLeft = 0.0f;

				CustomGaitVar() {}
				CustomGaitVar(Name const & _varID, float _value, void const * _source) : varID(_varID), value(_value), source(_source) {}

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			};

			/**
			 *	How actual trajectory looks like. Provided as placements, used as offsets (offset's Y is always 0, as we use transform_pt_to_pt_traj)
			 */
			struct TrajectoryFrame
			{
				float atTime = 0.0f; // as loaded
				Transform placement = Transform::identity;
				Array<CustomGaitVar> gaitVars; // should be in same order for all frames

				CACHED_ float atPT; // relative to length
				CACHED_ float invToNextPT;
				CACHED_ Transform offset; // without Y component
				CACHED_ float ptTraj; // along trajectory

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				// use methods below to make sure gaitVars are in order
				void insert_or_update(CustomGaitVar const & _gaitVar);
				CustomGaitVar & make_sure_exists(Name const & _varID); // creates if doesn't exist or returns existing
				bool has_gait_var(Name const & _varID) const;
				CustomGaitVar const * get_gait_var(Name const & _varID) const;
			};

			/**
			 *	Is single movement of leg.
			 *	Defines how long does it take to move there, what path does leg use.
			 *	Because it is quite abstract, it might be used by different gaits (different speeds etc).
			 *	Length is abstract as it is related to TrajectoryTrack length.
			 *	From that, during preparation proper leg movement is created.
			 */
			struct Trajectory
			: public RefCountObject
			{
			public:
				Trajectory();

				Events const & get_events() const { return events; }
				float get_foot_step_at_pt() const { return footStepAtPt; }

				Name const & get_name() const { return name; }
				bool is_relative_to_movement_direction() const { return isRelativeToMovementDirection; }

				float transform_pt_to_pt_traj(float _atPT) const;
				Transform calculate_offset_placement_at_pt(float _atPT, float _forTrajectoryObjectSize, Rotator3 const & _rotateBy) const;
				void calculate_and_add_gait_vars(float _atPT, Array<CustomGaitVar> & _gaitVars, Map<Name, Name> const & _translateNames) const;

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			private:
				Name name;
				bool isRelativeToMovementDirection = false;
				float length; // just to know in what time frame do we have trajectory defined (default = 1.0)
				float trajectoryObjectSize; // for how big objects this trajectory is created for (default = uses walker lib)
				
				Array<TrajectoryFrame> frames;
				Optional<float> footStepAtTime;
				CACHED_ float footStepAtPt = 1.0f;

				Events events;

				void insert(TrajectoryFrame const & _frame);

				inline int find_frame_index(float _atPT) const;

				friend class Lib;
			};

			/**
			 *	Helps TrajectoryTrack to define when trajectory starts, which one should be used etc.
			 */
			struct TrajectoryTrackElement
			{
			public:
				TrajectoryTrackElement();

				float get_starts_at() const { return startsAt; }
				float get_length() const { return length; }
				Trajectory* find_trajectory(Lib const * _lib) const;

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			private:
				float startsAt; // in some abstract time units
				float length; // in some abstract time units
				Name useTrajectory; // if invalid, uses own
				RefCountObjectPtr<Trajectory> trajectory;

				bool has_own_trajectory() const { return !useTrajectory.is_valid(); }
			};

			/**
			 *	Trajectory track is set of trajectories placed on a track of given length
			 *	As each trajectory just defines single leg movement, track tells how they are put relatively.
			 */
			struct TrajectoryTrack
			: public RefCountObject
			{
			public:
				TrajectoryTrack();

				Name const & get_name() const { return name; }
				float get_length() const { return length; }
				Array<TrajectoryTrackElement> const & get_elements() const { return elements; }

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			private:
				Name name;
				float length; // in some abstract time units
				Array<TrajectoryTrackElement> elements;
			};

			/**
			 *	Has offsets applied.
			 *	Has placement times calculated.
			 */
			struct GaitLegMovement
			{
				int elementIndex; // works as id
				float startsAt; // with offset applied
				float endsAt; // endsAt >= startsAt    <- ! always !
				float endPlacementAt; // placement for end location (where leg should be placed when we hit "endsAt")     endPlacementAt >= endsAt    <- ! always !
				float gaitLength;
				Trajectory* trajectory;

				GaitLegMovement();

				void adjust_to_contain(float _time); // adjusts to have _time between startsAt and endsAt (using gaitLength, if gaitLength is 0, it just moves to have startsAt at _time)
				void apply_offset(float _offset);

				float calculate_clamped_prop_through(float _time) const;
				inline float get_time_left(GaitPlayback const & _playback) const;
				inline float get_time_left_to_end_placement(GaitPlayback const & _playback) const;

			private:
				inline float calculate_offset_to_contain(float _time) const;
			};

			/**
			 *	This is set for each leg.
			 *	It defines how leg moves within gait.
			 *	All legs can share one trajectory set just with different offsets.
			 *
			 *	When preparing, use trajectories to create leg movements
			 *	Leg movements should know when they start, when they end etc.
			 *	Each leg movement should also know where leg should land:
			 *	from which point in animation it should take leg position
			 *	that point should be in the middle between each leg movement
			 *	and this is because we assume gaits are loops
			 */
			struct GaitLeg
			{
			public:
				GaitLeg();

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, float _gaitLength);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				// prepare leg movement
				bool bake(Lib const & _lib, Gait & _forGait);

				Array<GaitLegMovement> const & get_movements() const { return movements; }

			private:
				float offset; // offset (not pt!)
				Name useTrajectoryTrack; // if invalid, uses own
				RefCountObjectPtr<TrajectoryTrack> trajectoryTrack;

				// baked:
				Array<GaitLegMovement> movements; // they are placed in such manner that they cover whole 0 -> length (those at loop/boundary are doubled with offset)

				bool has_own_trajectory_track() const { return !useTrajectoryTrack.is_valid(); }

				void insert(GaitLegMovement const & _movement);
			};

			struct Gait
			: public RefCountObject
			{
			public:
				Gait();

				Events const & get_events() const { return events; }
				
				Name const & get_name() const { return name; }
				Tags const & get_sync_tags() const { return syncTags; }
				Optional<bool> const & get_does_allow_adjustments() const { return allowAdjustments; }
				float get_length() const { return length; }
				float get_trajectory_object_size() const { return trajectoryObjectSize; }
				Array<GaitLeg> const & get_legs() const { return legs; }
				Range const & get_playback_speed_range() const { return playbackSpeedRange; }
				float get_speed_XY_for_normal_playback_speed() const { return speedXYForNormalPlaybackSpeed; }
				Optional<Range> const & get_speeds_XY_for_playback_speed_range() const { return speedsXYForPlaybackSpeedRange; }
				bool get_turn_speeds_yaw_and_playback_speed_ranges(OUT_ Range & _turnSpeedsYaw, OUT_ Range & _playbackSpeedRanges) const { _turnSpeedsYaw = turnSpeedsYawForPlaybackSpeed_turnSpeedRange; _playbackSpeedRanges = turnSpeedsYawForPlaybackSpeed_playbackSpeedRange; return useTurnsSpeedsYawForPlaybackSpeed; }

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				// prepare leg movement
				bool bake(Lib const & _lib);

				GaitLegMovement const * find_movement(int _legIndex, float _atTime, float _ptBeforeEnd, float _timeBeforeEnd) const; // _timeBeforeEnd, _ptBeforeEnd - whatever is smaller of both

			private:
				Name name;
				Tags syncTags; // if both gaits share at least one tag, they are synced together
				Optional<bool> allowAdjustments; // allow adjustments when on ground (only allows, distance is not available per gait here)
				float length = 0.0f; // if 0, automatically calculated from legs
				float trajectoryObjectSize = 1.0f; // to allow modification per gait, by default uses walker lib
				Range playbackSpeedRange = Range(1.0f, 1.0f);
				float speedXYForNormalPlaybackSpeed = 0.0f; // playback speed is adjusted only if speed is non zero
				Optional<Range> speedsXYForPlaybackSpeedRange; // only if defined, above parameter is still applied but is interpolated to both extremes
				Range turnSpeedsYawForPlaybackSpeed_turnSpeedRange = Range(0.0f, 180.0f);
				Range turnSpeedsYawForPlaybackSpeed_playbackSpeedRange = Range(0.5f, 1.5f);
				bool useTurnsSpeedsYawForPlaybackSpeed = false;
				Array<GaitLeg> legs;

				Events events;

				friend class Lib;
			};

			/**
			 *	Gait playback structure that makes it easier to sync with main or to just go with current one
			 */
			struct GaitPlayback
			{
			public:
				GaitPlayback();

				void switch_to(Gait const * _newGait, GaitOption const * _useOption = nullptr);
				void dont_sync_to() { sync_to(nullptr); }
				void sync_to(GaitPlayback const * _other);

				void advance(float _deltaTime);
				// TODO syncing to external playback info?

				Gait const * get_gait() const { return gait; }
				Name const & get_gait_name() const { return gait ? gait->get_name() : Name::invalid(); }
				float get_at_time() const { return atTime; }
				float get_prev_time() const { return prevTime; }
				float get_pt() const { return gait ? atTime / gait->get_length() : 0.0f; }
				float get_prev_pt() const { return gait ? prevTime / gait->get_length() : 0.0f; }
				float get_playback_speed() const { return playbackSpeed; }
				void set_playback_speed(float _playbackSpeed) { playbackSpeed = _playbackSpeed; }

			private:
				float atTime;
				float prevTime; // always lower than atTime
				float playbackSpeed;
				Gait const * gait;
				GaitPlayback const * syncTo;
			};

			/**
			 *	Container for various gaits and trajectories. Can be put into custom data in library.
			 *	Can be included from libarary's custom datas.
			 */
			class Lib
			: public CustomLibraryData
			{
				FAST_CAST_DECLARE(Lib);
				FAST_CAST_BASE(CustomLibraryData);
				FAST_CAST_END();
				typedef CustomLibraryData base;
			public:
				bool bake();

				Gait* find_gait(Name const & _name) const;
				Trajectory* find_trajectory(Name const & _name) const;
				TrajectoryTrack* find_trajectory_track(Name const & _name) const;

			public: // CustomLibraryData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			private:
				Map<Name, RefCountObjectPtr<Gait>> gaits;
				Map<Name, RefCountObjectPtr<Trajectory>> trajectories;
				Map<Name, RefCountObjectPtr<TrajectoryTrack>> trajectoryTracks;

				Array<LibraryName> include; // including - if something already exists, it is not included, that's why include array is processed backwards. this means that include order in xml file is "base -> overrides"
				Array<LibraryName> alreadyIncluded;

				float trajectoryObjectSize = 1.0f;

				void sort_out_includes(Library* _library);
				void include_other(Lib const * _lib);
			};

			struct LegSetup
			{
				struct ForGait
				{
					struct Instance
					{
						// those are gait setups instanced per actor/object (to get actual leg lengths etc)
						ForGait const * gait = nullptr;
						Transform placementMS = Transform::identity; // cached or auto calculated or from socket
						Name placementSocket;
						Name placementSocketStopped;

						Instance() {}
						Instance(ForGait const * _gait, InstanceContext const & _instanceContext);

						Transform calculate_placement_MS(InstanceContext const & _instanceContext) const;
					};
					Transform placementMS = Transform::identity;
					Name placementSocket;
					Name placementSocketStopped;
					struct AutoPlacement
					{
						// will auto calculate placementMS, will drop onto xy plane (z==0)
						MeshGenParam<Name> bone0;
						MeshGenParam<Name> bone1;
						MeshGenParam<float> at = 0.0f;
						MeshGenParam<Vector3> location;
						MeshGenParam<Rotator3> orientation;
						AutoPlacement();
						bool update_placement_MS(REF_ Transform & _placementMS, InstanceContext const & _instanceContext) const;
						bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
						void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
					} autoPlacement;
					float disallowRedoLegMovementForTime = 0.0f; // for fast movers always allow adjusting
					float maxDifferenceOfOwnerToRedoLegMovement = 0.02f;
					float maxDifferenceOfLegToRedoLegMovement = 0.02f; // for zero it is ignored
					float allowSwitchingLegMovementPT = 0.3f; // allow changing leg movement if we finished 30% of current move
					Range dropDownTime = Range(0.05f, 0.08f); // combined with distance it should be very quick!
					float dropDownDistanceForMaxTime = 0.3f;
					Range fallingTime = Range(0.3f, 0.6f); // going from current into falling state - randomly
					Optional<bool> allowAdjustments; // either this or main gait has to allow
					MeshGenParam<float> adjustmentRequiredDistance;
					MeshGenParam<float> adjustmentRequiredVerticalDistance;
					MeshGenParam<float> adjustmentRequiredForwardAngle;
					Optional<float> adjustmentTime;
					Optional<Range> disallowAdjustmentMovementForTime;
					Optional<Range> disallowAdjustmentMovementWhenCollidingForTime;
					Array<CustomGaitVar> gaitVars;

					bool load_from_xml(IO::XML::Node const * _node, IO::XML::Node const* _mainNode, LibraryLoadingContext & _lc);
					void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
				};
				ForGait defaultGait;
				Map<Name, ForGait> perGait; // it tries to take 
				MeshGenParam<Name> boneForTraceStart; // will use preliminary pose for that
				MeshGenParam<Name> boneForLengthEnd; // used to calculate length
				MeshGenParam<Name> footBone; // used to get current placement 
				MeshGenParam<Name> footBaseSocket; // used to get current placement
				MeshGenParam<Name> soundSocket;
				float legLength = 0.0f; // if provided, will be used
				float collisionTraceMinAlt = -10000.0f;
				struct TrajectoryToAppearanceVar
				{
					Name trajectoryVar;
					Name appearanceVar;
					Optional<float> defaultValue;
					float sourceChangeBlendTime = 0.05f; // in case it switched source
				};
				Array<TrajectoryToAppearanceVar> trajectoryToAppearanceVars; // because trajectories may have some ... (some what? when you will learn to end sentences?)
				Map<Name, Name> translateVarFromTrajectoryToAppearance; // to make it easier

				Name useDropDownTrajectory; // if invalid, has own drop down trajectory (for breaking out of movement and dropping leg down)
				RefCountObjectPtr<Trajectory> dropDownTrajectory;
				Name useFallingTrajectory; // if invalid, has own falling down trajectory (for breaking out of movement and dropping leg down)
				RefCountObjectPtr<Trajectory> fallingTrajectory;
				Name useAdjustmentTrajectory; // if invalid, has own adjustment trajectory (for moving leg when stationary but leg is further from default location)
				RefCountObjectPtr<Trajectory> adjustmentTrajectory;
				Optional<float> adjustmentTime;
				Optional<Range> disallowAdjustmentMovementForTime;
				Optional<Range> disallowAdjustmentMovementWhenCollidingForTime;

				bool load_from_xml(IO::XML::Node const * _node, IO::XML::Node const* _mainNode, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
				bool bake(Lib const & _lib);
				void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

				bool has_own_drop_down_trajectory() const { return !useDropDownTrajectory.is_valid(); }
				bool has_own_falling_trajectory() const { return !useFallingTrajectory.is_valid(); }
				bool has_own_adjustment_trajectory() const { return !useAdjustmentTrajectory.is_valid(); }

			private:
				bool load_trajectory_to_apperance_var_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			};

			namespace LegAction
			{
				enum Type
				{
					None,
					Down,
					Up,
					// TODO add "wait after put down?"
				};
			};

			namespace LegActionFlag
			{
				enum Flags
				{
					None = 0,
					DropDown = 1,
					ForceNewMovement = 2, // matters only when starting new leg movement, isn't stored
					RedoLegMovement = 4, // matters only when starting new leg movement, isn't stored
					Adjustment = 8,
					Falling = 16
				};
			};

			/**
			 *	describes what leg is now doing
			 *	If leg is up, it tries to do what main is doing, unless it is doing something else
			 *	If leg is down, it tries to follow what main is doing
			 */
			struct LegInstance 
			{
			public:
				LegInstance();

				bool is_active() const { return isActive; }
				void request_active(bool _requestActive) { isActiveRequested = _requestActive; }

				// first initialise, then use_setup
				void initialise(InstanceContext const & _instanceContext, Instance* _instance, int _legIndex);
				void use_setup(InstanceContext const & _instanceContext, LegSetup const * _legSetup);

				void restart(InstanceContext const & _instanceContext); // restart after a while of being non active

				void advance(InstanceContext const & _instanceContext, float _deltaTime);
				void stop_sounds();
				void on_changed_gait(InstanceContext const & _instanceContext, bool _forceDropDown);
				void calculate_leg_placements_MS(InstanceContext const & _instanceContext);

				LegAction::Type get_action() const { return action; }

				Transform const & get_placement_MS() const { return legPlacementMS; }
				Transform const & get_traj_placement_MS() const { return legTrajPlacementMS; }
				Vector3 const & get_trajectory_MS() const { return trajectoryMS; }
				Array<CustomGaitVar> const & get_gait_vars() const { return gaitVars; }

				Transform calculate_end_traj_placement_os() const { return endTrajPlacement.calculate_placement_in_os(); }
				bool is_end_traj_placement_valid() const { return endTrajPlacementValid; }

				LegSetup::ForGait::Instance const * get_current_gait_instance() const { return get_for_gait_instance(currentDefault); }

			private:
				Instance* instance;
				
				// setup
				int legIndex;
				int boneForTraceStartIndex;
				int boneForLengthEndIndex;
				int footBoneIndex;
				Optional<Name> footBaseSocket;
				Optional<Name> soundSocket;
				float legLength;
				float collisionTraceMinAlt;
				LegSetup const * legSetup;
				Array<LegSetup::ForGait::Instance> gaitSetups;
				mutable LegSetup::ForGait::Instance const * recentGaitSetup = nullptr; // to speed up getting same one

				// current
				bool isActiveRequested = true;
				bool isActive = true;
				LegSetup::ForGait const * currentDefault;
				Name legMovementDoneForGait; // this is from instance to know what we were doing at top level

				LegAction::Type action = LegAction::None;
				bool footStepDone = false; // 
				int actionFlags;
				bool isFalling = false;
				GaitPlayback gaitPlayback;
				GaitLegMovement movement; // makes sense only if leg is up, is meant to always contain atTime - this means that if we loop, we adjust movement by moving it back by gaitLength
				float legMovementDirection = 0.0f; // relative to owner, stored at beginning of movement
				// placements for trajectories (from here we take offsets)
				float startTrajPlacementAtPTTraj; // if start traj placement is not placed at beginning but in the middle, this value tells where it is placed
				RelativeToPresencePlacement startTrajPlacement;
				RelativeToPresencePlacement endTrajPlacement;
				bool endTrajPlacementValid = false; // did it hit anything?
				PhysicalMaterial const * endTrajPhysicalMaterial = nullptr;
				float redoLegMovementDisallowedForTime;
				float adjustmentMovementDisallowedForTime;
				float adjustmentMovementDisallowedWhenCollidingForTime;

				RelativeToPresencePlacement endTrajPlacementDoneForPredictedOwnerPlacement; // when we were calculating end traj placement we were thinking that we (owner) will be at this placement
				RelativeToPresencePlacement endTrajPlacementDoneForPredictedLegPlacementBeforeTraces; // as above, but for leg - prediction where leg should be (before traces)

				Optional<Transform> defaultLegPlacementForAdjustmentMS; // this is to force adjusting if default has changed (as walker placement may change for many reasons)
				Optional<Transform> legPlacementAfterAdjustmentMS; // this is to avoid readjusting
				Name legAdjustedForGait; // this is compared to legMovementDoneForGait

				EventsInstance events;

				// output
				Transform legPlacementMS;
				Transform legTrajPlacementMS;
				Vector3 trajectoryMS; // start to end
				Array<CustomGaitVar> gaitVars;
				Vector3 legPlacementVelocityMS = Vector3::zero; // to maintain speed a little when dropping down - doesn't have to be accurate

#ifdef AN_ALLOW_EXTENDED_DEBUG
				bool legPlacementMSCalculatedInThisFrame = false;
#endif

				bool is_setup_correctly() const { return instance != nullptr && legIndex != NONE; }

				void set_start_traj_placement_to_current(InstanceContext const & _instanceContext);

				void put_leg_down(InstanceContext const & _instanceContext);
				void start_leg_movement(InstanceContext const & _instanceContext, GaitLegMovement const & _newMovement, int _actionFlags);
				void start_drop_down_movement(InstanceContext const & _instanceContext);
				void start_adjustment_movement(InstanceContext const & _instanceContext);
				void start_for_falling_movement(InstanceContext const & _instanceContext);

				void do_foot_step(InstanceContext const & _instanceContext);

				// current according to current gait
				inline LegSetup::ForGait const & get_current_default(InstanceContext const & _instanceContext) const;

				void find_end_placement(InstanceContext const & _instanceContext, bool _forDebug = false);
				void calculate_end_placements(InstanceContext const & _instanceContext, REF_ Transform & _endPlacementOfOwnerWS, REF_ Transform & _endPlacementOfLegWS) const;

				void update_gait_vars(InstanceContext const & _instanceContext, float _deltaTime);

				float calculate_adjustment_time() const;
				float calculate_disallow_adjustment_time() const;
				float calculate_disallow_adjustment_time_when_colliding() const;

				LegSetup::ForGait::Instance const * get_for_gait_instance(LegSetup::ForGait const* _gait) const;
			};

			struct InstanceSetup
			{
				MeshGenParam<float> playbackRate = 1.0f; // used when calculating playback rate using speed linear and turning
				MeshGenParam<float> predictionOffset = 0.0f; // additional time when predicting where do we want to be
				MeshGenParam<float> trajectoryObjectSize = 1.0f;
				MeshGenParam<float> disallowGaitChangingForTime = 0.2f;
				MeshGenParam<float> playbackSpeedUnitBlendTime = 0.1f;
				MeshGenParam<Name> alignWithGravityType;
				MeshGenParam<float> matchGravityOrientationSpeed;
				MeshGenParam<float> matchLocationXY = 0.0f; // will try to move so the body follows legs
				MeshGenParam<float> matchLocationZ = 0.0f;
				MeshGenParam<float> useHitNormalForPlacement = 1.0f; // if 1.0 will use full normal from hit, if 0.0 will use from OS

				MeshGenParam<bool> gravityPresenceTracesTouchSurroundingsRequired = true; // will internally activate/deactivate

				// adjustments importance (if set): this < gait < leg in a gait
				MeshGenParam<bool> allowAdjustments = false; // allow adjustments when on ground
				MeshGenParam<float> adjustmentTime = 0.2f;
				MeshGenParam<Range> disallowAdjustmentMovementForTime = Range(0.2f);
				MeshGenParam<Range> disallowAdjustmentMovementWhenCollidingForTime = Range(0.025f); // is capped at this value if colliding, updated on time limit

				MeshGenParam<float> adjustmentRequiredDistance = 0.05f;
				MeshGenParam<float> adjustmentRequiredVerticalDistance;
				MeshGenParam<float> adjustmentRequiredForwardAngle;

				MeshGenParam<float> nonAlignedGravitySlideDotThreshold = 0.7f; // check CheckCollisionContext - this is to prevent landing on walls and to slide down to the floor, used only if movement uses gravity

				struct WhereIWillBe
				{
					MeshGenParam<float> useRelativeRequestedMovementDirection = 0.0f;
					MeshGenParam<float> useLocomotionWhereIWantToBe = 0.2f;
				} whereIWillBe;

				InstanceSetup();

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
				void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
			};

			struct Instance
			{
			public:
				// add legs, each leg knows which movement it does
				// when we swich gait, we have to switch leg movement too
				// each leg has to know where movement has started, where it leads
				// what about emergency leg movement? we should have those too
				Instance();

				void setup(InstanceContext const & _instanceContext, int _legCount, InstanceSetup const & _instanceSetup);
				LegInstance const & get_leg(int _legIndex) const { return legs[_legIndex]; }
				LegInstance & access_leg(int _legIndex) { return legs[_legIndex]; }

				void set_requested_gait(Gait const * _requested, GaitOption const & _asOption) { requested = _requested; requestedAsOption = &_asOption; }
				void advance(InstanceContext const & _instanceContext, float _deltaTime);
				void stop_sounds();

				GaitPlayback const & get_gait_playback() const { return gaitPlayback; }
				Gait const * get_current_gait() const { return gaitPlayback.get_gait(); }
				Name const & get_current_gait_name() const { return gaitPlayback.get_gait() ? gaitPlayback.get_gait()->get_name() : Name::invalid(); }
				float get_trajectory_object_size() const { return instanceSetup->trajectoryObjectSize.get(); }
				float get_playback_rate() const { return instanceSetup->playbackRate.get(); }
				float get_prediction_offset() const { return instanceSetup->predictionOffset.get(); }				
				InstanceSetup const * get_setup() const { return instanceSetup; }

				bool does_adjust_to_gravity() const { return adjustsToGravity; }
				bool should_fall() const { return shouldFall; }
				Optional<float> const& get_preferred_gravity_alignment() const { return preferredGravityAlignment; }

			public: // legs
				int get_legs_up() const { return legsUpCount; }
				bool can_leg_adjust(int _legIndex) const { return _legIndex == legAllowedToAdjust && adjustingLegsCount < adjustingLegsLimit && legsUpCount == adjustingLegsCount; }
				void start_leg_adjustment() { an_assert(adjustingLegsCount < adjustingLegsLimit); ++adjustingLegsCount; }
				void stop_leg_adjustment() { an_assert(adjustingLegsCount > 0);  --adjustingLegsCount; }

				void mark_leg_up(int _legIndex);
				void mark_leg_down(int _legIndex);

			private:
				Gait const * requested;
				GaitOption const * requestedAsOption;
				GaitPlayback gaitPlayback;
				Name currentGaitReason; // stored from option - may force restart if reason is different
				Array<LegInstance> legs;
				InstanceSetup const * instanceSetup;
				float gaitChangingDisallowedForTime;
				int legAllowedToAdjust;
				int adjustingLegsCount; // to only allow specific number of legs adjusting in the same time
				int adjustingLegsLimit;
				int legsUpCount;
				int legsUpBits;
				CACHED_ bool adjustsToGravity = false;
				CACHED_ Optional<float> preferredGravityAlignment; // 1 same dir, -1 opposite (as cos_deg)
				CACHED_ bool shouldFall = false; // if does not hold to anything, should fall
				CACHED_ int shouldFallFrames = 0; // if does not hold to anything, should fall

				EventsInstance events;

				void switch_to(InstanceContext const & _instanceContext, Gait const * _new, GaitOption const * _asOption);

				float calculate_preferred_gait_playback_speed(InstanceContext const & _instanceContext) const;
				void adjust_placement_to_leg_placement(InstanceContext const & _instanceContext, float _deltaTime);

				void count_legs_up();
			};

			struct InstanceContext
			{
			public:
				InstanceContext();

				ModuleAppearance * get_owner() const { return owningAppearance; }
				void set_owner(ModuleAppearance * _owner) { owningAppearance = _owner; }
				void set_instance(Instance * _instance) { instance = _instance; }

				void use_pose_LS(Meshes::Pose* _poseLS) { an_assert(!prepared, TXT("reset for new frame?")); poseLS = _poseLS; }
				Meshes::Pose* access_pose_LS() const { return poseLS; }
				Meshes::Pose const * get_pose_LS() const { return poseLS; }

				bool is_colliding_with_non_scenery() const { return collidingWithNonScenery; }
				void set_colliding_with_non_scenery(bool _collidingWithNonScenery) { collidingWithNonScenery = _collidingWithNonScenery; }

				bool do_gravity_presence_traces_touch_surroundings() const { return gravityPresenceTracesTouchSurroundings; }
				void set_gravity_presence_traces_touch_surroundings(bool _gravityPresenceTracesTouchSurroundings) { gravityPresenceTracesTouchSurroundings = _gravityPresenceTracesTouchSurroundings; }

				Optional<float> const & get_preferred_gravity_alignment() const { return preferredGravityAlignment; }
				Optional<Vector3> const & get_relative_velocity_linear() const { return relativeVelocityLinear; }
				Optional<float> const & get_relative_velocity_direction() const { return relativeVelocityDirection; }
				Optional<float> const & get_relative_requested_movement_direction() const { return relativeRequestedMovementDirection; }
				Optional<Rotator3> const & get_velocity_orientation() const { return velocityOrientation; }
				Optional<float> const & get_turn_speed() const { return turnSpeed; }
				Optional<float> const & get_speed() const { return speed; }
				Optional<float> const & get_speed_XY() const { return speedXY; }
				void set_preferred_gravity_alignment(Optional<float> const & _preferredGravityAlignment) { an_assert(!prepared, TXT("reset for new frame?")); preferredGravityAlignment = _preferredGravityAlignment; }
				void set_relative_velocity_linear(Vector3 const & _relativeVelocityLinear) { an_assert(!prepared, TXT("reset for new frame?")); relativeVelocityLinear = _relativeVelocityLinear; }
				void set_relative_requested_movement_direction(float _relativeRequestedMovementDirection) { an_assert(!prepared, TXT("reset for new frame?")); relativeRequestedMovementDirection = _relativeRequestedMovementDirection; }
				void set_velocity_orientation(Rotator3 const & _velocityOrientation) { an_assert(!prepared, TXT("reset for new frame?")); velocityOrientation = _velocityOrientation; }
				
				Optional<Name> const & get_current_gait() const { return currentGait; }
				void set_current_gait(Name const & _gait) { an_assert(!prepared, TXT("reset for new frame?")); currentGait = _gait; }

				Optional<Name> const & get_gait_name_requested_by_controller() const { return gaitNameRequestedByController; }
				void set_gait_name_requested_by_controller(Name const & _gait) { an_assert(!prepared, TXT("reset for new frame?")); gaitNameRequestedByController = _gait; }

				Random::Generator & access_random_generator() const { return randomGenerator; }

			public: // management
				void reset_for_new_frame();
				void mark_just_activated() { justActivated = true; }
				bool has_just_activated() const { return justActivated; }
				bool is_prepared() const { return prepared; }
				void prepare();

			public: // queries
				Transform where_will_I_be_in_MS(float _time) const; // predicts relative movement in MS
				Transform where_leg_will_be_in_MS(float _time, int _boneIndex, Transform const & _defaultPlacementMS) const;

			private:
				bool prepared;
				bool justActivated;
				ModuleAppearance * owningAppearance;
				Instance * instance;

				mutable Random::Generator randomGenerator;

				Meshes::Pose* poseLS;
				bool collidingWithNonScenery = false;
				bool gravityPresenceTracesTouchSurroundings = false;
				Optional<float> preferredGravityAlignment;
				Optional<Vector3> relativeVelocityLinear;
				Optional<float> relativeVelocityDirection;
				Optional<float> relativeRequestedMovementDirection;
				Optional<Rotator3> velocityOrientation;
				Optional<float> turnSpeed;
				Optional<float> speed;
				Optional<float> speedXY;
				Optional<Name> currentGait;
				Optional<Name> gaitNameRequestedByController;
			};

			struct GaitOption
			{
				Name gait;
				float startAtPT = 0.0f;
				Name reason;
				bool forceDropDown = false;

				static GaitOption invalid;

				bool is_valid() const { return gait.is_valid() || reason.is_valid(); }
				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			};

			/**
			 *	To disallow changing gaits in certain situations
			 */
			struct LockGait
			{
			public:
				LockGait();
				~LockGait();
				bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				static bool is_allowed(Name const& _currentGait, Name const& _requestedGait, Array<LockGait> const& _lockGaits, Walkers::Instance const & _walkerInstance, Walkers::InstanceContext const & _walkerInstanceContext);

			private:
				Array<Name> currentGait;
				Array<Name> requestedGait;
				RangeInt legsUp = RangeInt::empty;
			};

			/**
			 *	Simple tree that allows choosing appropriate (walker) gait
			 *
			 *	In future it should take gaits from animations - from whatever animation is playing (maybe even without name, just straight gait pointer?)
			 */
			struct ChooseGait
			{
			public:
				ChooseGait();
				~ChooseGait();
				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				GaitOption const & run_for(InstanceContext const & _context) const;

			private:
				Optional<Range> speed; // current
				Optional<Range> speedXY;
				Optional<bool> collidingWithNonScenery; // if valid, checks collision module if has same value (through context)
				Optional<bool> gravityPresenceTracesTouchSurroundings; // if valid, checks presence module if has same value (through context)
				Optional<Range> preferredGravityAlignment; // where preferred gravity alignment is (-1 to -0.7 - we're at least 45 deg off)
				Optional<Range> relativeRequestedMovementDirection; // in which (relative) direction does it move
				Optional<Range> differenceBetweenRequestedMovementDirectionAndVelocityDirection; // both relatives but that doesn't matters, checked only if both are non zero
				Optional<Range> turnSpeed; // turn speeed - may differentiate turn direction
				Optional<Range> absTurnSpeed; // absolute turn speed 
				Optional<Range> turnSpeedYaw; // turn speeed - may differentiate turn direction
				Optional<Range> absTurnSpeedYaw; // absolute turn speed 
				Optional<Range> turnSpeedPitch; // turn speeed - may differentiate turn direction
				Optional<Range> absTurnSpeedPitch; // absolute turn speed 
				Optional<Range> turnSpeedRoll; // turn speeed - may differentiate turn direction
				Optional<Range> absTurnSpeedRoll; // absolute turn speed 
				Optional<Name> gaitNameRequestedByController; // if valid, speed is not checked!
				bool ignoreSpeedWhenGaitNameRequestedByControllerSuccessful = false;
				Optional<Name> currentGait;
				Optional<bool> currentGaitSet;
				Array<GaitOption> chooseGaits;
				Array<ChooseGait> chooseOptions;
			};

			/**
			 *	Structure allowing to provide gaits back (as apperance allowed in controller)
			 *	This is used to translate walker gaits into controller gaits
			 */
			struct ProvideAllowedGait
			{
			public:
				ProvideAllowedGait();
				~ProvideAllowedGait();
				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

				Name const & get_provided(Name const & _gait) const;

			private:
				struct Element
				{
					Name gait; // if none, any gait is changed
					Name provideAs; // if none, is not changed but provided
				};
				Array<Element> elements;				
			};

			/**
			 *	Variables for each gait (not blended!)
			 */
			struct GaitVar
			{
			public:
				GaitVar() { SET_EXTRA_DEBUG_INFO(forGaits, TXT("Walkers::GaitVar.forGaits")); }
				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				Name const & get_var_id() const { return varID; }
				float get_value(Name const & _gait) const;

			private:
				struct ForGait
				{
					Name gait; // if none, any gait is changed
					float value = 0.0f;
				};
				Name varID;
				float defaultValue = 0.0f;
				ArrayStatic<ForGait, 16> forGaits;
			};

			#include "walkers.inl"
		};
	};
};
