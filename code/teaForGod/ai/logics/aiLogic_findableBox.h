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
	class TexturePart;
	class ObjectType;
	interface_class IModulesOwner;
	struct DelayedObjectCreation;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class FindableBoxData;

			/**
			 */
			class FindableBox
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(FindableBox);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new FindableBox(_mind, _logicData); }

			public:
				FindableBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~FindableBox();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				FindableBoxData const * findableBoxData = nullptr;

				bool depleted = false;

				float timeToCheckPlayer = 0.0f;
				bool playerIsIn = false;
			};

			class FindableBoxData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(FindableBoxData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new FindableBoxData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class FindableBox;
			};
		};
	};
};