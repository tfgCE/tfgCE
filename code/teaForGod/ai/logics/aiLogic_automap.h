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
			class AutomapData;

			/**
			 */
			class Automap
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Automap);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Automap(_mind, _logicData); }

			public:
				Automap(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Automap();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				AutomapData const * automapData = nullptr;

				bool markedAsKnownForOpenWorldDirection = false;

				::Framework::Display* display = nullptr;

				bool redrawNow = false;
				float timeToRedraw = 0.0f;
				bool drawMap = true;

				Optional<VectorInt2> mapLoc;

				SafePtr<Framework::IModulesOwner> button;

				void trigger_pilgrim(bool _justMap = false);
				void update_emissive(bool _available);
			};

			class AutomapData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(AutomapData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new AutomapData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				int revealCellRadius = 0;
				int revealExitsCellRadius = 5;
				int revealDevicesCellRadius = 3;

				int longRangeRevealCellRadius = 0;
				int longRangeRevealExitsCellRadius = 8;
				int longRangeRevealDevicesCellRadius = 5;

				int autoMapOnlyRevealCellRadius = 8;
				int autoMapOnlyRevealExitsCellRadius = 8;
				int autoMapOnlyRevealDevicesCellRadius = 3;

				int autoMapOnlyLongRangeRevealCellRadius = 12;
				int autoMapOnlyLongRangeRevealExitsCellRadius = 12;
				int autoMapOnlyLongRangeRevealDevicesCellRadius = 5;
				
				int detectLongDistanceDetectableDevicesAmount = 3;
				int detectLongDistanceDetectableDevicesRadius = 10;

				Colour buttonColour = Colour::white;
				Colour hitIndicatorColour = Colour::white;

				friend class Automap;
			};
		};
	};
};