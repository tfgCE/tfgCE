#pragma once

#include "..\..\core\collision\iCollidableObject.h"
#include "..\..\core\types\string.h"
#include "..\..\core\memory\safeObject.h"
#include "..\modulesOwner\modulesOwner.h"

#include "temporaryObjectType.h"

#include "..\debugSettings.h"

#ifdef AN_DEVELOPMENT
	#ifdef WITH_HISTORY_FOR_OBJECTS
		#ifndef WITH_TEMPORARY_OBJECT_HISTORY
			//#define WITH_TEMPORARY_OBJECT_HISTORY
		#endif
	#endif
	#ifndef WITH_TEMPORARY_OBJECT_HISTORY
		//#define WITH_TEMPORARY_OBJECT_HISTORY
	#endif
#endif

namespace Framework
{
	class World;
	class SubWorld;
	class ModuleAppearance;
	class ModulePresence;
	class ModuleGameplay;
	class ModuleSound;

	struct TemporaryObjectTypeInSubWorld;

	namespace TemporaryObjectState
	{
		enum Type
		{
			NotAdded,
			BeingInitialised, // between NotAdded and Inactive, when being initialised
			Inactive, // was deactivated
			ToBeActivated, // will be activated when temporary objects are processed
			Active, // is now active
			ToBeDeactivated, // ceased to be
		};

		inline bool is_available_for_spawn(TemporaryObjectState::Type _state) { return _state <= Inactive; }

		inline tchar const* to_char(Type s)
		{
			if (s == NotAdded) return TXT("not added");
			if (s == BeingInitialised) return TXT("being initialised");
			if (s == Inactive) return TXT("inactive");
			if (s == ToBeActivated) return TXT("to be activated");
			if (s == Active) return TXT("active");
			if (s == ToBeDeactivated) return TXT("to be deactivated");
			return TXT("<unknown>");
		}
	};

