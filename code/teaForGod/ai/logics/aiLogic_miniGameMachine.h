#pragma once

#include "..\..\..\core\other\simpleVariableStorage.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiIMessageHandler.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\game\gameInputProxy.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\minigames\platformer\platformerInfo.h"
#include "..\..\..\framework\minigames\platformer\platformerGame.h"
#include "..\..\..\framework\presence\presencePath.h"

namespace Framework
{
	class Display;
	class DisplayText;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
#ifdef AN_MINIGAME_PLATFORMER
		namespace Logics
		{
			class MiniGameMachineData;

			/**
			 */
			class MiniGameMachine
			: public ::Framework::AI::Logic
			{
				FAST_CAST_DECLARE(MiniGameMachine);
				FAST_CAST_BASE(::Framework::AI::Logic);
				FAST_CAST_END();

				typedef ::Framework::AI::Logic base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new MiniGameMachine(_mind, _logicData); }

			public:
				MiniGameMachine(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~MiniGameMachine();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				MiniGameMachineData const * miniGameMachineData = nullptr;

				Framework::GameInputProxy useInput;

				float advanceDisplayDeltaTime = 0.0f;
				::Framework::Display* display = nullptr;
				::System::RenderTargetPtr mainRT;
				Platformer::Game* game = nullptr;
			};


			class MiniGameMachineData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(MiniGameMachineData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new MiniGameMachineData(); }

				MiniGameMachineData();
				virtual ~MiniGameMachineData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Framework::UsedLibraryStored<Platformer::Info> miniGameInfo;

				friend class MiniGameMachine;
			};
		};
#endif
	};
};