#include "reverb.h"

#include "..\io\xml.h"

using namespace ::Sound;

Reverb::Reverb()
{
}

Reverb::~Reverb()
{
}

bool Reverb::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	power = _node->get_float_attribute(TXT("power"), power);

	String preset = _node->get_string_attribute_or_from_child(TXT("preset"));

	if (!preset.is_empty())
	{
#ifdef AN_FMOD
		if (preset == TXT("off"))				{ properties = FMOD_PRESET_OFF; } else
		if (preset == TXT("generic"))			{ properties = FMOD_PRESET_GENERIC; } else
		if (preset == TXT("padded cell"))		{ properties = FMOD_PRESET_PADDEDCELL; } else
		if (preset == TXT("room"))				{ properties = FMOD_PRESET_ROOM; } else
		if (preset == TXT("bathroom"))			{ properties = FMOD_PRESET_BATHROOM; } else
		if (preset == TXT("living room"))		{ properties = FMOD_PRESET_LIVINGROOM; } else
		if (preset == TXT("stone room"))		{ properties = FMOD_PRESET_STONEROOM; } else
		if (preset == TXT("auditorium"))		{ properties = FMOD_PRESET_AUDITORIUM; } else
		if (preset == TXT("concert hall"))		{ properties = FMOD_PRESET_CONCERTHALL; } else
		if (preset == TXT("cave"))				{ properties = FMOD_PRESET_CAVE; } else
		if (preset == TXT("arena"))				{ properties = FMOD_PRESET_ARENA; } else
		if (preset == TXT("hangar"))			{ properties = FMOD_PRESET_HANGAR; } else
		if (preset == TXT("carpetted hallway"))	{ properties = FMOD_PRESET_CARPETTEDHALLWAY; } else
		if (preset == TXT("hallway"))			{ properties = FMOD_PRESET_HALLWAY; } else
		if (preset == TXT("stone corridor"))	{ properties = FMOD_PRESET_STONECORRIDOR; } else
		if (preset == TXT("alley"))				{ properties = FMOD_PRESET_ALLEY; } else
		if (preset == TXT("forest"))			{ properties = FMOD_PRESET_FOREST; } else
		if (preset == TXT("city"))				{ properties = FMOD_PRESET_CITY; } else
		if (preset == TXT("mountains"))			{ properties = FMOD_PRESET_MOUNTAINS; } else
		if (preset == TXT("quarry"))			{ properties = FMOD_PRESET_QUARRY; } else
		if (preset == TXT("plain"))				{ properties = FMOD_PRESET_PLAIN; } else
		if (preset == TXT("parking lot"))		{ properties = FMOD_PRESET_PARKINGLOT; } else
		if (preset == TXT("sewer pipe"))		{ properties = FMOD_PRESET_SEWERPIPE; } else
		if (preset == TXT("underwater"))		{ properties = FMOD_PRESET_UNDERWATER; } else
		{
			error_loading_xml(_node, TXT("preset \"%S\" not recognised"), preset.to_char());
			result = false;
		}
#endif
	}

	return result;
}
