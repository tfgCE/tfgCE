#include "frameworkRegisteredTypes.h"

#define AN_REGISTER_TYPES_OFFSET 2000

#include "..\core\memory\safeObject.h"
#include "..\core\other\registeredTypeRegistering.h"
#include "..\core\other\parserUtils.h"
#include "..\core\other\parserUtilsImpl.inl"

#include "library\libraryName.h"

#include "time\dateTime.h"

#include "ai\aiMessageHandler.h"
#include "ai\aiMindInstance.h"
#include "ai\aiTaskHandle.h"
#include "display\displayText.h"
#include "editor\editors\editorSprite.h"
#include "editor\editors\editorSpriteGrid.h"
#include "editor\editors\editorVoxelMold.h"
#include "general\cooldowns.h"
#include "module\custom\mc_lightningSpawner.h"
#include "nav\navPath.h"
#include "nav\navTask.h"
#include "presence\presencePath.h"
#include "presence\relativeToPresencePlacement.h"
#include "sound\soundSource.h"
#include "world\door.h"
#include "world\doorInRoom.h"
#include "world\inRoomPlacement.h"
#include "world\region.h"
#include "world\room.h"
#include "world\worldAddress.h"
#include "world\worldSettingCondition.h"

namespace Framework
{
	interface_class IModulesOwner;
	class IAIObject;
	class Actor;
	class Door;
	class DoorInRoom;
	class Item;
	class Object;
	class PresenceLink;
	class Scenery;
	struct PointOfInterestInstance;
	
	namespace AppearanceControllersLib
	{
		class PoseManager;
	};
};

using namespace Framework;

// some defaults
AI::LatentTaskHandle aiLatentTaskHandle;
AI::RegisteredLatentTaskHandles aiRegisteredLatentTaskHandles;
StaticDoorArray staticDoorArray;
WorldAddress worldAddress;

//

