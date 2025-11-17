#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class CameraRumble
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("camera rumble"); }

			private:
				Optional<Range3> maxLocationOffset; // might be read as single float
				Optional<Range3> maxOrientationOffset;
				Optional<float> blendTime;
				Optional<Range> interval;

				Optional<float> blendInTime;
				Optional<float> blendOutTime;

				bool reset = false;
			};
		};
	};
};
