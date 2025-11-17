#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			// provides (and blends!) right now only float parameters
			class CustomUniforms
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ void interrupted(Framework::GameScript::ScriptExecution& _execution) const; // on trap/end
				implement_ tchar const* get_debug_info() const { return TXT("custom uniforms"); }

			private:
				Name untilStoryFact; // works until story fact is active

				bool clear = false; // will force clear first

				// will keep providing values under this name
				Name id; // has to be set
				Array<Name> provide;

				// blending is just setup, doesn't stay to blend, blending is handled by provider
				// has to be providing them to make it work
				struct BlendUniform
				{
					Name uniform;
					float target = 1.0f;
					float blendTime = 0.0f;
				};
				Array<BlendUniform> blendUniforms;

				void clean_up_custom_uniforms() const;
			};
		};
	};
};
