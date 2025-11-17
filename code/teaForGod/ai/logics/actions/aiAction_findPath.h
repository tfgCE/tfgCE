#pragma once

#include <functional>

#include "..\..\..\..\core\globalDefinitions.h"
#include "..\..\..\..\core\memory\safeObject.h"

#include "..\..\..\..\framework\nav\navDeclarations.h"

struct Vector3;

namespace Latent
{
	class Frame;
};

namespace Framework
{
	struct RelativeToPresencePlacement;
	
	interface_class IModulesOwner;

	namespace AI
	{
		class MindInstance;
	};
};

namespace TeaForGodEmperor
{
	struct Energy;

	namespace AI
	{
		namespace Actions
		{
			// returns true if to continue execution, false if can't do a thing
			bool find_path(
				::Latent::Frame& _frame,
				Framework::AI::MindInstance* mind, ::Framework::Nav::TaskPtr& pathTask, SafePtr<Framework::IModulesOwner>& enemy,
				Framework::RelativeToPresencePlacement & enemyPlacement, bool perceptionSightImpaired,
				std::function<void(REF_ Vector3 & goToLoc)> _on_try_to_approach = nullptr);
		};
	};
};