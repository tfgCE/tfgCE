#pragma once

#include "components\aiLogicComponent_upgradeCanister.h"

#include "..\..\utils\interactiveDialHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class LineModel;

	namespace AI
	{
		namespace Logics
		{
			class EXMInstallerKioskData;

			/**
			 */
			class EXMInstallerKiosk
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(EXMInstallerKiosk);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMInstallerKiosk(_mind, _logicData); }

			public:
				EXMInstallerKiosk(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMInstallerKiosk();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				EXMInstallerKioskData const * exmInstallerKioskData = nullptr;

				bool markedAsKnownForOpenWorldDirection = false;

				SafePtr<::Framework::IModulesOwner> currentPilgrim; // for which we do stuff
				SafePtr<::Framework::IModulesOwner> presentPilgrim; // who is now present

				SafePtr<::Framework::IModulesOwner> displayOwner;
				::Framework::Display* display = nullptr;
				bool displaySetup = false;
				
				bool redrawNow = false;
				float timeToRedraw = 0.0f;

				enum State
				{
					Off,
					Start,
					ShowContent,
					ShutDown,
				};
				State state = Off;
				float inStateTime = 0.0f;
				int inStateDrawnFrames = 0;

				struct VariousDrawingVariables
				{
					LineModel* contentLineModel = nullptr;
					LineModel* borderLineModel = nullptr;
					float contentDrawLines = 0.0f;
					bool contentDrawn = false;
					float shutdownAtPt = 0.0f;
				} variousDrawingVariables;

				bool selectedActiveEXMs = false;
				int activeEXMIdx = NONE;
				int passiveEXMIdx = NONE;
				Array<EXMType const *> availableActiveEXMs;
				Array<EXMType const *> availablePassiveEXMs;

				InteractiveSwitchHandler chooseEXMTypeHandler;
				InteractiveDialHandler chooseEXMHandler;
				UpgradeCanister canister;

			};

			class EXMInstallerKioskData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(EXMInstallerKioskData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new EXMInstallerKioskData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Framework::UsedLibraryStored<LineModel> activeEXMLineModel;
				Framework::UsedLibraryStored<LineModel> passiveEXMLineModel;

				Colour selectColour = Colour::white;
				Colour activeEXMColour = Colour::yellow;
				Colour passiveEXMColour = Colour::blue;

				friend class EXMInstallerKiosk;
			};
		};
	};
};