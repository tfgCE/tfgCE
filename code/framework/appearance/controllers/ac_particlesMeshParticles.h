#pragma once

#include "ac_particlesBase.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace ParticlesPlacement
		{
			enum Type
			{
				Undefined, // should be provided by derived class
				OwnerPlacement,
				InstigatorPresence,
				InstigatorSkeleton, // within skeleton
				InstigatorBones, // precisely at bones
				CreatorPresence,
				CreatorSkeleton, // within skeleton
				CreatorBones, // precisely at bones
			};

			inline Type parse(String const & _value, Type _default = Undefined)
			{
				if (_value == TXT("ownerPlacement")) return OwnerPlacement;
				if (_value == TXT("instigatorPresence")) return InstigatorPresence;
				if (_value == TXT("instigatorSkeleton")) return InstigatorSkeleton;
				if (_value == TXT("instigatorBones")) return InstigatorBones;
				if (_value == TXT("creatorPresence")) return CreatorPresence;
				if (_value == TXT("creatorSkeleton")) return CreatorSkeleton;
				if (_value == TXT("creatorBones")) return CreatorBones;
				if (_value == TXT("none") ||
					_value == TXT("undefined")) return Undefined;
				if (_value == TXT("")) return _default;
				error(TXT("particles placement \"%S\" not recognised"), _value.to_char());
				return Undefined;
			}
		};

		namespace ParticlesScale
		{
			enum Type
			{
				Undefined, // should be provided by derived class or particles size should be used
				InstigatorPresenceSize,
				CreatorPresenceSize,
			};

			inline Type parse(String const & _value, Type _default = Undefined)
			{
				if (_value == TXT("instigatorPresenceSize")) return InstigatorPresenceSize;
				if (_value == TXT("creatorPresenceSize")) return CreatorPresenceSize;
				if (_value == TXT("none") ||
					_value == TXT("undefined")) return Undefined;
				if (_value == TXT("")) return _default;
				error(TXT("particles scale \"%S\" not recognised"), _value.to_char());
				return Undefined;
			}
		};

		class ParticlesMeshParticlesData;

		/**
		 *	
		 */
		class ParticlesMeshParticles
		: public ParticlesBase
		, public IPresenceObserver
		{
			FAST_CAST_DECLARE(ParticlesMeshParticles);
			FAST_CAST_BASE(ParticlesBase);
			FAST_CAST_BASE(IPresenceObserver);
			FAST_CAST_END();

			typedef ParticlesBase base;
		public:
			ParticlesMeshParticles(ParticlesMeshParticlesData const * _data);
			virtual ~ParticlesMeshParticles();

		public: // AppearanceController
			override_ void initialise(ModuleAppearance* _owner);
			override_ void activate();
			override_ void calculate_final_pose(REF_ AppearanceControllerPoseContext & _context);

			override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

		public: // IPresenceObserver
			implement_ void on_presence_changed_room(ModulePresence* _presence, Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors);
			implement_ void on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom * _enteredThroughDoor);
			implement_ void on_presence_removed_from_room(ModulePresence* _presence, Room* _room);
			implement_ void on_presence_destroy(ModulePresence* _presence);

		protected:
			Transform get_owner_placement_in_proper_space() const;

		protected:
			enum ActivationFlags
			{
				RequiresActivation = bit(1),
				MayActivate = bit(2),
				PendingInitialActivation = bit(3),

				ShouldActivate = RequiresActivation | MayActivate
			};
			struct Particle
			{
				int boneIndex = NONE;
				int activationFlags = RequiresActivation; // free to be activated
				float randomNumber = 0.0f;
				float timeBeingAlive = 0.0f;
				float alpha = 1.0f;
				float dissolve = 0.0f;
				float individualParticleScale = 1.0f;
				// in room space
				Transform placement;
				Vector3 velocityLinear = Vector3::zero;
				Rotator3 velocityRotation = Rotator3::zero;
				//
				float customData[16];
			};
			Array<Particle> particles;
			Array<Vector4> particleInfos; // as bone, x - alpha, y - dissolve, z - full scale, w - random value, due to gles handling uniform float arrays differently

			float useRoomVelocity = 0.0f;
			float useRoomVelocityInTime = 0.0f;

			// if in world space, may still try to follow owner for a short time
			float useOwnerMovement = 0.0f;
			float useOwnerMovementForTime = 0.0f;

			void activate_particle(Particle * firstParticle, Particle* endParticle = nullptr);
			virtual void internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle); // always place particles in WS! they will be moved into proper space

			float get_placement_separation() const { return placementSeparation; }

		private:
			ParticlesMeshParticlesData const * particlesMeshParticlesData;
			bool isValid = false;

			float placementOffRadiusCoef = 1.0f;
			Optional<float> placementOffAlongRadiusCoef;

			float placementSeparation = 0.0f;
			int minAmountSeparation = 1;

			float particlesScale = 1.0f;

			ModulePresence* observingPresence = nullptr;

			void stop_observing_presence();

			void move_from_ws_into_proper_space(Particle * firstParticle, Particle* endParticle);
		};

		class ParticlesMeshParticlesData
		: public ParticlesBaseData
		{
			FAST_CAST_DECLARE(ParticlesMeshParticlesData);
			FAST_CAST_BASE(ParticlesBaseData);
			FAST_CAST_END();

			typedef ParticlesBaseData base;
		public:
			bool is_in_local_space() const { return inLocalSpace; }
			bool is_in_vr_space() const { return inVRSpace; }
			bool should_align_to_velocity() const { return alignToVelocity; }
			bool should_scale_with_velocity() const { return scaleWithVelocity != 0.0f; }
			float get_scale_with_velocity() const { return scaleWithVelocity; }
			Range const & get_scale_with_velocity_range() const { return scaleWithVelocityRange; }
			Name const & get_bone_particle_type() const { return boneParticleType; }
			ParticlesPlacement::Type get_initial_placement() const { return initialPlacement; }
			float get_placement_off_radius_coef() const { return placementOffRadiusCoef.is_set()? placementOffRadiusCoef.get() : 0.0f; }
			Optional<float> get_placement_off_along_radius_coef() const { return placementOffAlongRadiusCoef.is_set()? Optional<float>(placementOffAlongRadiusCoef.get()) : NP; }
			int get_min_amount_separation() const { return minAmountSeparation.get(); }
			float get_placement_separation() const { return placementSeparation.is_set() ? placementSeparation.get() : 0.0f; }
			float get_placement_separation_ref() const { return placementSeparationRef.get(); }
			Range const & get_individual_particle_scale() const { return individualParticleScale.get(); }
			ParticlesScale::Type get_particles_scale_type() const { return particlesScaleType; }
			float get_particles_scale_ref() const { return particlesScaleRef.get(); }
			float get_particles_scale() const { return particlesScale.get() * particlesScaleMul.get(); }
			float get_particles_scale_mul() const { return particlesScaleMul.get(); }
			float get_particles_scale_pulse() const { return particlesScalePulse.get(); }
			float get_particles_scale_pulse_period() const { return particlesScalePulsePeriod.get(); }
			float get_use_room_velocity() const { return useRoomVelocity.get(); }
			float get_use_room_velocity_in_time() const { return useRoomVelocityInTime.get(); }
			float get_use_owner_movement() const { return useOwnerMovement.get(); }
			float get_use_owner_movement_for_time() const { return useOwnerMovementForTime.get(); }

		public: // AppearanceControllerData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);

		protected:
			bool inLocalSpace = false; // particles are in local space
			bool inVRSpace = false;
			bool alignToVelocity = false;
			float scaleWithVelocity = 0.0f;
			Range scaleWithVelocityRange = Range(0.0f, 1000.0f);
			ParticlesPlacement::Type initialPlacement = ParticlesPlacement::Undefined;
			MeshGenParam<float> placementOffRadiusCoef = 1.0f; // if using radius to place off centre, this is used
			MeshGenParam<float> placementOffAlongRadiusCoef; // along dir, used only if larger than placementOffRadiusCoef
			MeshGenParam<int> minAmountSeparation = 1; // minimal amount when using separation
			MeshGenParam<float> placementSeparation; // if non zero, will place evenly, if no place available, particle is not used
			MeshGenParam<float> placementSeparationRef = 0.0f; // to adjust separation to placement (presence|size / ref)
			MeshGenParam<Range> individualParticleScale = Range(1.0f);
			ParticlesScale::Type particlesScaleType = ParticlesScale::Undefined;
			MeshGenParam<float> particlesScale = 1.0f;
			MeshGenParam<float> particlesScaleMul = 1.0f;
			MeshGenParam<float> particlesScaleRef = 0.0f; // will get multiplied by size / ref
			MeshGenParam<float> particlesScalePulse = 0.0f; // how much does it differ
			MeshGenParam<float> particlesScalePulsePeriod = 0.0f;
			MeshGenParam<float> useRoomVelocity = 1.0f; // how much of room velocity is going to be used
			MeshGenParam<float> useRoomVelocityInTime = 1.0f; // in what time is it going to be used in 100%
			MeshGenParam<float> useOwnerMovement = 0.0f; // if WS will move with owner, doesn't work with LS and VR
			MeshGenParam<float> useOwnerMovementForTime = 1.0f; // for what time it is going to follow before not following at all

			Name boneParticleType;

			static AppearanceControllerData* create_data();
		};
	};
};
