#pragma once

#include "..\gameScriptElement.h"

namespace Framework
{
	class Room;
};

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class RoomAppearance
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("room appearance"); }

			private:
				Name roomVar;

				float blendTime = 0.0f;
				Name blendCurve;
				SimpleVariableStorage setShaderParams0;
				SimpleVariableStorage setShaderParams1;

				void act_on(Framework::Room* _a, float _blendPt) const;
			};
		};
	};
};
