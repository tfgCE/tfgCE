#pragma once

#include "..\..\..\..\framework\appearance\controllers\ac_particlesMeshParticles.h"
#include "..\..\..\..\framework\meshGen\meshGenParam.h"

namespace TeaForGodEmperor
{
	namespace AppearanceControllersLib
	{
		namespace Particles
		{
			class AirTrafficData;

			/**
			 *	air traffic moves using a single pattern (you want more? add more objects)
			 */
			class AirTraffic
			: public Framework::AppearanceControllersLib::ParticlesMeshParticles
			{
				FAST_CAST_DECLARE(AirTraffic);
				FAST_CAST_BASE(Framework::AppearanceControllersLib::ParticlesMeshParticles);
				FAST_CAST_END();

				typedef Framework::AppearanceControllersLib::ParticlesMeshParticles base;
			public:
				AirTraffic(AirTrafficData const * _data);

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
				Random::Generator rg;

				// all dirs might be applied on top of pattern target dir
				// for circular: it is always as object was on x axis moved forward on y
				//		only the final placement is calculated properly
				//		this means that we don't actually care about circular movement as it works out of the box, we change "x" into "yaw", "y" into "radius" and only "z" remains

				struct Element
				{
					Vector3 dir = Vector3::zero;

					Rotator3 patternTargetDir = Rotator3::zero; // depends on the pattern, if circle, relative to location in the circle
					Rotator3 targetDirOffset = Rotator3::zero;

					Rotator3 rotationOffset = Rotator3::zero; // visible

					float speed = 0.0f;
					float targetSpeed = 0.0f;

					Optional<float> speedChangeTimeLeft;
					Optional<float> targetDirChangeTimeLeft;
				};
				Array<Element> elements;

				//-- data

				Name pattern;

				struct CirclePattern
				{
					Range radius = Range(100.0f, 200.0f);
					Range verticalOffset = Range(0.0f, 0.0f);

					// if not set, will use both with same probability
					float moveClockwiseChance = 0.0f;
					float moveCounterClockwiseChance = 0.0f;
				} circlePattern;

				struct FreeRangePattern
				{
					float radius = 200.0f;
					Range verticalOffset = Range(0.0f, 0.0f);
				} freeRangePattern;

				struct InDirPattern
				{
					float yaw = 0.0f;
					Range rangeX = Range(-100.0f, 100.0f);
					Range rangeY = Range(-200.0f, 200.0f);
					Range verticalOffset = Range(0.0f, 0.0f);
					bool looped = true;
				} inDirPattern;

				struct RotationOffset
				{
					float applyDirToPitch = 0.0f;
					float applyAccelXToRoll = 0.0f;
					float pitchBlendTime = 0.2f;
					float rollBlendTime = 0.5f;
				} rotationOffset;

				struct MovementSpeed
				{
					Range speed = Range(10.0f, 20.0f);
					Range changeInterval = Range::empty; // no changes
					float acceleration = 0.6f;
				} movementSpeed;

				struct TargetDir
				{
					Range pitchOffset = Range::zero;
					Range yawOffset = Range::zero;
					Range changeInterval = Range::empty; // no changes
					float maxChangePerSecond = 1.0f;
					float blendTime = 0.6f;
				} targetDir;
			};

			class AirTrafficData
			: public Framework::AppearanceControllersLib::ParticlesMeshParticlesData
			{
				FAST_CAST_DECLARE(AirTrafficData);
				FAST_CAST_BASE(Framework::AppearanceControllersLib::ParticlesMeshParticlesData);
				FAST_CAST_END();

				typedef Framework::AppearanceControllersLib::ParticlesMeshParticlesData base;
			public:
				static void register_itself();

				AirTrafficData();

			public: // AppearanceControllerData
				override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

				override_ Framework::AppearanceControllerData* create_copy() const;
				override_ Framework::AppearanceController* create_controller();

			private:
				Framework::MeshGenParam<Name> pattern;

				struct CirclePattern
				{
					Framework::MeshGenParam<Range> radius = Range(100.0f, 200.0f);
					Framework::MeshGenParam<Range> verticalOffset = Range(0.0f, 0.0f);

					// if not set, will use both with same probability
					Framework::MeshGenParam<float> moveClockwiseChance = 0.0f;
					Framework::MeshGenParam<float> moveCounterClockwiseChance = 0.0f;
				} circlePattern;

				struct FreeRangePattern
				{
					Framework::MeshGenParam<float> radius = 200.0f;
					Framework::MeshGenParam<Range> verticalOffset = Range(0.0f, 0.0f);
				} freeRangePattern;

				struct InDirPattern
				{
					Framework::MeshGenParam<float> yaw = 0.0f;
					Framework::MeshGenParam<Range> rangeX = Range(-100.0f, 100.0f);
					Framework::MeshGenParam<Range> rangeY = Range(-200.0f, 200.0f);
					Framework::MeshGenParam<Range> verticalOffset = Range(0.0f, 0.0f);
					Framework::MeshGenParam<bool> looped = true;
				} inDirPattern;

				struct RotationOffset
				{
					Framework::MeshGenParam<float> applyDirToPitch = 0.0f;
					Framework::MeshGenParam<float> applyAccelXToRoll = 0.0f;
					Framework::MeshGenParam<float> pitchBlendTime = 0.0f;
					Framework::MeshGenParam<float> rollBlendTime = 0.0f;
				} rotationOffset;

				struct MovementSpeed
				{
					Framework::MeshGenParam<Range> speed = Range::zero;
					Framework::MeshGenParam<Range> changeInterval = Range::empty; // no changes
					Framework::MeshGenParam<float> acceleration = 0.0f;
				} movementSpeed;

				struct TargetDir
				{
					Framework::MeshGenParam<Range> pitchOffset = Range::zero;
					Framework::MeshGenParam<Range> yawOffset = Range::zero;
					Framework::MeshGenParam<Range> changeInterval = Range::empty; // no changes

					Framework::MeshGenParam<float> maxChangePerSecond = 0.0f;
					Framework::MeshGenParam<float> blendTime = 0.0f;
				} targetDir;

				static AppearanceControllerData* create_data();
				
				friend class AirTraffic;
			};

		};
	};
};
