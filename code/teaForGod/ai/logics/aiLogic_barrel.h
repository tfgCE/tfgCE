#pragma once

#include "aiLogic_npcBase.h"

#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

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
			class Barrel
			: public NPCBase
			{
				FAST_CAST_DECLARE(Barrel);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Barrel(_mind, _logicData); }

			public:
				Barrel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Barrel();

				void add_corrosion_leak();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(perception_blind);
				static LATENT_FUNCTION(run_away);

				InteractiveSwitchHandler switchHandler;

				struct CorrosionLeakSpawnContext
				: public RefCountObject
				{
					bool spawningCorrosionLeak = false;
					int corrosionLeaksQueued = 0;
					int corrosionLeaksSpawned = 0;
				};
				RefCountObjectPtr<CorrosionLeakSpawnContext> corrosionLeakSpawnContext;

				bool runAwayNow = false;
				Framework::RelativeToPresencePlacement runAwayFrom;

				void drop_target();
				void run_away_from(Framework::IModulesOwner const * _from);

				void play_sound(Name const & _sound);
			};

		};
	};
};