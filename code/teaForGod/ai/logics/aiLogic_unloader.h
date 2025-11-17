#pragma once

#include "..\..\teaEnums.h"

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
	class ModulePilgrim;

	namespace AI
	{
		namespace Logics
		{
			class UnloaderData;

			/**
			 */
			class Unloader
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Unloader);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Unloader(_mind, _logicData); }

			public:
				Unloader(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Unloader();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				UnloaderData const * unloaderData = nullptr;

				float unloaderOpenTarget = 1.0f;
				float unloaderOpen = 1.0f;

				::Framework::SocketID baySocket;
				float bayRadius = 0.1f;
				int bayOccupiedEmissive = 0;

				::Framework::RelativeToPresencePlacement objectInBay;
				bool objectInBayIsSole = false; // if there is only one object in bay currently
				bool objectInBayWasSet = false;
				bool allowAutoOpen = true;

				bool unloaderEnabled = true;

				SimpleVariableInfo unloaderOpenVar;

				void update_bay(::Framework::IModulesOwner* imo, bool _force = false);
				void update_object_in_bay(::Framework::IModulesOwner* imo);
			};

			class UnloaderData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(UnloaderData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new UnloaderData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:

				friend class Unloader;
			};
		};
	};
};