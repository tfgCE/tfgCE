#include "gameConfig.h"

#include "..\meshGen\meshGenConfig.h"

#include "..\..\core\copyToMainConfig.h"
#include "..\..\core\utils.h"
#include "..\..\core\debug\debug.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\io\xml.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(itemCollisionFlags);
DEFINE_STATIC_NAME(itemCollidesWithFlags);
DEFINE_STATIC_NAME(actorCollisionFlags);
DEFINE_STATIC_NAME(actorCollidesWithFlags);
DEFINE_STATIC_NAME(sceneryCollisionFlags);
DEFINE_STATIC_NAME(sceneryCollidesWithFlags);
DEFINE_STATIC_NAME(temporaryObjectCollisionFlags);
DEFINE_STATIC_NAME(temporaryObjectCollidesWithFlags);
DEFINE_STATIC_NAME(projectileCollidesWithFlags);
DEFINE_STATIC_NAME(pointOfInterestCollidesWithFlags);

//

CopyToMainConfig gameConfig_copyToMainConfig;

REGISTER_FOR_FAST_CAST(GameConfig);

GameConfig* GameConfig::s_config = nullptr;

void GameConfig::initialise_static()
{
	if (!s_config)
	{
		s_config = new GameConfig();
	}
}

void GameConfig::close_static()
{
	delete_and_clear(s_config);
}

GameConfig::GameConfig()
{
	todo_note(TXT("read from system"));
	systemDateTimeFormat.date = DateFormat::MDY;
	systemDateTimeFormat.time = TimeFormat::H12;
}

bool GameConfig::load_to_be_copied_to_main_config_xml(IO::XML::Node const* _node)
{
	return gameConfig_copyToMainConfig.load_to_be_copied_to_main_config_xml(_node);
}

void GameConfig::save_to_be_copied_to_main_config_xml(IO::XML::Node* _containerNode)
{
	gameConfig_copyToMainConfig.save_to_be_copied_to_main_config_xml(_containerNode);
}


bool GameConfig::load_from_xml(IO::XML::Node const * _node)
{
	an_assert(s_config != nullptr);
	return s_config->internal_load_from_xml(_node);
}

void GameConfig::save_to_xml(IO::XML::Node * _container, bool _userSpecific)
{
	an_assert(s_config != nullptr);
	IO::XML::Node* node = _container->add_node(TXT("gameConfig"));
	s_config->internal_save_to_xml(node, _userSpecific);
}

