#pragma once

#include "..\meshGenGenerationCondition.h"

#include "..\..\..\core\types\optional.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace GenerationConditions
		{
			class ParameterBase
			: public ICondition
			{
			public:
				implement_ bool check(ElementInstance & _instance) const;

				implement_ bool load_from_xml(IO::XML::Node const * _node);

			private:
				Name parameter;
				Optional<Name> isEqualToName;
				Optional<int> isEqualToInt;
				Optional<bool> isEqualToBool;

				template <typename Class>
				inline Class const * get_parameter(ElementInstance& _instance, Name const& _name) const;
				
				inline bool has_any_parameter(ElementInstance& _instance, Name const& _name) const;

			protected:
				bool useGlobal = false;
			};

			class Parameter
			: public ParameterBase
			{
			public:
				Parameter() { useGlobal = false; }
			};

			class ParameterGlobal
			: public ParameterBase
			{
			public:
				ParameterGlobal() { useGlobal = true; }
			};
		};
	};
};
