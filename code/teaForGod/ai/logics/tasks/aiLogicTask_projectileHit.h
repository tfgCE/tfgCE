#pragma once

#include "..\..\..\..\core\latent\latent.h"

namespace Framework
{
	namespace AI
	{
		class Message;
		class MindInstance;
		struct LatentTaskInfoWithParams;
	};
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Tasks
			{
				bool handle_projectile_hit_message(::Framework::AI::MindInstance* _mind, ::Framework::AI::LatentTaskInfoWithParams & _nextTask, Framework::AI::Message const & _message, float _distance, LATENT_FUNCTION_VARIABLE(_avoidTaskFunction), int _priority = 100, bool _allowToInterrupt = false);

				LATENT_FUNCTION(avoid_projectile_hit_3d);
			};
		};
	};
};
