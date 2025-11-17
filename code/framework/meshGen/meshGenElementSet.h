#pragma once

#include "meshGenElement.h"
#include "meshGenCreateCollisionInfo.h"
#include "meshGenCreateSpaceBlockerInfo.h"
#include "meshGenSimplifyToBoxInfo.h"

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
		class ElementSet
		: public Element
		{
			FAST_CAST_DECLARE(ElementSet);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementSet();

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			Array<RefCountObjectPtr<Element>> elements;

			SimplifyToBoxInfo simplifyToBoxInfo;

			CreateCollisionInfo createMovementCollisionMesh;
			CreateCollisionInfo createMovementCollisionBox;
			CreateCollisionInfo createPreciseCollisionMesh;
			CreateCollisionInfo createPreciseCollisionMeshSkinned; // will create mesh skinned
			CreateCollisionInfo createPreciseCollisionBox;

			CreateSpaceBlockerInfo createSpaceBlocker;
		};
	};
};
