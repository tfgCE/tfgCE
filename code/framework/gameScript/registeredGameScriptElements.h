#pragma once

#include <functional>

#include "..\..\core\types\name.h"

namespace Framework
{
	namespace GameScript
	{
		class ScriptElement;

		struct RegisteredScriptElement
		{
			typedef std::function<ScriptElement * ()> CreateFN;
			Name name;
			CreateFN create = nullptr;

			RegisteredScriptElement(Name const& _name, CreateFN _create = nullptr) : name(_name), create(_create) {}
			RegisteredScriptElement& temp_ref() { return *this; }
		};

		class RegisteredScriptElements
		{
		public:
			static void initialise_static();
			static void close_static();

			static ScriptElement * create(tchar const* _tse);

			static void add(RegisteredScriptElement _tse);

		private:
			static RegisteredScriptElements* s_elements;

			Array<RegisteredScriptElement> elements;
		};
	};

}