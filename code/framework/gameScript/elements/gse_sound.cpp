#include "gse_sound.h"

#include "..\..\game\game.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\moduleSound.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\sound\soundScene.h"
#include "..\..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Elements::Sound::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute_or_from_child(TXT("objectVar"), objectVar);
	sound = _node->get_name_attribute_or_from_child(TXT("sound"), sound);
	stop = _node->get_name_attribute_or_from_child(TXT("stop"), stop);
	fadeOutOnStop.load_from_xml(_node, TXT("fadeOut"));
	fadeOutOnStop.load_from_xml(_node, TXT("fadeOutOnStop"));

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	playAllSamples = _node->get_bool_attribute_or_from_child_presence(TXT("playAllSamples"), playAllSamples);
	sample.load_from_xml(_node, TXT("sample"), _lc);
	stopSample.load_from_xml(_node, TXT("stopSample"), _lc);
	playParams.load_from_xml(_node, _lc);

	poi.load_from_xml(_node, TXT("poi"));
	socket.load_from_xml(_node, TXT("socket"));
	socketVar.load_from_xml(_node, TXT("socketVar"));
	relativePlacement.load_from_xml_child_node(_node, TXT("relativePlacement"));
	playDetached = _node->get_bool_attribute_or_from_child_presence(TXT("playDetached"), playDetached || poi.is_set());

	return result;
}

bool Elements::Sound::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= sample.prepare_for_game(_library, _pfgContext);
	result &= stopSample.prepare_for_game(_library, _pfgContext);

	return result;
}

void Elements::Sound::load_on_demand_if_required()
{
	base::load_on_demand_if_required();
	for_every_ref(s, sample.samples)
	{
		s->load_on_demand_if_required();
	}
	for_every_ref(s, stopSample.samples)
	{
		s->load_on_demand_if_required();
	}
}

ScriptExecutionResult::Type Elements::Sound::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if ((sound.is_valid() || stop.is_valid() || ! sample.samples.is_empty() || ! stopSample.samples.is_empty()))
	{
		if (objectVar.is_valid())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
			{
				if (auto* imo = exPtr->get())
				{
					if (auto* s = imo->get_sound())
					{
						Transform useRelativePlacement = relativePlacement;
						if (stop.is_valid())
						{
							s->stop_sound(stop, fadeOutOnStop);
						}
						if (poi.is_set())
						{
							if (auto* r = imo->get_presence()->get_in_room())
							{
								Framework::PointOfInterestInstance* foundPOI = nullptr;
								if (r->find_any_point_of_interest(poi.get(), foundPOI))
								{
									useRelativePlacement = imo->get_presence()->get_placement().to_local(foundPOI->calculate_placement());
								}
								else
								{
									warn(TXT("couldn't find POI to play \"%S\""), poi.get().to_char());
								}
							}
						}
						Optional<Name> useSocket = socket;
						if (socketVar.is_set())
						{
							if (auto* v = _execution.get_variables().get_existing<Name>(socketVar.get()))
							{
								useSocket = *v;
							}
						}
						if (sound.is_valid())
						{
							if (auto* p = s->play_sound(sound, useSocket, useRelativePlacement, playParams))
							{
								if (playDetached)
								{
									p->detach_if_needed();
								}
							}
						}
						for_every_ref(sam, stopSample.samples)
						{
							s->stop_sound(sam, fadeOutOnStop);
						}
						if (playAllSamples)
						{
							for_every_ref(sam, sample.samples)
							{
								s->play_sound(sam, useSocket, useRelativePlacement, playParams, playDetached);
							}
						}
						else if (!sample.samples.is_empty())
						{
							int idx = Random::get_int(sample.samples.get_size());
							if (auto* sam = sample.samples[idx].get())
							{
								s->play_sound(sam, useSocket, useRelativePlacement, playParams, playDetached);
							}
						}
					}
					else
					{
						error_dev_investigate(TXT("\"%S\" has no sound module"), imo->ai_get_name().to_char());
					}
				}
			}
		}
	}
	
	return ScriptExecutionResult::Continue;
}

//

bool Elements::Sound::MutlipleSamples::load_from_xml(IO::XML::Node const* _node, tchar const* _what, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	if (auto* a = _node->get_attribute(_what))
	{
		Framework::UsedLibraryStored<Framework::Sample> s;
		if (s.load_from_xml(_node, _what, _lc))
		{
			samples.push_back(s);
		}
		else
		{
			result = false;
		}
	}
	for_every(n, _node->children_named(_what))
	{
		Framework::UsedLibraryStored<Framework::Sample> s;
		if (s.load_from_xml(n, TXT("sample"), _lc))
		{
			samples.push_back(s);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool Elements::Sound::MutlipleSamples::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(s, samples)
	{
		result &= s->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}
