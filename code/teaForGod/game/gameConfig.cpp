#include "gameConfig.h"

#include "..\..\core\utils.h"
#include "..\..\core\debug\debug.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\io\xml.h"

#include "..\..\framework\module\moduleData.h"

#include "..\roomGenerators\roomGenerationInfo.h"

//

using namespace TeaForGodEmperor;

//

// tags
DEFINE_STATIC_NAME(npcCharacter);

//

REGISTER_FOR_FAST_CAST(GameConfig);

Energy GameConfig::apply_health_coef(Energy const& _to, Framework::ModuleData const* _for)
{
	if (auto* ls = _for->get_in_library_stored())
	{
		if (ls->get_tags().get_tag(NAME(npcCharacter)))
		{
			if (auto* gc = GameConfig::get_as<GameConfig>())
			{
				return _to.adjusted(gc->get_npc_character_health_coef());
			}
		}
	}
	return _to;
}

Energy GameConfig::apply_health_coef_inv(Energy const& _to, Framework::ModuleData const* _for)
{
	if (auto* ls = _for->get_in_library_stored())
	{
		if (ls->get_tags().get_tag(NAME(npcCharacter)))
		{
			if (auto* gc = GameConfig::get_as<GameConfig>())
			{
				return _to.adjusted(EnergyCoef::one() / gc->get_npc_character_health_coef());
			}
		}
	}
	return _to;
}


Energy GameConfig::apply_loot_energy_quantum_ammo(Energy const& _to)
{
	if (auto* gc = GameConfig::get_as<GameConfig>())
	{
		return _to.adjusted(gc->get_loot_energy_quantum_ammo_coef());
	}
	return _to;
}


void GameConfig::initialise_static()
{
	base *prev = s_config;
	s_config = new GameConfig();
	if (prev)
	{
		if (GameConfig* prevAs = fast_cast<GameConfig>(prev))
		{
			*s_config = *prevAs;
		}
		else
		{
			*s_config = *prev;
		}
		delete prev;
	}
}

GameConfig::GameConfig()
{
}

bool GameConfig::internal_load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::internal_load_from_xml(_node);

	result &= npcCharacterHealthCoef.load_from_attribute_or_from_child(_node, TXT("npcCharacterHealthCoef"), true);
	result &= lootEnergyQuantumAmmoCoef.load_from_attribute_or_from_child(_node, TXT("lootEnergyQuantumAmmoCoef"), true);

	playerProfileName = _node->get_string_attribute_or_from_child(TXT("playerProfile"), playerProfileName);

	gamepadCameraSpeed = _node->get_float_attribute_or_from_child(TXT("gamepadCameraSpeed"), gamepadCameraSpeed);

	RoomGenerationInfo::access().load_from_xml(_node);

	vrOnScreenWithInfo = _node->get_bool_attribute_or_from_child(TXT("vrOnScreenWithInfo"), vrOnScreenWithInfo);

	for_every(n, _node->children_named(TXT("secondaryView")))
	{
		secondaryViewInfo = SecondaryViewInfo();
		secondaryViewInfo.show = n->get_bool_attribute_or_from_child(TXT("show"), secondaryViewInfo.show);
		secondaryViewInfo.mainSize = n->get_float_attribute_or_from_child(TXT("mainSize"), secondaryViewInfo.mainSize);
		secondaryViewInfo.mainSizeAuto = n->get_bool_attribute_or_from_child(TXT("mainSizeAuto"), secondaryViewInfo.mainSizeAuto);
		secondaryViewInfo.pip = n->get_bool_attribute_or_from_child(TXT("pip"), secondaryViewInfo.pip);
		secondaryViewInfo.pipSwap = n->get_bool_attribute_or_from_child(TXT("pipSwap"), secondaryViewInfo.pipSwap);
		secondaryViewInfo.pipSize = n->get_float_attribute_or_from_child(TXT("pipSize"), secondaryViewInfo.pipSize);
		secondaryViewInfo.mainZoom = n->get_bool_attribute_or_from_child(TXT("mainZoom"), secondaryViewInfo.mainZoom);
		secondaryViewInfo.secondaryZoom = n->get_bool_attribute_or_from_child(TXT("secondaryZoom"), secondaryViewInfo.secondaryZoom);
		secondaryViewInfo.mainAt.load_from_xml(n, TXT("mainAtX"), TXT("mainAtY"));
		secondaryViewInfo.secondaryAt.load_from_xml(n, TXT("secondaryAtX"), TXT("secondaryAtY"));
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	for_every(n, _node->children_named(TXT("externalView")))
	{
		externalViewInfo = ExternalViewInfo();
		if (auto* attr = n->get_attribute(TXT("showBoundary")))
		{
			externalViewInfo.showBoundary = ExternalViewShowBoundary::parse(attr->get_as_string());
		}
	}
#endif

	{
		String a = _node->get_string_attribute_or_from_child(TXT("allowVRScreenShots"));
		if (!a.is_empty())
		{
			allowVRScreenShots = AllowScreenShots::parse(a);
		}
	}
	{
		String v = _node->get_string_attribute_or_from_child(TXT("hubPointWithWholeHand"));
		if (!v.is_empty())
		{
			hubPointWithWholeHand = Option::parse(v.to_char(), Option::Auto, Option::Auto | Option::True | Option::False);
			hubPointWithWholeHand = Option::Auto; // keep it auto
		}
	}

	subtitlesLineLimit = _node->get_int_attribute_or_from_child(TXT("subtitlesLineLimit"), subtitlesLineLimit);

	return result;
}

