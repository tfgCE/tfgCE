#pragma once

#include "..\..\..\..\core\latent\latent.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class Pickup;
	};

	namespace AI
	{
		namespace Logics
		{
			namespace ShouldTask
			{
				bool being_held(CustomModules::Pickup const* _asPickup);
			};

			namespace Tasks
			{
				/**
				 *	Manages being held (in hand or in pocket), requires to be initiated by main function (with latent yield!)
				 *	Auto exits to thrown.
				 */
				LATENT_FUNCTION(being_held);
			};
		};
	};
};
