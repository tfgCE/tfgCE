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
			class HeartRoomProjectilesData;

			/**
			 */
			class HeartRoomProjectiles
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(HeartRoomProjectiles);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new HeartRoomProjectiles(_mind, _logicData); }

			public:
				HeartRoomProjectiles(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~HeartRoomProjectiles();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				HeartRoomProjectilesData const* heartRoomProjectilesData = nullptr;

				Random::Generator rg;

				bool readyAndRunning = false;
				
				bool active = false;

				float interval = 0.0f;
				float timeToShoot = 0.0f;
				Optional<int> shotsLeft;
			};

			class HeartRoomProjectilesData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(HeartRoomProjectilesData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new HeartRoomProjectilesData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class HeartRoomProjectiles;

			};
		};
	};
};