// should be mirrored in headers with DECLARE_REGISTERED_TYPE
DEFINE_TYPE_LOOK_UP(Editor::Sprite::Mode::Type);
DEFINE_TYPE_LOOK_UP(Editor::SpriteGrid::Mode::Type);
DEFINE_TYPE_LOOK_UP(Editor::VoxelMold::Mode::Type);
DEFINE_TYPE_LOOK_UP_OBJECT(AI::MessageHandler, &AI::MessageHandler::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(AI::LatentTaskHandle, &aiLatentTaskHandle);
DEFINE_TYPE_LOOK_UP_OBJECT(AI::RegisteredLatentTaskHandles, &aiRegisteredLatentTaskHandles);
DEFINE_TYPE_LOOK_UP_OBJECT(CooldownsSmall, &CooldownsSmall::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(PresencePath, &PresencePath::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(RelativeToPresencePlacement, &RelativeToPresencePlacement::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(InRoomPlacement, &InRoomPlacement::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(WorldSettingCondition, &WorldSettingCondition::empty());
DEFINE_TYPE_LOOK_UP_OBJECT(StaticDoorArray, &staticDoorArray);
DEFINE_TYPE_LOOK_UP_OBJECT(WorldAddress, &worldAddress);
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(DateTime, &DateTime::none());
DEFINE_TYPE_LOOK_UP_PLAIN_DATA(LibraryName, &LibraryName::invalid());
DEFINE_TYPE_LOOK_UP_PTR(Actor);
DEFINE_TYPE_LOOK_UP_PTR(AI::LatentTaskHandle);
DEFINE_TYPE_LOOK_UP_PTR(AI::Logic);
DEFINE_TYPE_LOOK_UP_PTR(AI::MindInstance);
DEFINE_TYPE_LOOK_UP_PTR(CustomModules::LightningSpawner);
DEFINE_TYPE_LOOK_UP_PTR(Door);
DEFINE_TYPE_LOOK_UP_PTR(DoorInRoom);
DEFINE_TYPE_LOOK_UP_PTR(IAIObject);
DEFINE_TYPE_LOOK_UP_PTR(InRoomPlacement);
DEFINE_TYPE_LOOK_UP_PTR(Item);
DEFINE_TYPE_LOOK_UP_PTR(Object);
DEFINE_TYPE_LOOK_UP_PTR(PresenceLink);
DEFINE_TYPE_LOOK_UP_PTR(PresencePath);
DEFINE_TYPE_LOOK_UP_PTR(RelativeToPresencePlacement);
DEFINE_TYPE_LOOK_UP_PTR(Region);
DEFINE_TYPE_LOOK_UP_PTR(Room);
DEFINE_TYPE_LOOK_UP_PTR(Scenery);
DEFINE_TYPE_LOOK_UP_REF_COUNT_OBJECT_PTR(DisplayText);
DEFINE_TYPE_LOOK_UP_REF_COUNT_OBJECT_PTR(Nav::Path);
DEFINE_TYPE_LOOK_UP_REF_COUNT_OBJECT_PTR(Nav::Task);
DEFINE_TYPE_LOOK_UP_REF_COUNT_OBJECT_PTR(SoundSource);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(IAIObject);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(Door);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(DoorInRoom);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(IModulesOwner);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(AppearanceControllersLib::PoseManager);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(PointOfInterestInstance);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(Region);
DEFINE_TYPE_LOOK_UP_SAFE_PTR(Room);

namespace Framework
{
	namespace AI
	{
	};
};

//

void ::Framework::RegisteredTypes::initialise_static()
{
}

void ::Framework::RegisteredTypes::close_static()
{
}

//
//
// combined parsing, saving, loading, logging

ADD_SPECIALISATIONS_FOR_ENUM_USING_TO_CHAR(Editor::Sprite::Mode);
ADD_SPECIALISATIONS_FOR_ENUM_USING_TO_CHAR(Editor::SpriteGrid::Mode);
ADD_SPECIALISATIONS_FOR_ENUM_USING_TO_CHAR(Editor::VoxelMold::Mode);

//
//
// parsing

void parse_value__library_name(String const & _from, void * _into)
{
	CAST_TO_VALUE(LibraryName, _into) = LibraryName::from_string(_from);
}
ADD_SPECIALISED_PARSE_VALUE(LibraryName, parse_value__library_name);

//
//
// saving to xml

bool save_to_xml__library_name(IO::XML::Node* _node, void const* _value)
{
	_node->add_text(CAST_TO_VALUE(LibraryName, _value).to_string());
	return true;
}
ADD_SPECIALISED_SAVE_TO_XML(LibraryName, save_to_xml__library_name);

//
//
// loading from xml

bool load_from_xml__library_name(IO::XML::Node const * _node, void * _into)
{
	CAST_TO_VALUE(LibraryName, _into) = LibraryName::from_string(_node->get_internal_text());
	return true;
}
ADD_SPECIALISED_LOAD_FROM_XML(LibraryName, load_from_xml__library_name);

ADD_SPECIALISED_LOAD_FROM_XML_USING(WorldSettingCondition, load_from_xml);

//
//
// logging

void log_value__library_name(LogInfoContext & _log, void const * _value, tchar const * _name)
{
	if (_name)
	{
		_log.log(TXT("%S [LibraryName] : %S"), _name, CAST_TO_VALUE(LibraryName, _value).to_string().to_char());
	}
	else
	{
		_log.log(TXT("[LibraryName] : %S"), CAST_TO_VALUE(LibraryName, _value).to_string().to_char());
	}
}
ADD_SPECIALISED_LOG_VALUE(LibraryName, log_value__library_name);

ADD_SPECIALISED_LOG_VALUE_USING_LOG(PresencePath);
ADD_SPECIALISED_LOG_VALUE_USING_LOG(RelativeToPresencePlacement);
ADD_SPECIALISED_LOG_VALUE_USING_LOG(InRoomPlacement);
ADD_SPECIALISED_LOG_VALUE_USING_LOG(WorldSettingCondition);
ADD_SPECIALISED_LOG_VALUE_USING_LOG(AI::LatentTaskHandle);
ADD_SPECIALISED_LOG_VALUE_USING_LOG_PTR(PresencePath);
ADD_SPECIALISED_LOG_VALUE_USING_LOG_PTR(RelativeToPresencePlacement);
ADD_SPECIALISED_LOG_VALUE_USING_LOG_PTR(InRoomPlacement);
ADD_SPECIALISED_LOG_VALUE_USING(::SafePtr<Framework::IModulesOwner>, Framework::IModulesOwner::log);
ADD_SPECIALISED_LOG_VALUE_USING(::SafePtr<Framework::Room>, Framework::Room::log);
ADD_SPECIALISED_LOG_VALUE_USING(::SafePtr<Framework::Region>, Framework::Region::log);
ADD_SPECIALISED_LOG_VALUE_USING(Framework::CooldownsSmall, Framework::CooldownsSmall::log);
