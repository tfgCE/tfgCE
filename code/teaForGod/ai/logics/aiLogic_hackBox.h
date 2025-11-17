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
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class HackBoxData;

			/**
			 */
			class HackBox
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(HackBox);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new HackBox(_mind, _logicData); }

			public:
				HackBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~HackBox();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				HackBoxData const * hackBoxData = nullptr;

				Random::Generator rg;

				bool depleted = false;
				Optional<float> depletingTime;

				float timeToCheckPlayer = 0.0f;
				bool playerIsIn = false;

				SimpleVariableInfo openVar;

				struct Button
				{
					VectorInt2 at;
					SafePtr<Framework::IModulesOwner> imo;

					bool on = false; // all have to be on or off

					bool pressed = false;
					bool prevPressed = false;

					void update_emissive(bool _depleted);
				};
				Array<Button> buttons;

				void act_on_button(VectorInt2 const& _at);
				bool do_all_buttons_have_same_state() const;
			};

			class HackBoxData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(HackBoxData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new HackBoxData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class HackBox;
			};
		};
	};
};