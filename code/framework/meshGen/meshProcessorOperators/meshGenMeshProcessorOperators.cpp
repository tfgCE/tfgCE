#include "..\meshGenElementMeshProcessor.h"

#include "meshGenMeshProcOp_changeMaterial.h"
#include "meshGenMeshProcOp_closeConvexGapOnPlane.h"
#include "meshGenMeshProcOp_cut.h"
#include "meshGenMeshProcOp_deform.h"
#include "meshGenMeshProcOp_delete.h"
#include "meshGenMeshProcOp_dropVerticesOnPlane.h"
#include "meshGenMeshProcOp_duplicate.h"
#include "meshGenMeshProcOp_extrude.h"
#include "meshGenMeshProcOp_numberCustomData.h"
#include "meshGenMeshProcOp_removeSurfaces.h"
#include "meshGenMeshProcOp_reverseSurfaces.h"
#include "meshGenMeshProcOp_scale.h"
#include "meshGenMeshProcOp_scaleVerticesOnPlane.h"
#include "meshGenMeshProcOp_select.h"
#include "meshGenMeshProcOp_setCustomData.h"
#include "meshGenMeshProcOp_setCustomDataBetweenPoints.h"
#include "meshGenMeshProcOp_setCustomDataDistanceBased.h"
#include "meshGenMeshProcOp_setCustomDataNormalBased.h"
#include "meshGenMeshProcOp_setU.h"
#include "meshGenMeshProcOp_skin.h"
#include "meshGenMeshProcOp_skinBetween.h"
#include "meshGenMeshProcOp_split.h"
#include "meshGenMeshProcOp_weld.h"

//

#define ADD(name, ClassName) RegisteredElementMeshProcessorOperators::add(String(TXT(#name)), [](){ return new ClassName(); })

namespace Framework
{
	namespace MeshGeneration
	{
		namespace MeshProcessorOperators
		{
			void add_operators()
			{
				ADD(changeMaterial, MeshProcessorOperators::ChangeMaterial);
				ADD(closeConvexGapOnPlane, MeshProcessorOperators::CloseConvexGapOnPlane);
				ADD(cut, MeshProcessorOperators::Cut);
				ADD(deform, MeshProcessorOperators::Deform);
				ADD(delete, MeshProcessorOperators::Delete);
				ADD(duplicate, MeshProcessorOperators::Duplicate);
				ADD(dropVerticesOnPlane, MeshProcessorOperators::DropVerticesOnPlane);
				ADD(extrude, MeshProcessorOperators::Extrude);
				ADD(numberCustomData, MeshProcessorOperators::NumberCustomData);
				ADD(removeSurfaces, MeshProcessorOperators::RemoveSurfaces);
				ADD(reverseSurfaces, MeshProcessorOperators::ReverseSurfaces);
				ADD(scale, MeshProcessorOperators::Scale);
				ADD(scaleVerticesOnPlane, MeshProcessorOperators::ScaleVerticesOnPlane);
				ADD(select, MeshProcessorOperators::Select);
				ADD(setCustomData, MeshProcessorOperators::SetCustomData);
				ADD(setCustomDataBetweenPoints, MeshProcessorOperators::SetCustomDataBetweenPoints);
				ADD(setCustomDataDistanceBased, MeshProcessorOperators::SetCustomDataDistanceBased);
				ADD(setCustomDataNormalBased, MeshProcessorOperators::SetCustomDataNormalBased);
				ADD(setU, MeshProcessorOperators::SetU);
				ADD(skin, MeshProcessorOperators::Skin);
				ADD(skinBetween, MeshProcessorOperators::SkinBetween);
				ADD(split, MeshProcessorOperators::Split);
				ADD(weld, MeshProcessorOperators::Weld);
			}
		};
	};
};
