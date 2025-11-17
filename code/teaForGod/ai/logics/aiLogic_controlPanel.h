#pragma once

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\game\gameInputProxy.h"
#include "..\..\..\framework\text\localisedString.h"

namespace Framework
{
	class Display;
	class DisplayText;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class ControlPanelData;

			namespace ControlPanelPage
			{
				enum Type
				{
					Main
				};
			};

			/**
			 */
			class ControlPanel
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(ControlPanel);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new ControlPanel(_mind, _logicData); }

			public:
				ControlPanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~ControlPanel();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

				static LATENT_FUNCTION(main_page);

			private:
				void request_page(ControlPanelPage::Type _page) { requestedPage = _page; }

			private:
				void set_devices_enable(bool _enable);

			private:
				ControlPanelData const * controlPanelData = nullptr;

				ControlPanelPage::Type requestedPage = ControlPanelPage::Main;

				Array<SafePtr<Framework::IModulesOwner>> devices;

				Framework::GameInputProxy useInput;

				float advanceDisplayDeltaTime = 0.0f;
				::Framework::Display* display = nullptr;
			};


			class ControlPanelData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(ControlPanelData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new ControlPanelData(); }

				ControlPanelData();
				virtual ~ControlPanelData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Name idVar;

				Framework::LocalisedString lsEnable;
				Framework::LocalisedString lsDisable;

				Framework::LocalisedString lsNext;
				Framework::LocalisedString lsPrev;

				Framework::LocalisedString lsCurrentPowerOutput; // if any power source modules detected
				Framework::LocalisedString lsCurrentPowerOutputNone; // 0, off
				Framework::LocalisedString lsObjectIntegrity; // if any health modules detected
				Framework::LocalisedString lsObjectIntegrityOK; // 100%, ok, full

				Optional<Colour> currentPowerOutputNoneColour;
				Optional<Colour> objectIntegrityNoneColour;

				friend class ControlPanel;
			};
		};
	};
};