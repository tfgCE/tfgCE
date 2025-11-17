#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

#include "..\world\pointOfInterestTypes.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	namespace MeshGeneration
	{
		/**
		 *	To set parameters, add one to poiParameters. If set with a value (non initial) will be used, otherwise,
		 *	if parameter of same name exists in context, will fill with one from context.
		 */
		class ElementPointOfInterest
		: public Element
		{
			FAST_CAST_DECLARE(ElementPointOfInterest);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			ElementPointOfInterest();
			virtual ~ElementPointOfInterest();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			PointOfInterestPtr poi;
			ValueDef<Name> poiName;
			Name tagsParam;
		};
	};
};
