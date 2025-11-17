#pragma once

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\text\localisedString.h"

namespace Framework
{
	class Display;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class DisplayMessageData;

			/**
			 */
			class DisplayMessage
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(DisplayMessage);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new DisplayMessage(_mind, _logicData); }

			public:
				DisplayMessage(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~DisplayMessage();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				DisplayMessageData const * displayMessageData = nullptr;

				int languageChangeCount = 0;

				int messageIdx = -1;
				float messageTimeLeft = 0.0f;
				bool redrawNow = false;

				::Framework::Display* display = nullptr;
			};

			class DisplayMessageData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(DisplayMessageData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new DisplayMessageData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				struct Message
				{
					bool immediateDraw = false;
					Framework::LocalisedString message;
					Optional<float> messageTime;
					Name variableRequired; // requires bool variable to be set to true
					int align = -1; // -1 left, 0, centre, 1 right
					int valign = 1; // -1 bottom, 0, centre, 1 top
				};
				Array<Message> messages;

				bool noHeader = false;
				int headerSize = 0;
				Framework::LocalisedString header;

				float messageTime = 5.0f;
				bool variableControlled = false;

				friend class DisplayMessage;
			};
		};
	};
};