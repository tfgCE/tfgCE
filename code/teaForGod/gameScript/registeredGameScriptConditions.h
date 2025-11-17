#pragma once

#include "..\..\framework\gameScript\registeredGameScriptConditions.h"

namespace Framework
{
	namespace GameScript
	{
		class ScriptCondition;
	};
};

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		class RegisteredScriptConditions
		{
		public:
			static void initialise_static();
			static void close_static();
		};
	};

}