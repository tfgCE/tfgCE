#pragma once

#include "..\meshGenGenerationCondition.h"

#include "..\..\..\core\types\optional.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace GenerationConditions
		{
			class AllowMoreDetails
			: public ICondition
			{
			public:
				virtual ~AllowMoreDetails();

				implement_ bool check(ElementInstance & _instance) const;
			};
		};
	};
};
