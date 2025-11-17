#pragma once

#include "meshGenElement.h"
#include "meshGenValueDef.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Meshes
{
	class Skeleton;
};

namespace Framework
{
	class AppearanceControllerData;

	namespace MeshGeneration
	{
		class ElementAppearanceController
		: public Element
		{
			FAST_CAST_DECLARE(ElementAppearanceController);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			ElementAppearanceController();
			virtual ~ElementAppearanceController();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			AppearanceControllerData* controller = nullptr;

		};
	};
};
