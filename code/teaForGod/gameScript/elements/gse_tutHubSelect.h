#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\tutorials\tutorialHubId.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class TutHubSelect
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("tut hub select"); }

			private:
				Name screen;
				Name widget;
				TeaForGodEmperor::TutorialHubId draggable;
			};
		};
	};
};
