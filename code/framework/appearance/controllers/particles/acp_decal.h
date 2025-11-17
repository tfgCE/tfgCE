#pragma once

#include "..\ac_particlesContinuous.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class DecalData;

			/**
			 *	Particles stay until told to deactivate. Should be continuous, by default is continuous.
			 */
			class Decal
			: public ParticlesContinuous
			{
				FAST_CAST_DECLARE(Decal);
				FAST_CAST_BASE(ParticlesContinuous);
				FAST_CAST_END();

				typedef ParticlesContinuous base;
			public:
				Decal(DecalData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / decal"); }

			protected: // ParticlesMeshParticles
				override_ void internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle = nullptr);

			public:
				override_ void desire_to_deactivate();

			private:
				DecalData const * decalData;

				bool shouldDeactivate = false;
				float activeDecal = 0.0f;
			};

			class DecalData
			: public ParticlesContinuousData
			{
				FAST_CAST_DECLARE(DecalData);
				FAST_CAST_BASE(ParticlesContinuousData);
				FAST_CAST_END();

				typedef ParticlesContinuousData base;
			public:
				static void register_itself();

				DecalData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

			private:
				Range3 placementOffset = Range3::zero;
				float attackTime = 0.5f;
				float decayTime = 30.0f;

				bool modifyAlpha = false;
				bool modifyScale = false;

				static AppearanceControllerData* create_data();
				
				friend class Decal;
			};

		};
	};
};
