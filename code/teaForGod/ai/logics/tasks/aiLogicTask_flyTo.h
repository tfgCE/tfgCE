#pragma once

#include "..\..\..\..\core\latent\latent.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Tasks
			{
				/**
				 *	Flies using a relative to presence placement.
				 *	Handles rooms that allow free flying.
				 */
				LATENT_FUNCTION(fly_to);
				/**
				 *	Flies using a in room placement.
				 *	Handles rooms that allow free flying.
				 */
				LATENT_FUNCTION(fly_to_room);
			};
		};
	};
};
