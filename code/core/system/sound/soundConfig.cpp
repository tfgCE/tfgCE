#include "soundConfig.h"

#include "..\..\recordVideoSettings.h"

#include "..\..\io\xml.h"
#include "..\..\mainSettings.h"

using namespace System;

SoundConfig::SoundConfig()
{
}

bool SoundConfig::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (!_node)
	{
		return result;
	}
	enableOcclusionLowPass = _node->get_bool_attribute_or_from_child(TXT("enableOcclusionLowPass"), enableOcclusionLowPass);
	maxConcurrentSounds = _node->get_int_attribute_or_from_child(TXT("maxConcurrentSounds"), maxConcurrentSounds);

	todo_note(TXT("change this to read channel volume"));

	volume = _node->get_float_attribute_or_from_child(TXT("volume"), volume);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	developmentVolumeVR = _node->get_float_attribute_or_from_child(TXT("developmentVolume"), developmentVolumeVR);
	developmentVolumeNonVR = _node->get_float_attribute_or_from_child(TXT("developmentVolume"), developmentVolumeNonVR);
	developmentVolumeVR = _node->get_float_attribute_or_from_child(TXT("developmentVolumeVR"), developmentVolumeVR);
	developmentVolumeNonVR = _node->get_float_attribute_or_from_child(TXT("developmentVolumeNonVR"), developmentVolumeNonVR);
#endif
#ifdef AN_RECORD_VIDEO
	// always full
	developmentVolumeVR = 1.0f;
	developmentVolumeNonVR = 1.0f;
#endif

	if (IO::XML::Node const * node = _node->first_child_named(TXT("channels")))
	{
		channels.clear();
		for_every(child, node->all_children())
		{
			Name channelName = Name(child->get_name());
			if (!channels.get_existing(channelName))
			{
				Channel channel;
				channels[channelName] = channel;
			}
			channels[channelName].volume = child->get_float_attribute(TXT("volume"), channels[channelName].volume);
		}
	}

	for_every(node, _node->children_named(TXT("prefer")))
	{
		if (node->has_attribute(TXT("device")))
		{
			preferredDevices.push_back(node->get_string_attribute(TXT("device")));
		}
#ifdef AN_RECORD_VIDEO
		else if (node->has_attribute(TXT("deviceForRecording")))
		{
			preferredDevicesForRecording.push_back(node->get_string_attribute(TXT("deviceForRecording")));
		}
#endif
	}

	return result;
}

void SoundConfig::save_to_xml(IO::XML::Node * _node, bool _userConfig) const
{
	_node->set_bool_to_child(TXT("enableOcclusionLowPass"), enableOcclusionLowPass);
	if (maxConcurrentSounds != defaultMaxConcurrentSounds)
	{
		_node->set_int_to_child(TXT("maxConcurrentSounds"), maxConcurrentSounds);
	}
	_node->set_float_to_child(TXT("volume"), volume);
	if (IO::XML::Node* channelsNode = _node->add_node(TXT("channels")))
	{
		for_every(channel, channels)
		{
			if (IO::XML::Node* channelNode = channelsNode->add_node(for_everys_map_key(channel).to_string()))
			{
				channelNode->set_float_attribute(TXT("volume"), channel->volume);
			}
		}
	}
}

void SoundConfig::fill_missing()
{
	// add missing sound channels
	for_every(soundChannel, MainSettings::global().get_sound_channels())
	{
		if (!channels.get_existing(soundChannel->name))
		{
			Channel & ch = channels[soundChannel->name];
			ch.volume = 1.0f;
		}
	}

	// use volume from main config
	for_every(soundChannel, MainSettings::global().get_sound_channels())
	{
		if (auto* ch = channels.get_existing(soundChannel->name))
		{
			ch->overallVolume = soundChannel->volume;
		}
	}
}

bool SoundConfig::is_option_set(Name const & _option) const
{
	if (_option == TXT("occlusionlowpass")) return enableOcclusionLowPass;
	
	return false;
}
