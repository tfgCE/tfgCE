#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\text\localisedString.h"

namespace Framework
{
	class Sample;
};

namespace TeaForGodEmperor
{
	namespace Story
	{
		class Scene;
	};

	namespace GameScript
	{
		namespace Elements
		{
			class PilgrimOverlayInfo
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("pilgrim overlay info"); }

				implement_ void load_on_demand_if_required();

			private:
				Optional<bool> showMain;
				
				Optional<bool> updateMapIfVisible;

				Optional<Name> showTip;
				Optional<Name> hideTip;
				Optional<bool> hideAnyTip;
				Optional<float> tipPitch;
				Optional<float> tipScale;
				Optional<bool> tipFollowCamera;
				Framework::LocalisedString tipCustomText;
				
				Optional<int> bootLevel;

				Optional<bool> showMainLogAtCamera;

				Optional<Name> mainLogId;
				bool mainLogEmptyLine = false;
				Optional<Colour> mainLogColour;
				bool clearMainLog = false;
				bool clearMainLogLineIndicator = false;
				Name clearMainLogLineIndicatorOnNoVoiceoverActor;

				Framework::UsedLibraryStored<Framework::Sample> speak;
				bool speakAutoClear = true;
				Optional<Colour> speakColour;
				Optional<bool> blockAutoEnergySpeak;
				Optional<float> speakDelay;
				Optional<bool> speakClearLogOnSpeak;
				Optional<int> speakAtLine;

			};
		};
	};
};
