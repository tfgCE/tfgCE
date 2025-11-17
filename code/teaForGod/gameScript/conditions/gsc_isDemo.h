#pragma once

#include "..\..\..\framework\gameScript\gameScriptCondition.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Conditions
		{
			class IsDemo
			: public Framework::GameScript::ScriptCondition
			{
				typedef Framework::GameScript::ScriptCondition base;
			public: // ScriptCondition
				implement_ bool check(Framework::GameScript::ScriptExecution const& _execution) const;
			};
		}
	}
}