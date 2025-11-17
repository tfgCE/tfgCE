#pragma once

#include "..\..\..\..\core\latent\latent.h"

namespace Framework
{
	namespace AI
	{
		class MindInstance;
	};
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			bool should_hide_disappear(::Framework::AI::MindInstance* mind, bool _evenIfVisible = false);

			namespace Tasks
			{
				LATENT_FUNCTION(hide_disappear);
			};
		};
	};
};
