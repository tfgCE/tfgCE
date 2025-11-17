#pragma once

#include "..\..\core\types\string.h"
#include "..\..\core\types\name.h"

#define CREATE_AI_LOGIC_FUNCTION(_var) ::Framework::AI::Logic* (*_var)(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
#define CREATE_AI_LOGIC_DATA_FUNCTION(_var) ::Framework::AI::LogicData* (*_var)()

namespace Framework
{
	namespace AI
	{
		class Logic;
		class LogicData;
		class MindInstance;

		class RegisteredLogic
		{
		public:
			RegisteredLogic(Name const & _name, CREATE_AI_LOGIC_FUNCTION(_cail), CREATE_AI_LOGIC_DATA_FUNCTION(_caild));

			Name const & get_name() const { return name; }
			Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) const { return create_ai_logic_fn(_mind, _logicData); }
			LogicData* create_ai_logic_data() const { return create_ai_logic_data_fn? create_ai_logic_data_fn() : nullptr; }

		private:
			Name name;
			CREATE_AI_LOGIC_FUNCTION(create_ai_logic_fn);
			CREATE_AI_LOGIC_DATA_FUNCTION(create_ai_logic_data_fn);
		};
	};
};

