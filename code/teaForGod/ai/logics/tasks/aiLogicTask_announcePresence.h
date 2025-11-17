#pragma once

#include "..\..\..\..\core\latent\latent.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace ShouldTask
			{
				bool announce_presence(Framework::IModulesOwner* imo);
			};

			namespace Tasks
			{
				/**
				 *	Announce itselfs as traverses the world.
				 */
				LATENT_FUNCTION(announce_presence);
			};
		};
	};
};
