#pragma once

#include "..\ac_particlesContinuous.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class EmitterData;

			/**
			 *	
			 */
			class Emitter
			: public ParticlesContinuous
			{
				FAST_CAST_DECLARE(Emitter);
				FAST_CAST_BASE(ParticlesContinuous);
				FAST_CAST_END();

				typedef ParticlesContinuous base;
			public:
				Emitter(EmitterData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / emitter"); }

			protected: // ParticlesMeshParticles
				override_ void internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle = nullptr);

			private:
				EmitterData const * emitterData;
			};

			class EmitterData
			: public ParticlesContinuousData
			{
				FAST_CAST_DECLARE(EmitterData);
				FAST_CAST_BASE(ParticlesContinuousData);
				FAST_CAST_END();

				typedef ParticlesContinuousData base;
			public:
				static void register_itself();

				EmitterData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				float useGravity = 0.0f;
				float timeAlive = 0.5f;
				float timeAliveVarietyCoef = 0.2f;
				float slowDownTime = 0.4f; // if zero, not used
				Range initialDelay = Range::zero; // initial delay, might be negative - will be clamped to 0 then
				Range timeBetween = Range::zero;
				Range initialRadius = Range::zero;
				Range3 initialRelativeLocOffset = Range3::zero;
				Range3 initialVelocityVector = Range3(Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(-1.0f, 1.0f));
				bool initialVelocityRelativeToParticlePlacement = false;
				Range initialSpeed = Range(1.3f, 1.3f);
				float slowSpeed = 0.01f;
				float initialOrientationMaxSpeed = 90.0f;
				Rotator3 randomiseOrientation = Rotator3(1.0f, 1.0f, 1.0f);
				Range3 randomiseOrientationAngle = Range3::zero;
				bool randomiseRotationRelativeToParticlePlacement = false;

				Name playSoundDetachedOnParticleActivate;

				static AppearanceControllerData* create_data();
				
				friend class Emitter;
			};

		};
	};
};
