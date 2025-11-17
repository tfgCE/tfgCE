#pragma once

#include "..\meshGenGenerationCondition.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace GenerationConditions
		{
			class LOD
			: public ICondition
			{
			public:
				implement_ bool check(ElementInstance & _instance) const;

				implement_ bool load_from_xml(IO::XML::Node const * _node);

			private:
				int atMaxLevel = 1;
				RangeInt atLevelRange = RangeInt::empty;
			};
		};
	};
};
