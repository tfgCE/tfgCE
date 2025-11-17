#include "wmp_tools.h"

#include "..\..\core\wheresMyPoint\iWMPTool.h"

#include "wmp_allowMoreDetails.h"
#include "wmp_applyBoneRenamer.h"
#include "wmp_bonePlacement.h"
#include "wmp_chooseLibraryStored.h"
#include "wmp_debugDrawArrow.h"
#include "wmp_debugDrawLine.h"
#include "wmp_debugDrawTransform.h"
#include "wmp_doesBoneExist.h"
#include "wmp_environment.h"
#include "wmp_getParameterFromLibraryStored.h"
#include "wmp_hasDoor.h"
#include "wmp_isLibraryNameValid.h"
#include "wmp_libraryName.h"
#include "wmp_lod.h"
#include "wmp_makeBoneNameUnique.h"
#include "wmp_makeSocketNameUnique.h"
#include "wmp_meshNode.h"
#include "wmp_nominalDoorSize.h"
#include "wmp_previewGame.h"
#include "wmp_randomGenerator.h"
#include "wmp_regionGenerator.h"
#include "wmp_slidingLocomotion.h"
#include "wmp_uniqueID.h"

using namespace Framework;

void WheresMyPointTools::initialise_static()
{
	REGISTER_WHERES_MY_POINT_TOOL(TXT("allowMoreDetails"), [](){ return new AllowMoreDetails(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("applyBoneRenamer"), [](){ return new ApplyBoneRenamer(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("addEnvironmentOverlay"), []() { return new AddEnvironmentOverlay(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("bonePlacement"), [](){ return new BonePlacement(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("chooseLibraryStored"), []() { return new ChooseLibraryStored(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("clearEnvironmentOverlays"), []() { return new ClearEnvironmentOverlays(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("debugDrawArrow"), [](){ return new DebugDrawArrow(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("debugDrawLine"), [](){ return new DebugDrawLine(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("debugDrawTransform"), [](){ return new DebugDrawTransform(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("doesBoneExist"), [](){ return new DoesBoneExist(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("getParameterFromLibraryStored"), []() { return new GetParameterFromLibraryStored(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("hasDoor"), []() { return new HasDoor(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("isLibraryNameValid"), [](){ return new IsLibraryNameValid(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("ifLibraryNameEmpty"), []() { return new IfLibraryNameEmpty(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("ifLibraryNameValid"), []() { return new IfLibraryNameValid(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("libraryName"), []() { return new LibraryNameWMP(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("lod"), [](){ return new LOD(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("makeBoneNameUnique"), [](){ return new MakeBoneNameUnique(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("makeSocketNameUnique"), [](){ return new MakeSocketNameUnique(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("meshNodeName"), [](){ return new MeshNodeName(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("meshNodePlacement"), [](){ return new MeshNodePlacement(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("meshNodeRestore"), [](){ return new MeshNodeRestore(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("meshNodeStore"), [](){ return new MeshNodeStore(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("nominalDoorHeight"), []() { return new NominalDoorHeight(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("nominalDoorHeightScale"), []() { return new NominalDoorHeightScale(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("nominalDoorWidth"), []() { return new NominalDoorWidth(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("previewGame"), []() { return new PreviewGameWMP(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("regionGeneratorOuterConnectorsCount"), []() { return new RegionGeneratorOuterConnectorsCount(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("regionGeneratorCheckGenerationTags"), []() { return new RegionGeneratorCheckGenerationTags(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("slidingLocomotion"), []() { return new SlidingLocomotion(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("spawnRandomGenerator"), []() { return new SpawnRandomGenerator(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("uniqueID"), []() { return new UniqueID(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("usedEnvironmentFromRegion"), []() { return new UsedEnvironmentFromRegion(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("useEnvironmentFromRegion"), []() { return new UseEnvironmentFromRegion(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("useRandomGenerator"), []() { return new UseRandomGenerator(); });
}
