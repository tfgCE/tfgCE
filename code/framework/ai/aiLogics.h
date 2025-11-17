#pragma once

#include "registeredAILogic.h"

#include "..\..\core\containers\map.h"
#include "..\..\core\latent\latent.h"

namespace Framework
{
	namespace AI
	{
		class Logic;
		class LogicData;

		class Logics
		{
		public:
			static void initialise_static();
			static void close_static();

			static void register_ai_logic(tchar const * _name, CREATE_AI_LOGIC_FUNCTION(_create_ai_logic), CREATE_AI_LOGIC_DATA_FUNCTION(_create_ai_logic_data) = nullptr);
			//
			static Logic* create(Name const & _name, ::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
			static LogicData* create_data(Name const & _name);

			static void register_ai_logic_latent_task(tchar const * _name, LatentFunction _function);
			//
			static LatentFunction get_latent_task(Name const & _name, LatentFunction _default = nullptr);

			void clear();

		private:
			static Logics* s_logics;
			Map<Name, RegisteredLogic*> registeredLogics;
			Map<Name, LatentFunction> registeredLatentTasks;

			~Logics();
		};
	};
};
