#pragma once

#include "..\..\framework\gameScript\registeredGameScriptElements.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		class ScriptElement;

		class RegisteredScriptElements
		{
		public:
			static void initialise_static();
			static void close_static();
		};
	};

};