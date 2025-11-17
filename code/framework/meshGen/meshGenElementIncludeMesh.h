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
		class ElementIncludeMesh
		: public Element
		{
			FAST_CAST_DECLARE(ElementIncludeMesh);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementIncludeMesh();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			struct IncludeMeshGenerator
			{
				float probabilityCoef = 1.0f;
				ValueDef<LibraryName> mesh; // can be empty
				TagCondition tagged;
			};
			ValueDef<int> randomNumber;
			Array<IncludeMeshGenerator> meshes;

			CreateCollisionInfo createMovementCollisionMesh;
			CreateCollisionInfo createMovementCollisionBox;
			CreateCollisionInfo createPreciseCollisionMesh;
			CreateCollisionInfo createPreciseCollisionMeshSkinned; // will create mesh skinned
			CreateCollisionInfo createPreciseCollisionBox;

			CreateSpaceBlockerInfo createSpaceBlocker;
		};
	};
};
