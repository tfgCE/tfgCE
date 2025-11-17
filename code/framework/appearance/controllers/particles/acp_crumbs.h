#pragma once

#include "..\ac_particlesContinuous.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class CrumbsData;

			/**
			 *	Each particle is left after specific time period. Should be used within room.
			 *	Should be continuous!
			 */
			class Crumbs
			: public ParticlesContinuous
			{
				FAST_CAST_DECLARE(Crumbs);
				FAST_CAST_BASE(ParticlesContinuous);
				FAST_CAST_END();

				typedef ParticlesContinuous base;
			public:
				Crumbs(CrumbsData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / crumbs"); }

			public: // ParticlesBase
				override_ void desire_to_deactivate();

			protected: // ParticlesMeshParticles
				override_ void internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle = nullptr);

			private:
				CrumbsData const * crumbsData;

				// will start to disappear quicker from the back/the last one
				struct QuickerFallBack
				{
					bool active = false;
					float timeBeingAlive = 0.0f; // how long is it alive, to have a common reference point with particles' timeBeingAlive
					float catchUpTime = 0.0f; // to only catch up to a certain amount of particles
					float activeTime = 0.0f; // to calculate how precisely it should lower
				} quickerFallBack;

				float timeToCrumb = 0.0f;
				float distanceToCrumb = 0.0f;
				Optional<Vector3> lastLocWS;

				float timeNotContinuous = 0.0f;
			};

			class CrumbsData
			: public ParticlesContinuousData
			{
				FAST_CAST_DECLARE(CrumbsData);
				FAST_CAST_BASE(ParticlesContinuousData);
				FAST_CAST_END();

				typedef ParticlesContinuousData base;
			public:
				static void register_itself();

				CrumbsData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				struct QuickerFallBack
				{
					float catchUpTime = 0.0f; // when starts to get activated
					float catchUpSpeed = 1.0f; // to speed up a bit?
					float catchUpSpeedBeyondLimit = 0.0f;
					float catchUpTimeLimit = 0.0f; // to limit growing if exceeds this value
					float fallBackTime = 0.0f; // has to be defined to function
				} quickerFallBack;
				float timeAlive = 0.5f;
				float timeAliveVarietyCoef = 0.0f;
				float keepActivatingParticlesTime = 0.0f; // even if falling back will still activate particles with just less time
				Optional<Range> crumbPeriod;
				Optional<Range> crumbDistance;
				Rotator3 initialOrientationVelocity = Rotator3(0.0f, 0.0f, 0.0f);
				Rotator3 initialOrientationOff = Rotator3(0.0f, 0.0f, 0.0f); // locally 
				bool alignCrumbsToMovement = false;
				
				static AppearanceControllerData* create_data();
				
				friend class Crumbs;
			};

		};
	};
};
