#pragma once

#include "..\ac_particlesContinuous.h"

namespace Framework
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class DustData;

			/**
			 *	
			 */
			class Dust
			: public ParticlesContinuous
			{
				FAST_CAST_DECLARE(Dust);
				FAST_CAST_BASE(ParticlesContinuous);
				FAST_CAST_END();

				typedef ParticlesContinuous base;
			public:
				Dust(DustData const * _data);

			public: // AppearanceController
				override_ void activate();
				override_ void advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context);

			protected:
				virtual tchar const * get_name_for_log() const { return TXT("particles / dust"); }

			protected: // ParticlesContinuous
				override_ void internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle = nullptr);

			private:
				DustData const * dustData;

				Range initialDistance;
				Range3 initialPlacementBox;

				enum CustomData
				{
					TimeToChangeVelocity,
					VelocityRequestedX,
					VelocityRequestedY,
					VelocityRequestedZ,
				};
			};

			class DustData
			: public ParticlesContinuousData
			{
				FAST_CAST_DECLARE(DustData);
				FAST_CAST_BASE(ParticlesContinuousData);
				FAST_CAST_END();

				typedef ParticlesContinuousData base;
			public:
				static void register_itself();

				DustData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

				override_ AppearanceControllerData* create_copy() const;
				override_ AppearanceController* create_controller();

				override_ void apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const& _context);

			private:
				float timeAlive = 5.5f;
				float timeAliveVarietyCoef = 0.2f;
				Range speed = Range(0.0f);
				Range3 initialVelocityVector = Range3(Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(-1.0f, 1.0f));
				Range changeVelocityInterval = Range(0.0f);
				float changeVelocityBlendTime = 0.0f;
				Framework::MeshGenParam<Range> initialDistance;
				Framework::MeshGenParam<Range3> initialPlacementBox;
				Name initialPlacementBoxVar;
				float initialOrientationMaxSpeed = 90.0f;

				static AppearanceControllerData* create_data();
				
				friend class Dust;
			};

		};
	};
};
