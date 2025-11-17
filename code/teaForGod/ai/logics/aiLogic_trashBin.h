#pragma once

#include "..\..\teaEnums.h"

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
	class ModulePilgrim;

	namespace AI
	{
		namespace Logics
		{
			class TrashBinData;

			/**
			 *	Trash bin is used to destroy items inside permanently (removes them from persistence as well!)
			 *	This has to work with the armoury as armoury is responsible for creating weapons and sending them to persistence
			 * 
			 *	It has a bay and a handle (pull it upwards)
			 *	When activated:
			 *		1. TrashBin closes and remains closed.
			 *		2. TrashBin marks weapon as "can't be picked up".
			 *		3. TrashBin broadcasts "weapon in trash bin to destroy".
			 *		4. Armoury receives, stores sender, removes weapon from persistence.
			 *		5. Armoury responds "weapon ok to be destroyed".
			 *		6. TrashBin removes IMO from the world.
			 *		7. TrashBin opens.
			 */
			class TrashBin
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(TrashBin);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new TrashBin(_mind, _logicData); }

			public:
				TrashBin(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~TrashBin();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				TrashBinData const * trashBinData = nullptr;

				InteractiveSwitchHandler destroyHandle; // handle that has to be pushed to destroy things inside

				float trashBinOpenTarget = 1.0f;
				float trashBinOpen = 1.0f;

				::Framework::SocketID baySocket;
				float bayRadius = 0.1f;
				int bayOccupiedEmissive = 0;

				::Framework::RelativeToPresencePlacement objectInBay;
				bool objectInBayIsSole = false; // if there is only one object in bay currently
				bool objectInBayWasSet = false;
				bool allowClosingBay = false;
				bool bayShouldRemainClosed = false;

				SimpleVariableInfo trashBinOpenVar;

				void update_bay(::Framework::IModulesOwner* imo, bool _force = false);
				void update_object_in_bay(::Framework::IModulesOwner* imo);
			};

			class TrashBinData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(TrashBinData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new TrashBinData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:

				friend class TrashBin;
			};
		};
	};
};