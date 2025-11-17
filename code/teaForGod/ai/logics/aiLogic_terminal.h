#pragma once

#include "..\..\game\energy.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class TerminalData;

			/**
			 */
			class Terminal
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Terminal);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Terminal(_mind, _logicData); }

			public:
				Terminal(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Terminal();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				TerminalData const * terminalData = nullptr;

				::Framework::SocketID baySocket;
				float bayRadius = 0.1f;
				bool bayActive = true;
				bool bayActiveEmissive = false;
				bool bayOccupiedEmissive = false;
				::Framework::RelativeToPresencePlacement objectInBay;
				float objectInBayTime = 0.0f;
				bool requiresEmpty = false;

				void clear_object_in_bay();
				void update_bay(::Framework::IModulesOwner* imo, bool _force = false);
				void update_object_in_bay(::Framework::IModulesOwner* imo, float _deltaTime);
			};

			class TerminalData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(TerminalData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new TerminalData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:

				friend class Terminal;
			};
		};
	};
};