#include "mc_musicAccidental.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

REGISTER_FOR_FAST_CAST(MusicAccidental);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new MusicAccidental(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new MusicAccidentalData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & MusicAccidental::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("musicAccidental")), create_module, create_module_data);
}

MusicAccidental::MusicAccidental(Framework::IModulesOwner* _owner)
	: base(_owner)
{
}

MusicAccidental::~MusicAccidental()
{
}

void MusicAccidental::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	musicAccidentalData = fast_cast<MusicAccidentalData>(_moduleData);
	if (musicAccidentalData)
	{
		music = musicAccidentalData->play;
		for_every(m, music)
		{
			m->cooldownIndividualTS.reset(-1000.0f);
		}
	}
}

//

REGISTER_FOR_FAST_CAST(MusicAccidentalData);

MusicAccidentalData::MusicAccidentalData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

MusicAccidentalData::~MusicAccidentalData()
{
}

bool MusicAccidentalData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("play"))
	{
		MusicAccidentalPlay map;
		if (map.load_from_xml(_node, _lc))
		{
			play.push_back(map);
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("can't load music accidental's play"));
			return false;
		}
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

//

bool MusicAccidentalPlay::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	sound = _node->get_name_attribute(TXT("sound"), sound);
	emissive = _node->get_name_attribute(TXT("emissive"), emissive);

	cooldownIndividual.load_from_xml(_node, TXT("cooldownIndividual"));
	cooldown.load_from_xml(_node, TXT("cooldown"));

	requiresVisualContact = _node->get_bool_attribute_or_from_child_presence(TXT("requiresVisualContact"), requiresVisualContact);

	maxDistance.load_from_xml(_node, TXT("maxDistance"));
	movingCloser.load_from_xml(_node, TXT("movingCloser"));

	return result;
}

void MusicAccidentalPlay::reset_cooldown_check_ts()
{
	if (!cooldownCheck.is_empty())
	{
		cooldownCheckTS.reset(Random::get(cooldownCheck));
	}
}

void MusicAccidentalPlay::reset_cooldown_individual_ts()
{
	if (!cooldownIndividual.is_empty())
	{
		cooldownIndividualTS.reset(Random::get(cooldownIndividual));
	}
}