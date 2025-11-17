#pragma once

#include "..\..\game\energy.h"

#include "..\..\..\core\tags\tag.h"

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class EngineGenerator
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(EngineGenerator);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EngineGenerator(_mind, _logicData); }

			public:
				EngineGenerator(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EngineGenerator();

			public: // Logic
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

				enum State
				{
					Off,
					InVisibleRoom,
					Idle,
					Wall,
					IdlePostWall
				};
				bool idleLightning = false;
				State state = State::Off;

				Optional<float> redoWallAtDist;
				Array<SafePtr<Framework::IModulesOwner>> steamObjects;
				ArrayStatic<Name, 8> leftUpSockets;
				ArrayStatic<Name, 8> leftDownSockets;
				ArrayStatic<Name, 8> rightUpSockets;
				ArrayStatic<Name, 8> rightDownSockets;
				Optional<Name> lastHitSocket;
				bool hitAnything = false;

				int leftUp = 0;
				int rightUp = 0;
				int leftDown = 0;
				int rightDown = 0;

				bool mayDoWall = true;
				int generatorIdx = 0;
				float redoWallChance = 0.3f;
				float platformWidth = 2.0f;
				int platformTileWidth = 3;
				Energy damage = Energy(1);

				bool discharge();

				void set_idle_lightning(bool _idleLightning);
			};

		};
	};
};