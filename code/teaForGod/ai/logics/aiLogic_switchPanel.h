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
			class SwitchPanelData;

			/**
			 */
			class SwitchPanel
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(SwitchPanel);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new SwitchPanel(_mind, _logicData); }

			public:
				SwitchPanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~SwitchPanel();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

			private:
				void set_devices_enable(bool _enable);
				void set_switches_enable(bool _enable);

			private:
				SwitchPanelData const * switchPanelData = nullptr;

				Array<SafePtr<Framework::IModulesOwner>> devices;
				struct Switch
				{
					Optional<int> activeAt;
					SafePtr<Framework::IModulesOwner> imo;
				};
				Array<Switch> switches;
			};

			class SwitchPanelData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(SwitchPanelData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new SwitchPanelData(); }

				SwitchPanelData();
				virtual ~SwitchPanelData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Name controlledIdVar;
				Name interactiveIdVar;

				friend class SwitchPanel;
			};
		};
	};
};