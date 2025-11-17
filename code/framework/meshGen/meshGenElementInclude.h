#pragma once

#include "meshGenCreateCollisionInfo.h"
#include "meshGenCreateSpaceBlockerInfo.h"
#include "meshGenElement.h"
#include "meshGenValueDef.h"

#include "..\library\usedLibraryStored.h"

#include "..\..\core\tags\tagCondition.h"

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
		class ElementInclude
		: public Element
		{
			FAST_CAST_DECLARE(ElementInclude);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementInclude();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			struct IncludeMeshGenerator
			{
				float probabilityCoef = 1.0f;
				ValueDef<LibraryName> meshGenerator; // can be empty
				TagCondition tagged;
			};
			ValueDef<int> randomNumber;
			Array<IncludeMeshGenerator> meshGenerators;

			CreateCollisionInfo createMovementCollisionMesh;
			CreateCollisionInfo createMovementCollisionBox;
			CreateCollisionInfo createPreciseCollisionMesh;
			CreateCollisionInfo createPreciseCollisionMeshSkinned; // will create mesh skinned
			CreateCollisionInfo createPreciseCollisionBox;

			CreateSpaceBlockerInfo createSpaceBlocker;
		};
	};
};