#define LOAD_COLLISION_FLAGS(flags, providedName) \
	providedName = _node->get_name_attribute_or_from_child(TXT(#flags), providedName);

#define SAVE_COLLISION_FLAGS(flags, providedName) \
	if (providedName.is_valid()) \
	{ \
		_node->set_string_to_child(TXT(#flags), providedName.to_string()); \
	}

bool GameConfig::internal_load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	result &= dateTimeFormat.load_from_xml(_node);
	
	result &= maxConcurrentApparentRoomSoundSources.load_from_xml(_node, TXT("maxConcurrentApparentRoomSoundSources"));

	autoGenerateLODs = _node->get_int_attribute_or_from_child(TXT("autoGenerateLODs"), autoGenerateLODs);
	minLODSize = _node->get_float_attribute_or_from_child(TXT("minLODSize"), minLODSize);
	lodSelectorStart = _node->get_float_attribute_or_from_child(TXT("lodSelectorStart"), lodSelectorStart);
	lodSelectorStep = _node->get_float_attribute_or_from_child(TXT("lodSelectorStep"), lodSelectorStep);

	advancementSuspensionRoomDistance = _node->get_int_attribute_or_from_child(TXT("advancementSuspensionRoomDistance"), advancementSuspensionRoomDistance);

	LOAD_COLLISION_FLAGS(itemCollisionFlags, providedItemCollisionFlags);
	LOAD_COLLISION_FLAGS(actorCollisionFlags, providedActorCollisionFlags);
	LOAD_COLLISION_FLAGS(sceneryCollisionFlags, providedSceneryCollisionFlags);
	LOAD_COLLISION_FLAGS(temporaryObjectCollisionFlags, providedTemporaryObjectCollisionFlags);
	LOAD_COLLISION_FLAGS(itemCollidesWithFlags, providedItemCollidesWithFlags);
	LOAD_COLLISION_FLAGS(actorCollidesWithFlags, providedActorCollidesWithFlags);
	LOAD_COLLISION_FLAGS(sceneryCollidesWithFlags, providedSceneryCollidesWithFlags);
	LOAD_COLLISION_FLAGS(temporaryObjectCollidesWithFlags, providedTemporaryObjectCollidesWithFlags);
	LOAD_COLLISION_FLAGS(projectileCollidesWithFlags, providedProjectileCollidesWithFlags);
	LOAD_COLLISION_FLAGS(pointOfInterestCollidesWithFlags, providedPointOfInterestCollidesWithFlags);

	MeshGeneration::Config::access().load_from_xml(_node);

	return result;
}

void GameConfig::internal_save_to_xml(IO::XML::Node * _node, bool _userSpecific) const
{
	dateTimeFormat.save_to_xml(_node);

	if (maxConcurrentApparentRoomSoundSources.is_set())
	{
		_node->set_int_to_child(TXT("maxConcurrentApparentRoomSoundSources"), maxConcurrentApparentRoomSoundSources.get());
	}

	if (!_userSpecific)
	{
		_node->set_int_to_child(TXT("autoGenerateLODs"), autoGenerateLODs);
		_node->set_float_to_child(TXT("minLODSize"), minLODSize);
		_node->set_float_to_child(TXT("lodSelectorStart"), lodSelectorStart);
		_node->set_float_to_child(TXT("lodSelectorStep"), lodSelectorStep);

		_node->set_int_to_child(TXT("advancementSuspensionRoomDistance"), advancementSuspensionRoomDistance);

		SAVE_COLLISION_FLAGS(itemCollisionFlags, providedItemCollisionFlags);
		SAVE_COLLISION_FLAGS(actorCollisionFlags, providedActorCollisionFlags);
		SAVE_COLLISION_FLAGS(sceneryCollisionFlags, providedSceneryCollisionFlags);
		SAVE_COLLISION_FLAGS(temporaryObjectCollisionFlags, providedTemporaryObjectCollisionFlags);
		SAVE_COLLISION_FLAGS(itemCollidesWithFlags, providedItemCollidesWithFlags);
		SAVE_COLLISION_FLAGS(actorCollidesWithFlags, providedActorCollidesWithFlags);
		SAVE_COLLISION_FLAGS(sceneryCollidesWithFlags, providedSceneryCollidesWithFlags);
		SAVE_COLLISION_FLAGS(temporaryObjectCollidesWithFlags, providedTemporaryObjectCollidesWithFlags);
		SAVE_COLLISION_FLAGS(projectileCollidesWithFlags, providedProjectileCollidesWithFlags);
		SAVE_COLLISION_FLAGS(pointOfInterestCollidesWithFlags, providedPointOfInterestCollidesWithFlags);

		MeshGeneration::Config::get().save_to_xml(_node, _userSpecific);
	}
}

void GameConfig::update_date_time_format(REF_ DateTimeFormat & _dateTimeFormat)
{
	if (GameConfig* config = fast_cast<GameConfig>(s_config))
	{
		if (config->dateTimeFormat.date != DateFormat::Default)
		{
			_dateTimeFormat.date = config->dateTimeFormat.date;
		}
		if (_dateTimeFormat.date == DateFormat::Default)
		{
			_dateTimeFormat.date = config->systemDateTimeFormat.date;
		}
		//
		if (config->dateTimeFormat.time != TimeFormat::Default)
		{
			_dateTimeFormat.time = config->dateTimeFormat.time;
		}
		if (_dateTimeFormat.time == TimeFormat::Default)
		{
			_dateTimeFormat.time = config->systemDateTimeFormat.time;
		}
	}
}

bool GameConfig::is_option_set(Name const & _option) const
{
	return false;
}

#define PREPARE_COLLISION_FLAGS(flags, providedName, defaultName) \
	flags.clear(); flags.add(providedName.is_valid()? providedName : defaultName);

void GameConfig::prepare_on_game_start()
{
	prepared = true;
	PREPARE_COLLISION_FLAGS(itemCollisionFlags, providedItemCollisionFlags, NAME(itemCollisionFlags));
	PREPARE_COLLISION_FLAGS(itemCollidesWithFlags, providedItemCollidesWithFlags, NAME(itemCollidesWithFlags));
	PREPARE_COLLISION_FLAGS(actorCollisionFlags, providedActorCollisionFlags, NAME(actorCollisionFlags));
	PREPARE_COLLISION_FLAGS(actorCollidesWithFlags, providedActorCollidesWithFlags, NAME(actorCollidesWithFlags));
	PREPARE_COLLISION_FLAGS(sceneryCollisionFlags, providedSceneryCollisionFlags, NAME(sceneryCollisionFlags));
	PREPARE_COLLISION_FLAGS(sceneryCollidesWithFlags, providedSceneryCollidesWithFlags, NAME(sceneryCollidesWithFlags));
	PREPARE_COLLISION_FLAGS(temporaryObjectCollisionFlags, providedTemporaryObjectCollisionFlags, NAME(temporaryObjectCollisionFlags));
	PREPARE_COLLISION_FLAGS(temporaryObjectCollidesWithFlags, providedTemporaryObjectCollidesWithFlags, NAME(temporaryObjectCollidesWithFlags));
	PREPARE_COLLISION_FLAGS(projectileCollidesWithFlags, providedProjectileCollidesWithFlags, NAME(projectileCollidesWithFlags));
	PREPARE_COLLISION_FLAGS(pointOfInterestCollidesWithFlags, providedPointOfInterestCollidesWithFlags, NAME(pointOfInterestCollidesWithFlags));
}
