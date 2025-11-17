#pragma once

#include "..\meshGenGenerationCondition.h"

#include "..\..\..\core\types\optional.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace GenerationConditions
		{
			class Or
			: public ICondition
			{
			public:
				virtual ~Or();

				implement_ bool check(ElementInstance & _instance) const;

				implement_ bool load_from_xml(IO::XML::Node const * _node);

			private:
				Array<ICondition*> conditions;
			};
		};
	};
};
