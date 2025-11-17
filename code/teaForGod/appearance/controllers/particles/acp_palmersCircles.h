#pragma once

#include "..\..\..\..\framework\appearance\controllers\ac_particlesMeshParticles.h"
#include "..\..\..\..\framework\meshGen\meshGenParam.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class PalmersCirclesData;

			/**
			 *	Palmers are in two rings/circles, outer and inner
			 */
			class PalmersCircles
			: public Framework::AppearanceControllersLib::ParticlesMeshParticles
			{
				FAST_CAST_DECLARE(PalmersCircles);
				FAST_CAST_BASE(Framework::AppearanceControllersLib::ParticlesMeshParticles);
				FAST_CAST_END();

				typedef Framework::AppearanceControllersLib::ParticlesMeshParticles base;
			public:
				PalmersCircles(PalmersCirclesData const * _data);

			public: // AppearanceController
				override_ void initialise(Framework::ModuleAppearance* _owner);
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / palmers; cirles"); }

			protected: // ParticlesMeshParticles
				override_ void desire_to_deactivate() {}

				override_ void internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle); // always place particles in WS! they will be moved into proper space

			private:
				struct Palmer
				{
					bool active = true;

					Vector3 velocity = Vector3::zero;

					float speed = 0.0f;
					float speedKeptForTime = 0.0f;

					float radius = 0.0f;
					float radiusKeptForTime = 0.0f;
					float innerRingDeg = 0.0f;

					ArrayStatic<Palmer *, 4> closest;
					float findClosestIn = 0.0f;
				};
				Array<Palmer> palmers;

				Vector3 centre = Vector3::zero;

				Range outerRing = Range(10.0f, 20.0f);
				Range innerRing = Range(3.0f, 6.0f);

				Range speed = Range(0.5f, 1.0f);
				Range speedKeptForTime = Range(2.0f, 200.0f);

				Range radiusKeptForTime = Range(10.0f, 60.0f);

				PalmersCirclesData const * palmersCirclesData;
			};

			class PalmersCirclesData
			: public Framework::AppearanceControllersLib::ParticlesMeshParticlesData
			{
				FAST_CAST_DECLARE(PalmersCirclesData);
				FAST_CAST_BASE(Framework::AppearanceControllersLib::ParticlesMeshParticlesData);
				FAST_CAST_END();

				typedef Framework::AppearanceControllersLib::ParticlesMeshParticlesData base;
			public:
				static void register_itself();

				PalmersCirclesData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

				override_ Framework::AppearanceControllerData* create_copy() const;
				override_ Framework::AppearanceController* create_controller();

			private:
				Framework::MeshGenParam<Vector3> centre = Vector3::zero;

				Framework::MeshGenParam<Range> outerRing = Range(10.0f, 20.0f);
				Framework::MeshGenParam<Range> innerRing = Range(3.0f, 6.0f);

				Framework::MeshGenParam<Range> speed = Range(0.5f, 1.0f);
				Framework::MeshGenParam<Range> speedKeptForTime = Range(2.0f, 200.0f);
				
				Framework::MeshGenParam<Range> radiusKeptForTime = Range(10.0f, 60.0f);

				static AppearanceControllerData* create_data();
				
				friend class PalmersCircles;
			};

		};
	};
};
