#include "mc_gameSettingsDependant.h"

#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleAppearanceImpl.inl"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module parameters
DEFINE_STATIC_NAME(setting);
DEFINE_STATIC_NAME(option);

// settings
DEFINE_STATIC_NAME(realityAnchors);

//

REGISTER_FOR_FAST_CAST(GameSettingsDependant);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new GameSettingsDependant(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & GameSettingsDependant::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("gameSettingsDependant")), create_module);
}

GameSettingsDependant::GameSettingsDependant(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

GameSettingsDependant::~GameSettingsDependant()
{
}

void GameSettingsDependant::reset()
{
	base::reset();
	update();
}

void GameSettingsDependant::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		setting = _moduleData->get_parameter<Name>(this, NAME(setting), setting);
		setting = _moduleData->get_parameter<Name>(this, NAME(option), setting);

		if (setting != NAME(realityAnchors))
		{
			error(TXT("setting \"%S\" not recognised"), setting.to_char());
		}
	}
}

void GameSettingsDependant::initialise()
{
	base::initialise();
	update();
}

void GameSettingsDependant::on_game_settings_channged(GameSettings& _gameSettings)
{
	update();
}

void GameSettingsDependant::update()
{
	if (setting == NAME(realityAnchors))
	{
		for_every_ptr(appearance, get_owner()->get_appearances())
		{
			appearance->be_visible(get_game_settings().settings.realityAnchors);
		}
	}
}