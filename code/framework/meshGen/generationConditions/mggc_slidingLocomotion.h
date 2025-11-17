#pragma once

#include "..\meshGenGenerationCondition.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace GenerationConditions
		{
			class SlidingLocomotion
			: public ICondition
			{
			public:
				implement_ bool check(ElementInstance & _instance) const;
			};
		};
	};
};
