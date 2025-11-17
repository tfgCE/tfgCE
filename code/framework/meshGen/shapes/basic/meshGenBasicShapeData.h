#pragma once

#include "..\..\meshGenElementShape.h"
#include "..\..\meshGenCreateCollisionInfo.h"
#include "..\..\meshGenCreateSpaceBlockerInfo.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace BasicShapes
		{
			class Data
			: public ElementShapeData
			{
				FAST_CAST_DECLARE(Data);
				FAST_CAST_BASE(ElementShapeData);
				FAST_CAST_END();

				typedef ElementShapeData base;

			public:
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

			public:
				CreateCollisionInfo createMovementCollision; // will create collision primitive
				CreateCollisionInfo createMovementCollisionBox;
				CreateCollisionInfo createMovementCollisionMesh; // will create mesh basing on ipu
				CreateCollisionInfo createPreciseCollision; // will create collision primitive
				CreateCollisionInfo createPreciseCollisionBox;
				CreateCollisionInfo createPreciseCollisionMesh; // will create mesh basing on ipu
				CreateSpaceBlockerInfo createSpaceBlocker;
				bool noMesh = false;
				bool ignoreForCollision = false;
			};
		};
	};
};
