#pragma once

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
		class ElementRayCast
		: public Element
		{
			FAST_CAST_DECLARE(ElementRayCast);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementRayCast();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			enum Target
			{
				Mesh,
				MovementCollision,
				PreciseCollision,
			};
			bool againstAll = false; // check against all created meshes, not only last checkpoint
			Name againstNamedCheckpoint; // will use named checkpoint as a base
			Target against = Target::Mesh;
			int againstCheckpointsUp = 0; // to get extra checkpoints (if we know how much there are)

			Name outParam; // will be vector3
			Name outNormalParam; // will be vector3
			Name outHitParam; // will be bool

			bool debugDraw = false;
			bool debugDrawDetailed = false;
		};
	};
};
