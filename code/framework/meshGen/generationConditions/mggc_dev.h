#pragma once

#include "..\meshGenGenerationCondition.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace GenerationConditions
		{
			class Dev
			: public ICondition
			{
			public:
				implement_ bool check(ElementInstance & _instance) const;

				implement_ bool load_from_xml(IO::XML::Node const * _node);

			private:
#ifdef AN_DEVELOPMENT
				int atLevel = 1;
#endif
			};
		};
	};
};
