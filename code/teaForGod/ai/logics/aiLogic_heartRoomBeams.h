#pragma once

#include "..\..\..\core\collision\collisionFlags.h"
#include "..\..\..\core\mesh\boneID.h"
#include "..\..\..\core\tags\tagCondition.h"

#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace Framework
{
	class Display;
	class ItemType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class HeartRoomBeamsData;

			/**
			 *	Rotates "rotating" elements to shoot out beams
			 *	Beams are represented by pair of bones
			 *	Beams are updated one per frame
			 *	Each beam checks one target once. If not valid, shoots randomly (if hits anything then, good for it)
			 *	They shoot out using beamB_ beamT_ sockets
			 *	If beams are inactive, they reside at beamRestSocket
			 */
			class HeartRoomBeams
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(HeartRoomBeams);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new HeartRoomBeams(_mind, _logicData); }

			public:
				HeartRoomBeams(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~HeartRoomBeams();

			public:
				struct BeamInfo
				{
					Meshes::BoneID boneStart;
					Meshes::BoneID boneEnd;
					Transform startWS;
					Transform endWS;
				};
				void fill_beam_infos(REF_ Array<BeamInfo> & _beams) const;

				float get_rotating_yaw() const { return rotatingYaw; }

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				HeartRoomBeamsData const* heartRoomBeamsData = nullptr;

				Random::Generator rg;

				int sideCount = 3;
				int beamsPerSide = 3;

				float rotatingYaw = 0.0f;
				float rotateSpeed = 0.0f;
				float rotateDir = 0.0f;

				struct Beam
				{
					int sideIdx;
					int beamIdx;
					float timeSinceUpdate = 0.0f;
					float scale = 0.0f;
					Meshes::BoneID boneStart;
					Meshes::BoneID boneEnd;
					Vector3 startLocWS = Vector3::zero;
					Vector3 endLocWS = Vector3::zero;

					Transform startWS = Transform::identity;
					Transform endWS = Transform::identity;

					Transform restStartWS = Transform::identity;
					Transform restEndWS = Transform::identity;

					void update_transforms();

					Framework::SocketID beamB;
					Framework::SocketID beamT;
				};
				Array<Beam> beams;
				float updateBeamTimeLeft = 0.0f;
				int updateBeamIdx = 0;

				struct Side
				{
					struct Tracked
					{
						Vector3 prevRelLoc;
						SafePtr<Framework::IModulesOwner> imo;
					};
					Array<Tracked> tracked;
				};
				Array<Side> sides;

				Framework::SocketID beamRest;

				bool readyAndRunning = false;

				bool requestedActive = true;
				bool active = false;
				float timeLeftActive = 0.0f;
				float timeNextActive = 0.0f;
				Optional<float> forceRotateSpeed = 0.0f; // start stopped
				Optional<float> forceAcceleration;
				bool accelerateInQuickly = false;
				bool leavePlayerAlone = false;
				bool noVerticalCheck = false;

				//

				bool shoot = false;
				float shootInterval = 0.0f;
				float timeToNextShoot = 0.0f;

				//

				SimpleVariableInfo spinSpeedVar;
				bool spinSound = false;
				bool activeSound = false;
			};

			class HeartRoomBeamsData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(HeartRoomBeamsData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new HeartRoomBeamsData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class HeartRoomBeams;

				Collision::Flags beamCollisionFlags = Collision::Flags::none(); // where should it attach
				
				float beamInterval = 0.0f;
				float beamActive = 0.0f;
				float rotateSpeed = 10.0f;
				float rotateSpeedDown = 5.0f;
				float rotateAccelerateUp = 2.0f;
				float rotateAccelerateDown = 0.5f;
				Energy damage = Energy(10.0f);
			};
		};
	};
};