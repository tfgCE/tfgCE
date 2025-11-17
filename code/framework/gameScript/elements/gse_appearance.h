#pragma once

#include "..\gameScriptElement.h"

namespace Framework
{
	class ModuleAppearance;
};

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Appearance
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("appearance"); }

			private:
				bool mayFail = false;

				Name objectVar;
				Name toAppearance;

				Optional<bool> setVisible;
				Optional<bool> setMain;

				float blendTime = 0.0f;
				Name blendCurve;
				SimpleVariableStorage setShaderParams0;
				SimpleVariableStorage setShaderParams1;

				void act_on(Framework::ModuleAppearance* _a, float _blendPt) const;
			};
		};
	};
};
