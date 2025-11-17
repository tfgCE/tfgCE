#include "wmp_tools.h"

#include "..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\library\library.h"

#include "..\..\framework\library\usedLibraryStored.inl"

#include "wmp_gameDefinition.h"
#include "wmp_gameSettings.h"
#include "wmp_openWorld.h"
#include "wmp_overrideDoorDirectionsType.h"
#include "wmp_pilgrimageInstance.h"
#include "wmp_roomGeneratorBalconies.h"
#include "wmp_roomGeneratorInfo.h"
#include "wmp_roomGeneratorTowers.h"
#include "wmp_teaBoxSeed.h"
#include "wmp_vistaSceneryWindowPlacement.h"

using namespace TeaForGodEmperor;

void WheresMyPointTools::initialise_static()
{
	REGISTER_WHERES_MY_POINT_TOOL(TXT("doesAllowCrouch"), []() { return new DoesAllowCrouch(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("doesAllowAnyCrouch"), []() { return new DoesAllowAnyCrouch(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("gameDefinition"), []() { return new GameDefinitionForWheresMyPoint(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("gameSettings"), []() { return new GameSettingsForWheresMyPoint(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("isOpenWorld"), []() { return new IsOpenWorld(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("isSameDependentOnForOpenWorldCell"), []() { return new IsSameDependentOnForOpenWorldCell(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("openWorld"), []() { return new OpenWorld(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("overrideDoorDirectionsType"), []() { return new OverrideDoorDirectionsType(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("pilgrimageInstance"), []() { return new PilgrimageInstanceForWheresMyPoint(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("playAreaSize"), []() { return new PlayAreaSize(); }); // Vector2
	REGISTER_WHERES_MY_POINT_TOOL(TXT("playAreaZone"), []() { return new PlayAreaZone(); }); // Range2
	REGISTER_WHERES_MY_POINT_TOOL(TXT("roomGeneratorBalconies"), []() { return new RoomGeneratorBalconies(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("roomGeneratorTowers"), []() { return new RoomGeneratorTowers(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("teaBoxSeed"), []() { return new TeaBoxSeed(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("tileCount"), []() { return new TileCount(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("tileSize"), []() { return new TileSize(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("tilesZone"), []() { return new TilesZone(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("useOpenWorldCellRandomSeed"), []() { return new UseOpenWorldCellRandomSeed(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("vistaSceneryWindowPlacement"), []() { return new VistaSceneryWindowPlacement(); });
}
