#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class BlendEmissivesForRoom
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("blend emissives for room"); }

			private:
				Optional<Colour> emissiveColour;
				Optional<Colour> emissiveBaseColour;
				Optional<float> emissivePower;

				float forTime = 0.0f;

				float blendTime = 0.0f; // uses blend which makes it less predictable and requires forTime to be precised
				Name curve;

				Name roomVar;
			};
		};
	};
};
	