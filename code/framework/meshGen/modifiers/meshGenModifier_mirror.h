#include "meshGenModifierUtils.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace Modifiers
		{
			/**
			 *	Allows:
			 *		mirror using mirror axis and point
			 */
			class MeshMirrorData
			: public ElementModifierData
			{
				FAST_CAST_DECLARE(MeshMirrorData);
				FAST_CAST_BASE(ElementModifierData);
				FAST_CAST_END();

				typedef ElementModifierData base;
			public:
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

			public: // we won't be exposed anywhere else, so we may be public here!
				ValueDef<Vector3> mirrorDir = ValueDef<Vector3>(Vector3::zero);
				ValueDef<Vector3> mirrorPoint = ValueDef<Vector3>(Vector3::zero);

				bool dontCutMeshNodes = false;

				int flags = Modifiers::Utils::Flags::All;

				Meshes::BoneRenamer boneRenamer;
			};
		};
	};

};