void GameConfig::internal_save_to_xml(IO::XML::Node * _node, bool _userSpecific) const
{
	base::internal_save_to_xml(_node, _userSpecific);

	if (!_userSpecific)
	{
		_node->set_string_to_child(TXT("npcCharacterHealthCoef"), npcCharacterHealthCoef.as_string_percentage());
		_node->set_string_to_child(TXT("lootEnergyQuantumAmmoCoef"), lootEnergyQuantumAmmoCoef.as_string_percentage());
	}

	if (!playerProfileName.is_empty())
	{
		_node->set_attribute(TXT("playerProfile"), playerProfileName);
	}

	//_node->set_float_to_child(TXT("gamepadCameraSpeed"), gamepadCameraSpeed); // we should not use it?

	RoomGenerationInfo::get().save_to_xml(_node);

	_node->set_bool_to_child(TXT("vrOnScreenWithInfo"), vrOnScreenWithInfo);

	if (auto* n = _node->add_node(TXT("secondaryView")))
	{
		n->set_bool_attribute(TXT("show"), secondaryViewInfo.show);
		n->set_float_attribute(TXT("mainSize"), secondaryViewInfo.mainSize);
		n->set_bool_attribute(TXT("mainSizeAuto"), secondaryViewInfo.mainSizeAuto);
		n->set_bool_attribute(TXT("pip"), secondaryViewInfo.pip);
		n->set_bool_attribute(TXT("pipSwap"), secondaryViewInfo.pipSwap);
		n->set_float_attribute(TXT("pipSize"), secondaryViewInfo.pipSize);
		n->set_bool_attribute(TXT("mainZoom"), secondaryViewInfo.mainZoom);
		n->set_bool_attribute(TXT("secondaryZoom"), secondaryViewInfo.secondaryZoom);
		secondaryViewInfo.mainAt.save_to_xml(n, TXT("mainAtX"), TXT("mainAtY"));
		secondaryViewInfo.secondaryAt.save_to_xml(n, TXT("secondaryAtX"), TXT("secondaryAtY"));
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (auto* n = _node->add_node(TXT("externalView")))
	{
		n->set_attribute(TXT("showBoundary"), ExternalViewShowBoundary::to_char(externalViewInfo.showBoundary));
	}
#endif

	_node->set_string_to_child(TXT("allowVRScreenShots"), AllowScreenShots::to_char(allowVRScreenShots));
	_node->set_string_to_child(TXT("hubPointWithWholeHand"), Option::to_char(hubPointWithWholeHand));

	if (!_userSpecific)
	{
		_node->set_int_to_child(TXT("subtitlesLineLimit"), subtitlesLineLimit);
	}
}

void GameConfig::next_hub_point_with_whole_hand()
{
	if (hubPointWithWholeHand == Option::Auto)
	{
		hubPointWithWholeHand = Option::False;
	}
	else if (hubPointWithWholeHand == Option::False)
	{
		hubPointWithWholeHand = Option::True;
	}
	else
	{
		hubPointWithWholeHand = Option::Auto;
	}
}
