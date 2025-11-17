#pragma once

#include <functional>

#include "..\..\core\types\name.h"

namespace Framework
{
	namespace GameScript
	{
		class ScriptCondition;

		struct RegisteredScriptCondition
		{
			typedef std::function<ScriptCondition * ()> CreateFN;
			Name name;
			CreateFN create = nullptr;

			RegisteredScriptCondition(Name const& _name, CreateFN _create = nullptr) : name(_name), create(_create) {}
			RegisteredScriptCondition& temp_ref() { return *this; }
		};

		class RegisteredScriptConditions
		{
		public:
			static void initialise_static();
			static void close_static();

			static ScriptCondition* create(tchar const* _tse);

			static void add(RegisteredScriptCondition _tse);

		private:
			static RegisteredScriptConditions* s_conditions;

			Array<RegisteredScriptCondition> conditions;
		};
	};

}