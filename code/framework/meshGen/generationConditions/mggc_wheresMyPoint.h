#pragma once

#include "..\meshGenGenerationCondition.h"

#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace GenerationConditions
		{
			class WheresMyPoint
			: public ICondition
			{
			public:
				implement_ bool check(ElementInstance & _instance) const;

				implement_ bool load_from_xml(IO::XML::Node const * _node);

			private:
				::WheresMyPoint::ToolSet value;

			};
		};
	};
};
