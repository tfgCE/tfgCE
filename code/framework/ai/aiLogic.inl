#ifdef AN_CLANG
#include "aiMindInstance.h"
#include "..\module\moduleAI.h"
#endif

namespace Framework
{
	namespace AI
	{
		template <typename Class>
		Class* Logic::get_logic_as(IModulesOwner* _imo)
		{
			if (_imo)
			{
				if (auto * ai = _imo->get_ai())
				{
					if (auto * mind = ai->get_mind())
					{
						return fast_cast<Class>(mind->get_logic());
					}
				}
			}
			return nullptr;
		}
	};
};
