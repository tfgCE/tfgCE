#pragma once

#include "aiLogic_npcBase.h"

#include "components\aiLogicComponent_spawnableShield.h"

#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\presence\presencePath.h"

namespace Framework
{
	class DoorInRoom;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Centipede;

			class CentipedeSelfManager
			: public RefCountObject
			{
			public:
				struct Orders
				{
					// self manager gives orders, each logic clears them when done
					Concurrency::Counter pendingShot; // this will work as a memory barrier
				};

				static CentipedeSelfManager* get_or_create(); // creates if required, we should always have all centipedes in the same place, there's no need for multiple managers
				static CentipedeSelfManager* get_existing(); // does not create, hacky but acceptable way to check if centipede exists

				Orders* advance_for(Centipede* _centipede, float _deltaTime); // advances only if first

				void add(Centipede* _centipede);
				void remove(Centipede* _centipede);

				void mark_entered_boundary();
				
				void mark_segment_destroyed();

			private:
				static CentipedeSelfManager* s_manager;
				static Concurrency::SpinLock s_managerLock;

				Random::Generator rg;

				struct CentipedeSegment
				{
					Centipede* centipede = nullptr;
					SafePtr<Framework::IModulesOwner> imo;
					Orders orders;
				};
				enum Constants
				{
					MAX_CENTIPEDES = 64
				};
				ArrayStatic<CentipedeSegment, MAX_CENTIPEDES> centipedes; // static array to allow multi threading, it's a bit unsafish but shouldn't break the memory completely
				Centipede* mainAdvancer = nullptr; // updated when removed, if we skip a frame, it doesn't matter

				Range shootingInterval = Range(10.0f);
				float timeLeftToShoot = 0.0f;
				
				float frontAngle = 50.0f;
				float backShotChance = 0.2f;
				float backShotMeanChance = 0.2f;

				bool enteredBoundary = false;
				float timeSinceLastDestroyed = 0.0f;
				float timeToDestroy = 0.0f;
				float timeToDestroyFirst = 0.0f;
				float timeToDestroyNext = 0.0f;
				float timeToDestroyLast = 0.0f;

				CentipedeSelfManager();
				virtual ~CentipedeSelfManager();

				friend class Centipede;
			};

			class Centipede
			: public NPCBase
			{
				FAST_CAST_DECLARE(Centipede);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Centipede(_mind, _logicData); }

			public:
				Centipede(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Centipede();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void ready_to_activate(); // first thought in world, when placed but before initialised

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(head_behaviour);

			private:
				int centipedeId = 0;

				RefCountObjectPtr<CentipedeSelfManager> selfManager;

				struct Setup
				{
					int createCentipedeTails = 0;
				} setup;

				SpawnableShield shield;

				SimpleVariableInfo isHeadVar;

				Range callInterval = Range(5.0f, 20.0f);

				int segmentsBehind = 0;

				SafePtr<Framework::IModulesOwner> tailIMO;
				SafePtr<Framework::IModulesOwner> headIMO;
				RefCountObjectPtr<Framework::DelayedObjectCreation> tailDOC;

				Optional<bool> eyesOn;
				Optional<bool> backPlate;

				struct BoundaryPoint
				{
					Vector3 loc;
					float perpendicularRadius = 0.0f;
				};
				Array<BoundaryPoint> boundaries; // if two, will move between them
				bool enteredBoundary = false;

				enum Action
				{
					StayStationary,
					Stop,
					MoveAround,
					HeadBack,
					TurnAway
				} action = MoveAround;

				Optional<Vector3> headTowards;
				Optional<float> headTowardsUntilAngle;
				Optional<float> headTowardsTimeLeft;

				float maxHeadingOff = 20.0f;
				float maxHeadingBackOffAngle = 60.0f;
				
				float shootingDelay = 0.0f;

				Optional<float> preferredTurn;

				SafePtr<Framework::IModulesOwner> preShootTO;

				void set_eyes(bool _on);
				void set_back_plate(bool _on);

				void update_movement_module();

				friend class CentipedeSelfManager;
			};
	
		};
	};
};