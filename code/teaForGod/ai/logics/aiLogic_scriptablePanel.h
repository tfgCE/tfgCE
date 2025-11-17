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
			class ScriptablePanelData;

			/**
			 */
			class ScriptablePanel
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(ScriptablePanel);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new ScriptablePanel(_mind, _logicData); }

			public:
				ScriptablePanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~ScriptablePanel();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

			private:
				void set_switches_enable(bool _enable);

			private:
				ScriptablePanelData const * scriptablePanelData = nullptr;

				struct Switch
				{
					Optional<int> activeAt;
					SafePtr<Framework::IModulesOwner> imo;
				};
				Array<Switch> switches;
			};

			class ScriptablePanelData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(ScriptablePanelData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new ScriptablePanelData(); }

				ScriptablePanelData();
				virtual ~ScriptablePanelData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Name interactiveIdVar;

				friend class ScriptablePanel;
			};
		};
	};
};