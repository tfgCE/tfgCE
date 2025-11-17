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
				bool being_thrown(Framework::IModulesOwner const * _imo);
			};

			namespace Tasks
			{
				/**
				 *	Manages being thrown, requires to be initiated by main function (with latent yield!)
				 *	Auto exits to default movement when hits (touches) something at lower speed.
				 */
				LATENT_FUNCTION(being_thrown);
				LATENT_FUNCTION(being_thrown_and_stay);
			};
		};
	};
};
