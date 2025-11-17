#include "moduleSoundData.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

//

bool ModuleSoundSample::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"));
	if (! id.is_valid())
	{
		error_loading_xml(_node, TXT("no id provided for sample in module sound data"));
		result = false;
	}

	as = _node->get_name_attribute(TXT("as"));
	paramsAs = _node->get_name_attribute(TXT("paramsAs"));

	mayNotExist = _node->get_bool_attribute_or_from_child_presence(TXT("mayNotExist"));

	onStartPlay = _node->get_name_attribute(TXT("onStartPlay"), onStartPlay);
	onStopPlay = _node->get_name_attribute(TXT("onStopPlay"), onStopPlay);

	socket = _node->get_name_attribute(TXT("socket"), socket);
	placement.load_from_xml_child_node(_node, TXT("placement"));

	playDetached = _node->get_bool_attribute_or_from_child_presence(TXT("playDetached"), playDetached);
	playDetached = _node->get_bool_attribute_or_from_child_presence(TXT("playInRoom"), playDetached);
	playDetached = _node->get_bool_attribute_or_from_child_presence(TXT("detached"), playDetached);

	result &= playParams.load_from_xml(_node, _lc);

	autoPlay = _node->get_bool_attribute_or_from_child_presence(TXT("autoPlay"), autoPlay);

	cooldown.load_from_xml(_node, TXT("cooldown"));
	chance.load_from_xml(_node, TXT("chance"));

	tags.load_from_xml_attribute_or_child_node(_node);

	if (auto* attr = _node->get_attribute(TXT("sample")))
	{
		Framework::UsedLibraryStored<Framework::Sample> sample;
		if (sample.load_from_xml(attr, _lc))
		{
			samples.push_back(sample);
		}
		else
		{
			error_loading_xml(_node, TXT("problem loading sample from attribute"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("sample")))
	{
		Framework::UsedLibraryStored<Framework::Sample> sample;
		if (sample.load_from_xml(node, TXT("sample"), _lc))
		{
			samples.push_back(sample);
		}
		else
		{
			error_loading_xml(_node, TXT("problem loading sample from attribute"));
			result = false;
		}
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("silent")) ||
		_node->get_bool_attribute_or_from_child_presence(TXT("silence")) ||
		_node->get_name() == TXT("silentSample"))
	{
		samples.clear();
	}
	else
	{
		if (samples.is_empty() && ! as.is_valid())
		{
			error_loading_xml(_node, TXT("no samples provided and no \"as\" defined"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("aiMessage")))
	{
		AI::MessageTemplate aiMessage;
		if (aiMessage.load_from_xml(node))
		{
			aiMessages.push_back(aiMessage);
		}
		else
		{
			error_loading_xml(node, TXT("could not load ai message"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("pitchMap")))
	{
		remapPitchContinuous = node->get_bool_attribute_or_from_child_presence(TXT("continuous"), remapPitchContinuous);
		bool atStart = node->get_bool_attribute_or_from_child_presence(TXT("justOnce"), false);
		warn_loading_xml_on_assert(remapPitchContinuous || atStart, node, TXT("specify if map is \"continuous\" or \"justOnce\""));
		remapPitchUsingVar = node->get_name_attribute(TXT("usingVar"), remapPitchUsingVar);
		remapPitch.clear();
		remapPitch.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("volumeMap")))
	{
		remapVolumeContinuous = node->get_bool_attribute_or_from_child_presence(TXT("continuous"), remapVolumeContinuous);
		bool atStart = node->get_bool_attribute_or_from_child_presence(TXT("justOnce"), false);
		warn_loading_xml_on_assert(remapVolumeContinuous || atStart, node, TXT("specify if map is \"continuous\" or \"justOnce\""));
		remapVolumeUsingVar = node->get_name_attribute(TXT("usingVar"), remapVolumeUsingVar);
		remapVolume.clear();
		remapVolume.load_from_xml(node);
	}

	return result;
}

bool ModuleSoundSample::prepare_for_game(Map<Name, ModuleSoundSample> const & _samples, Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(sample, samples)
	{
		if (mayNotExist)
		{
			sample->prepare_for_game_find_may_fail(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
		else
		{
			result &= sample->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
	}

	if (paramsAs.is_valid())
	{
		IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::InitialPrepare)
		{
			if (auto * other = _samples.get_existing(paramsAs))
			{
				playParams.fill_with(other->playParams);
			}
			else
			{
				error(TXT("could not find sample \"%S\" for \"paramsAs\""), as.to_char());
				result = false;
			}
		}
	}

	if (as.is_valid())
	{
		IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::InitialPrepare + 1)
		{
			if (auto * other = _samples.get_existing(as))
			{
				samples = other->samples;
				playParams.fill_with(other->playParams);
				if (!socket.is_valid()) socket = other->socket;
				if (!onStartPlay.is_valid()) onStartPlay = other->onStartPlay;
				if (!onStopPlay.is_valid()) onStopPlay = other->onStopPlay;
			}
			else
			{
				error(TXT("could not find sample \"%S\" for \"as\""), as.to_char());
				result = false;
			}
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(ModuleSoundData);

ModuleSoundData::ModuleSoundData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleSoundData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("sample") ||
		_node->get_name() == TXT("silentSample"))
	{
		Name id = _node->get_name_attribute(TXT("id"));
		if (id.is_valid())
		{
			return samples[id].load_from_xml(_node, _lc);
		}
		else
		{
			error_loading_xml(_node, TXT("no id provided for sample in module sound data"));
			return false;
		}
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool ModuleSoundData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool ModuleSoundData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(sample, samples)
	{
		result &= sample->prepare_for_game(samples, _library, _pfgContext);
	}

	return result;
}
