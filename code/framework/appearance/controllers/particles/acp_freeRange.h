#pragma once

#include "..\ac_particlesMeshParticles.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class FreeRangeData;

			/**
			 *	
			 */
			class FreeRange
			: public ParticlesMeshParticles
			{
				FAST_CAST_DECLARE(FreeRange);
				FAST_CAST_BASE(ParticlesMeshParticles);
				FAST_CAST_END();

				typedef ParticlesMeshParticles base;
			public:
				FreeRange(FreeRangeData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / free range"); }

			private:
				FreeRangeData const * freeRangeData;

				float fovLimit = 0.0f;

				void setup_particle(Particle* _particle, Transform const & _ownerPlacement, bool _atMaxDistance = false);
			};

			class FreeRangeData
			: public ParticlesMeshParticlesData
			{
				FAST_CAST_DECLARE(FreeRangeData);
				FAST_CAST_BASE(ParticlesMeshParticlesData);
				FAST_CAST_END();

				typedef ParticlesMeshParticlesData base;
			public:
				static void register_itself();

				FreeRangeData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				float useGravity = 0.0f;
				float maxDistance = 5.0f;
				Range3 initialVelocityVector = Range3(Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(-1.0f, 1.0f));
				Range speed = Range(1.0f, 1.0f);
				Range3 initialVelocityOrientation = Range3(Range(-20.0f, 20.0f), Range(-20.0f, 20.0f), Range(-20.0f, 20.0f));
				float fovLimit = 0.0f;
				bool fovLimitFromVR = false;

				static AppearanceControllerData* create_data();
				
				friend class FreeRange;
			};

		};
	};
};
