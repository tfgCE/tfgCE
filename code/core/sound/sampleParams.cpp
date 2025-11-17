#include "sampleParams.h"

#include "playback.h"

#include "..\utils.h"
#include "..\mainSettings.h"

#include "..\io\file.h"
#include "..\io\xml.h"
#include "..\system\sound\soundSystem.h"
#include "..\types\string.h"
#include "..\serialisation\serialiser.h"

using namespace ::Sound;

SampleParams::SampleParams()
{
}

#define LOAD_CUSTOM_BOOL_PARAM(_var, _paramName, _paramValue) \
	{ \
		String param = _node->get_string_attribute_or_from_child(_paramName); \
		if (!param.is_empty()) \
		{ \
			_var = String::compare_icase(param, _paramValue); \
		} \
	}

bool SampleParams::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	looped = _node->get_bool_attribute_or_from_child(TXT("looped"), looped);
	isStream = _node->get_bool_attribute_or_from_child(TXT("stream"), looped);
	LOAD_CUSTOM_BOOL_PARAM(is3D, TXT("type"), TXT("3d"));
	LOAD_CUSTOM_BOOL_PARAM(useAs3D, TXT("type"), TXT("2dUseAs3d"));
	LOAD_CUSTOM_BOOL_PARAM(headRelative, TXT("relativeTo"), TXT("head"));
	dopplerLevel = _node->get_float_attribute_or_from_child(TXT("dopplerLevel"), dopplerLevel);
	volume.load_from_xml(_node, TXT("volume"));
	pitch.load_from_xml(_node, TXT("pitch"));
	channel = _node->get_name_attribute_or_from_child(TXT("channel"), channel);
	usesReverbs.load_from_xml(_node, TXT("reverb"));
	if (_node->first_child_named(TXT("reverb")))
	{
		usesReverbs = _node->get_bool_attribute_or_from_child_presence(TXT("reverb"), usesReverbs.get(true));
	}
	if (_node->first_child_named(TXT("noReverb")))
	{
		usesReverbs = ! _node->get_bool_attribute_or_from_child_presence(TXT("noReverb"), !usesReverbs.get(false));
	}
	distances.load_from_xml(_node, TXT("distances"));
	if (IO::XML::Node const * node = _node->first_child_named(TXT("distances")))
	{
		distances.min = node->get_float_attribute(TXT("min"), distances.min);
		distances.max = node->get_float_attribute(TXT("max"), distances.max);
	}
	priority = _node->get_int_attribute_or_from_child(TXT("priority"), priority);
	if (priority < -127 || priority > 127)
	{
		warn_loading_xml(_node, TXT("priority for sounds should not exceed -127, 127 range"));
	}
	priority = clamp(priority, -127, 127);

	return result;
}

#define VER_BASE 0
#define VER_VOLUME_PITCH 1
#define VER_PRIORITY 2
#define VER_DOPPLER_LEVEL 3
#define CURRENT_VERSION VER_DOPPLER_LEVEL

bool SampleParams::serialise(Serialiser & _serialiser)
{
	bool result = true;
	int version = CURRENT_VERSION;
	serialise_data(_serialiser, version);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid sound version"));
		return false;
	}
	result &= serialise_data(_serialiser, looped);
	result &= serialise_data(_serialiser, is3D);
	result &= serialise_data(_serialiser, channel);
	result &= serialise_data(_serialiser, distances);
	if (version >= VER_VOLUME_PITCH)
	{
		result &= serialise_data(_serialiser, volume);
		result &= serialise_data(_serialiser, pitch);
	}
	if (version >= VER_PRIORITY)
	{
		result &= serialise_data(_serialiser, priority);
	}
	if (version >= VER_DOPPLER_LEVEL)
	{
		result &= serialise_data(_serialiser, dopplerLevel);
	}
	return result;
}

void SampleParams::prepare_create_sound_flags(REF_ int & _flags) const
{
#ifdef AN_FMOD
	// loop
	clear_flag(_flags, FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI);
	set_flag(_flags, looped? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

	// 2d/3d
	clear_flag(_flags, FMOD_2D | FMOD_3D);
	set_flag(_flags, is3D ? FMOD_3D : FMOD_2D);

	if (is3D)
	{
		// relative to head/world
		clear_flag(_flags, FMOD_3D_HEADRELATIVE | FMOD_3D_WORLDRELATIVE);
		set_flag(_flags, headRelative ? FMOD_3D_HEADRELATIVE : FMOD_3D_WORLDRELATIVE);
	}
#endif
}

