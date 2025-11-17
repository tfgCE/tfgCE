#pragma once

#include "..\..\..\..\core\latent\latent.h"

namespace Framework
{
	interface_class IModulesOwner;
	struct RelativeToPresencePlacement;

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
			void issue_perception_alert_ai_message(Framework::IModulesOwner* _owner, Framework::IModulesOwner* _target, Framework::RelativeToPresencePlacement const& _placement);

			namespace Tasks
			{
				LATENT_FUNCTION(perception);
			};
		};
	};
};
