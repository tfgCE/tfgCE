#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\wheresMyPoint\wmp.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	namespace MeshGeneration
	{
		struct ElementInstance;

		interface_class ICondition
		{
		public:
			virtual ~ICondition() {}

			virtual bool check(ElementInstance & _instance) const = 0;

			virtual bool load_from_xml(IO::XML::Node const* _node) { return true; }

		public:
			static ICondition * create_condition(String const & _name);
			static ICondition * load_condition_from_xml(IO::XML::Node const * _node);
		};

		/**
		 *	By default it works as "AND" condition.
		 */
		struct GenerationCondition
		{
		public:
			~GenerationCondition();
			bool check(ElementInstance & _instance) const;

			bool load_from_xml(IO::XML::Node const * _node, tchar const * _childName = TXT("generationCondition"));

		private:
			Array<ICondition*> conditions;
		};

	};
};