	/**
	 *	Call Init after creation.
	 *
	 *	Once created, it always exists within SubWorld
	 *	To get new TemporaryObject, call subWorld->get_one_for(temporaryObjectType, roomOfPlacement), when everything is setup, call temporaryObject->activate()
	 *	To remove TemporaryObject, call temporaryObject->deactivate()
	 */
	class TemporaryObject
	: public IModulesOwner // +IAIObject
	, public Collision::ICollidableObject
	{
		FAST_CAST_DECLARE(TemporaryObject);
		FAST_CAST_BASE(IModulesOwner);
		FAST_CAST_BASE(Collision::ICollidableObject);
		FAST_CAST_END();

		typedef IModulesOwner base;
	public:
		TemporaryObject(TemporaryObjectType const * _temporaryObjectType); // requires initialisation: init()
		virtual ~TemporaryObject(); // should use destroy()

		void destruct_and_delete() { on_destruct(); delete this; }

		bool does_require_reset() const { return requiresReset; }

		bool is_active() const { return state == TemporaryObjectState::Active; } // contrary to standard Object, activation differs at SubWorld level and Room level (Room is same, SubWorld is "where do we have it")
		void mark_to_activate(); // add to sub world
		void mark_to_deactivate(bool _removeFromWorldImmediately = true); // remove from subworld
		TemporaryObjectState::Type get_state() const { return state; }

		Optional<Vector3> const& get_initial_additional_absolute_velocity() const { return onActivationParams.addAbsoluteVelocity; } // this is useful to align projectiles properly

	public:
		void desire_to_deactivate(); // will stop sounds, desired to deactivate particles etc
		bool is_desired_to_deactivate() const { return desiredToDeactivate; }
		void mark_desired_to_deactivate(bool _desiredToDeactivate = true) { desiredToDeactivate = _desiredToDeactivate; } // ideally only utils should set this

	public:

		void learn_from(SimpleVariableStorage & _parameters); // not const as it should be possible to modify this storage (for any reason)

		TemporaryObjectType const * get_object_type() const { return objectType; }
		String const & get_name() const { return name; }

#ifdef WITH_TEMPORARY_OBJECT_HISTORY
		void store_history(String const& _history);
#endif

#ifdef AN_ALLOW_BULLSHOTS
		void set_bullshot_allow_advance_for(float _time) { bullshotAllowAdvanceFor = _time; }
		void clear_bullshot_allow_advance_for() { bullshotAllowAdvanceFor.clear(); }
		Optional<float> & access_bullshot_allow_advance_for() { return bullshotAllowAdvanceFor; }
#endif

	public: // activation setup
		void on_activate_place_in(Room* _room, Transform const & _placement);
		void on_activate_place_in_fail_safe(Room* _room, Transform const & _placement);
		void on_activate_place_at(IModulesOwner* _object, Name const & _socketName = Name::invalid(), Transform const & _relativePlacement = Transform::identity);
		void on_activate_add_absolute_location_offset(Vector3 const& _locationOffset);
		void on_activate_face_towards(Vector3 const& _inRoomLocWS, Optional<Vector3> const & _upDirWS = NP);
		void on_activate_set_velocity(Vector3 const & _velocity);
		void on_activate_add_velocity(Vector3 const& _velocity);
		void on_activate_set_relative_velocity(Vector3 const & _velocity);
		void on_activate_set_velocity_rotation(Rotator3 const & _velocity);
		void on_activate_attach_to(IModulesOwner* _object, bool _following = false, Transform const & _relativePlacement = Transform::identity);
		void on_activate_attach_to_bone(IModulesOwner* _object, Name const & _bone, bool _following = false, Transform const & _relativePlacement = Transform::identity);
		void on_activate_attach_to_socket(IModulesOwner* _object, Name const & _socket, bool _following = false, Transform const & _relativePlacement = Transform::identity);

		// we require this as we activate temporary objects later in the frame, not immediately, and activation may change stuff
		void perform_on_activate(std::function<void(TemporaryObject* _to)> _performOnActivate);

	public: // IModulesOwner
		implement_ Name const & get_object_type_name() const;
		implement_ LibraryName const & get_library_name() const { return objectType ? objectType->get_name() : LibraryName::invalid(); }

		implement_ void cease_to_exist(bool _removeFromWorldImmediately) { mark_to_deactivate(_removeFromWorldImmediately); }

		implement_ void init(SubWorld* _subWorld); // initialise, called from SubWorld only! this also adds to inactive objects and requires locking temporary objects
		implement_ void on_destruct(); // close and remove

		implement_ bool does_allow_to_attach(IModulesOwner const * _imo) const { return fast_cast<TemporaryObject>(_imo) != nullptr; }
		implement_ bool should_do_calculate_final_pose_attachment_with(IModulesOwner const * _imo) const; //  we separate calculation of attached temporary objects from objects - first go objects alone and then temporary ones

#ifdef AN_PERFORMANCE_MEASURE
	public:
		implement_ String const & get_performance_context_info() const { return name; }
#endif

	public:
		void particles_activated();
		void particles_deactivated();
		void no_more_sounds();

	protected: // IModulesOwner
		override_ void reset();
#ifdef WITH_TEMPORARY_OBJECT_HISTORY
		override_ void ready_modules_to_activate(); // before object is activated but after it is placed
		override_ void activate_modules(); // when object is being activated
#endif
		override_ void initialise_modules(SetupModule _setup_module_pre = nullptr);

	protected: // SafeObject
		void make_safe_object_available(TemporaryObject* _object);
		void make_safe_object_unavailable();

	public: // IAIObject
		implement_ bool does_handle_ai_message(Name const& _messageName) const;
		implement_ AI::MessagesToProcess * access_ai_messages_to_process();
		implement_ String const & ai_get_name() const { return name; }
		implement_ Transform ai_get_placement() const;
		implement_ Room* ai_get_room() const;

	protected: friend class SubWorld;
		void set_state(TemporaryObjectState::Type _state);
		void set_subworld_entry(TemporaryObjectTypeInSubWorld * _subWorldEntry);

	private: // called internally
		void on_activate();
		void on_deactivate();

	private:
		TemporaryObjectType const * objectType;
		TemporaryObjectTypeInSubWorld * subWorldEntry = nullptr;

		TemporaryObjectState::Type state = TemporaryObjectState::NotAdded;
#ifdef WITH_TEMPORARY_OBJECT_HISTORY
		int numberOfTimesModulesReadiedToActivate = 0;
		int numberOfTimesModulesActivated = 0;
		int numberOfTimesModulesReset = 0;
		int numberOfTimesModulesResetUnnecessary = 0;
		int numberOfTimesActivated = 0;
		int numberOfTimesOnActivated = 0;
		Array<TemporaryObjectState::Type> stateHistory;
		Concurrency::SpinLock toHistoryLock = Concurrency::SpinLock(TXT("Framework.TemporaryObject.toHistoryLock"));
		Array<String> toHistory;
#endif

#ifdef AN_ALLOW_BULLSHOTS
		Optional<float> bullshotAllowAdvanceFor;
#endif

		bool initialised = false;
		bool requiresReset = false;
		bool desiredToDeactivate = false;

		int particlesActive = 0;

		String name;
		Tags tags;
		float individualityFactor;

		struct OnActivationParams
		{
			::SafePtr<Room> placeIn;
			Transform placement = Transform::identity;
			::SafePtr<Room> failSafePlaceIn;
			Transform failSafePlacement = Transform::identity;
			Transform relativePlacement = Transform::identity;
			Optional<Vector3> addAbsoluteLocationOffset; // world space, only if placed in the world, not if attached
			::SafePtr<IModulesOwner> placeAt;
			Name boneName;
			Name socketName;
			::SafePtr<IModulesOwner> attachTo;
			bool attachToFollowing = false;

			Optional<Vector3> faceTowardsInRoomLocWS;
			Optional<Vector3> faceTowardsUpDirWS;

			Optional<Vector3> absoluteVelocity;
			Optional<Vector3> addAbsoluteVelocity;
			Optional<Vector3> relativeVelocity;
			Optional<Rotator3> velocityRotation;
		} onActivationParams;

		ArrayStatic<std::function<void(TemporaryObject* to)>, 8> performOnActivate;

		void set_state_internal(TemporaryObjectState::Type _state);

		inline bool should_cease_to_exist() const;
	};
};
