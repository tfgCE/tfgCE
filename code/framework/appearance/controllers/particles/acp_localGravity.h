#pragma once

#include "..\ac_particlesContinuous.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class LocalGravityData;

			/**
			 *	
			 */
			class LocalGravity
			: public ParticlesContinuous
			{
				FAST_CAST_DECLARE(LocalGravity);
				FAST_CAST_BASE(ParticlesContinuous);
				FAST_CAST_END();

				typedef ParticlesContinuous base;
			public:
				LocalGravity(LocalGravityData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / local gravity"); }

			protected: // ParticlesContinuous
				override_ void internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle = nullptr);

			private:
				LocalGravityData const * localGravityData;

				Range initialDistance;
				Range3 initialPlacementOffset;
				Range initialPlacementAtYaw;
				Box initialPlacementBox; // already offset (axis aligned)
				Box initialPlacementBoxCollapsed; // to get centre of gravity
				Vector3 scaleInitialPlacement = Vector3::one;
			};

			class LocalGravityData
			: public ParticlesContinuousData
			{
				FAST_CAST_DECLARE(LocalGravityData);
				FAST_CAST_BASE(ParticlesContinuousData);
				FAST_CAST_END();

				typedef ParticlesContinuousData base;
			public:
				static void register_itself();

				LocalGravityData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				float localGravity = 0.5f;
				float maxSpeed = 0.0f;
				float timeAlive = 0.5f;
				float timeAliveVarietyCoef = 0.2f;
				Range3 initialVelocityVector = Range3(Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(-1.0f, 1.0f));
				Range initialStaySpeed = Range(0.9f, 0.9f); // stay speed is speed to stay in local-orbit (depends on local gravity
				Range initialDistance = Range(0.8f, 1.1f); // will be placed perpendicular to vector
				Range3 initialPlacementOffset = Range3::zero; // inside offset + distance perpendicular to velocity vector
				Vector3 scaleInitialPlacement = Vector3::one;
				bool useAttachedToForInitialPlacementOffset = false; // will use owner size for initial placement offset (and distance)
				Vector3 useAttachedToForInitialPlacementOffsetAmount = Vector3::one;
				Range useAttachedToForInitialPlacementOffsetRadiusPt = Range(1.5f); // > sqr
				float initialOrientationMaxSpeed = 90.0f;

				Range initialPlacementAtYaw = Range::empty; // if not empty, the particles will be placed all around (velocity rotated as well), will use just distance + Z
															// if you want to have particles going clockwise, use X > 0 and initialPlacementAtYaw

				static AppearanceControllerData* create_data();
				
				friend class LocalGravity;
			};

		};
	};
};
