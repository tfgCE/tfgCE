#pragma once

#include "meshGenCreateCollisionInfo.h"
#include "meshGenCreateSpaceBlockerInfo.h"
#include "meshGenElement.h"
#include "meshGenValueDef.h"

#include "..\library\usedLibraryStored.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class MeshGenerator;

	namespace MeshGeneration
	{
		class ElementIncludeStack
		: public Element
		{
			FAST_CAST_DECLARE(ElementIncludeStack);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementIncludeStack();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			ValueDef<Name> stackElement;
			bool allowNoMeshGenerator = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			UsedLibraryStored<MeshGenerator> previewMeshGenerator;
#endif
			UsedLibraryStored<MeshGenerator> emptyMeshGenerator; // mesh generator if nothing is provided, useful to provide a cap for a missing element

			CreateCollisionInfo createMovementCollisionMesh;
			CreateCollisionInfo createMovementCollisionBox;
			CreateCollisionInfo createPreciseCollisionMesh;
			CreateCollisionInfo createPreciseCollisionMeshSkinned; // will create mesh skinned
			CreateCollisionInfo createPreciseCollisionBox;

			CreateSpaceBlockerInfo createSpaceBlocker;
		};
	};
};
