#include "soundSample.h"

#include "..\library\library.h"
#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

#include "..\..\core\serialisation\importerHelper.h"
#include "..\..\core\sound\playback.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define INSPECT_DYNAMIC_LOADING_UNLOADING

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Framework::Sample);
LIBRARY_STORED_DEFINE_TYPE(Framework::Sample, sample);

Sample::Sample(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
#ifdef AN_PERFORMANCE_MEASURE
	performanceContextInfo = _name.to_string();
#endif
}

Sample::~Sample()
{
	for_every_ptr(sample, samples)
	{
		if (sample)
		{
			delete sample;
		}
	}
}

struct ::Framework::SampleImporter
: public RefCountObject
, public ImporterHelper<::Sound::Sample, ::Sound::SampleParams>
{
	typedef ImporterHelper<::Sound::Sample, ::Sound::SampleParams> base;

	SampleImporter(Framework::Sample* _sample, int _index)
	: base(_sample->samples[_index])
	{}

protected:
	override_ void import_object()
	{
		object = ::Sound::Sample::importer().import(importFromFileName, importOptions);
	}

private:
};

bool Framework::Sample::load_multiple_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	String filePath = _node->get_string_attribute_or_from_child(TXT("filePath"));
	String fileExt = _node->get_string_attribute_or_from_child(TXT("fileExt"));
	String importFromPath = _node->get_string_attribute_or_from_child(TXT("importFromPath"));
	String importFromExt = _node->get_string_attribute_or_from_child(TXT("importFromExt"));

	Optional<int> loadLaterPrio;
	bool readLoadOnDemand = false;
	bool readAllowUnloading = false;
	if (Library::does_allow_loading_on_demand())
	{
		result &= LibraryStored::load_later_on_demand_loading(_node, OUT_ loadLaterPrio, OUT_ readLoadOnDemand, OUT_ readAllowUnloading);
	}

	::Sound::SampleParams commonSampleParams;
	IO::XML::Node const* customSampleParamsNode = nullptr;
	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("loadLater"))
		{
			if (Library::does_allow_loading_on_demand())
			{
				loadLaterPrio.clear();
				readLoadOnDemand = false;
				readAllowUnloading = false;

				loadLaterPrio = node->get_int_attribute(TXT("priority"), 0);
			}
		}
		else if (node->get_name() == TXT("loadOnDemand"))
		{
			if (Library::does_allow_loading_on_demand())
			{
				loadLaterPrio.clear();
				readLoadOnDemand = node->get_bool(true);
				readAllowUnloading = false;
			}
		}
		else if (node->get_name() == TXT("allowUnloading"))
		{
			if (Library::does_allow_loading_on_demand())
			{
				loadLaterPrio.clear();
				readLoadOnDemand = true; // force
				readAllowUnloading = node->get_bool(true);
			}
		}
		else if (node->get_name() == TXT("commonSampleParams"))
		{
			commonSampleParams = ::Sound::SampleParams();
			result &= commonSampleParams.load_from_xml(node);
			customSampleParamsNode = node;
		}
		else if (node->get_name() == TXT("sample"))
		{
			String name = node->get_string_attribute_or_from_child(TXT("name"));
			if (!name.is_empty())
			{
				Framework::LibraryName lname(_lc.get_current_group(), Name(name));
				Framework::Sample* sample = _lc.get_library()->get_samples().find_or_create(lname);
				result &= sample->Framework::LibraryStored::load_from_xml(node, _lc);
				_lc.push_id(sample->get_name().to_string());
				_lc.push_owner(sample);
				if (customSampleParamsNode)
				{
					result &= sample->load_params_from_xml(customSampleParamsNode, _lc);
				}
				result &= sample->load_params_from_xml(node, _lc);
				while (sample->samples.get_size() > 1)
				{
					delete sample->samples.get_last();
				}
				while (sample->samples.get_size() < 1)
				{
					sample->samples.push_back(new ::Sound::Sample());
				}
				RefCountObjectPtr<SampleImporter> sampleImporter;
				sampleImporter = new SampleImporter(sample, 0);
				sampleImporter->set_import_options(commonSampleParams);
				result &= sampleImporter->setup_from_xml_use_provided(node, _lc.get_library_source() == LibrarySource::Assets, name, filePath, fileExt, importFromPath, importFromExt);
				
				if (loadLaterPrio.is_set())
				{
					sample->set_requires_later_loading(loadLaterPrio);
					sample->set_requires_loading_on_demand(false, false);
					sample->onMultipleLoadOutsideQueueSampleImporter = sampleImporter;
				}
				else if (readAllowUnloading)
				{
					sample->set_requires_later_loading(NP);
					sample->set_requires_loading_on_demand(true, true);
					sample->onMultipleLoadOutsideQueueSampleImporter = sampleImporter;
				}
				else if (readLoadOnDemand)
				{
					sample->set_requires_later_loading(NP);
					sample->set_requires_loading_on_demand(true, readAllowUnloading);
					sample->onMultipleLoadOutsideQueueSampleImporter = sampleImporter;
				}
				else
				{
					if (!sampleImporter->import())
					{
#ifdef AN_OUTPUT_IMPORT_ERRORS
						error_loading_xml(_node, TXT("can't import"));
#endif
						result = false;
					}
				}
				_lc.pop_owner();
				_lc.pop_id();
			}
			else
			{
				error_loading_xml(node, TXT("no \"name\" provided"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("not recognised element of loadMultipleSamples"));
			result = false;
		}
	}

	return result;
}

bool Framework::Sample::load_params_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;
	hearableByOwnerOnly = _node->get_bool_attribute_or_from_child_presence(TXT("hearableByOwnerOnly"), hearableByOwnerOnly);
	hearingDistance = _node->get_float_attribute_or_from_child(TXT("hearingDistance"), hearingDistance);
	if (_node->has_attribute(TXT("distanceBeyondFirstRoom")))
	{
		warn_loading_xml(_node, TXT("\"distanceBeyondFirstRoom\" is deprecated, use \"hearingDistanceBeyondFirstRoom\""));
	}
	hearingDistanceBeyondFirstRoom.load_from_xml(_node, TXT("hearingDistanceBeyondFirstRoom"));
	hearingRoomDistanceRecentlySeenByPlayer.load_from_xml(_node, TXT("hearingRoomDistanceRecentlySeenByPlayer"));
	maxConcurrentApparentSoundSources = _node->get_int_attribute_or_from_child(TXT("maxConcurrentApparentSoundSources"), maxConcurrentApparentSoundSources);
	fadeInTime = _node->get_float_attribute_or_from_child(TXT("fadeTime"), fadeInTime);
	fadeOutTime = _node->get_float_attribute_or_from_child(TXT("fadeTime"), fadeOutTime);
	fadeInTime = _node->get_float_attribute_or_from_child(TXT("fadeIn"), fadeInTime);
	fadeInTime = _node->get_float_attribute_or_from_child(TXT("fadeInTime"), fadeInTime);
	fadeOutTime = _node->get_float_attribute_or_from_child(TXT("fadeOut"), fadeOutTime);
	fadeOutTime = _node->get_float_attribute_or_from_child(TXT("fadeOutTime"), fadeOutTime);
	earlyFadeOutTime = _node->get_float_attribute_or_from_child(TXT("earlyFadeOut"), earlyFadeOutTime);
	earlyFadeOutTime = _node->get_float_attribute_or_from_child(TXT("earlyFadeOutTime"), earlyFadeOutTime);
	startAtPt.load_from_xml(_node, TXT("startAtPt"));
	if (_node->has_attribute_or_child(TXT("text"))) // text has to be explicitly defined as a child node or attribute
	{
		text.load_from_xml_child(_node, TXT("text"), _lc, TXT("text"));
	}
			
	return result;
}

String const& Framework::Sample::get_text(Optional<float> const & _at) const
{
	if (_at.is_set())
	{
		return textCache.get(text.get(), _at);
	}
	else
	{
		return text.get();
	}
}

void Framework::Sample::load_outside_queue(tchar const * _reason)
{
#ifdef INSPECT_DYNAMIC_LOADING_UNLOADING
	output(TXT("[sample] load %S"), get_name().to_string().to_char());
#endif

	// load resource first
	if (onMultipleLoadOutsideQueueSampleImporter.is_set())
	{
		// just import, we're loaded fine
		SampleImporter* sampleImporter = onMultipleLoadOutsideQueueSampleImporter.get();
		sampleImporter->import();
		if (!does_allow_unloading())
		{
			onMultipleLoadOutsideQueueSampleImporter.clear();
		}
	}

	base::load_outside_queue(_reason);

#ifdef INSPECT_DYNAMIC_LOADING_UNLOADING
	output(TXT("[sample] loaded %S"), get_name().to_string().to_char());
#endif
}

void Framework::Sample::unload_outside_queue(tchar const * _reason)
{
#ifdef INSPECT_DYNAMIC_LOADING_UNLOADING
	output(TXT("[sample] unload %S"), get_name().to_string().to_char());
#endif

	// load resource first
	if (onMultipleLoadOutsideQueueSampleImporter.is_set())
	{
		// just unload data for samples
		SampleImporter* sampleImporter = onMultipleLoadOutsideQueueSampleImporter.get();
		sampleImporter->unload();
	}

	// and free memory for all other samples
	for_count(int, i, samples.get_size())
	{
		delete_and_clear(samples[i]);
	}

	base::unload_outside_queue(_reason);

#ifdef INSPECT_DYNAMIC_LOADING_UNLOADING
	output(TXT("[sample] unloaded %S"), get_name().to_string().to_char());
#endif
}

bool Framework::Sample::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	result &= load_params_from_xml(_node, _lc);

	int sampleIdx = 0;
	{
		// we need to have at least place for sample importer
		while (samples.get_size() <= sampleIdx)
		{
			samples.push_back(nullptr);
		}
		SampleImporter sampleImporter(this, sampleIdx);
		if (sampleImporter.has_file_attribute(_node))
		{
			if (samples[sampleIdx] == nullptr)
			{
				samples[sampleIdx] = new ::Sound::Sample();
			}
			result &= sampleImporter.setup_from_xml(_node, _lc.get_library_source() == LibrarySource::Assets);
			if (!sampleImporter.import())
			{
#ifdef AN_OUTPUT_IMPORT_ERRORS
				error_loading_xml(_node, TXT("can't import"));
#endif
				result = false;
			}
			++sampleIdx;
		}
	}
	for_every(node, _node->children_named(TXT("sample")))
	{
		// we need to have at least place for sample importer
		while (samples.get_size() <= sampleIdx)
		{
			samples.push_back(nullptr);
		}
		SampleImporter sampleImporter(this, sampleIdx);
		if (sampleImporter.has_file_attribute(_node))
		{
			if (samples[sampleIdx] == nullptr)
			{
				samples[sampleIdx] = new ::Sound::Sample();
			}

			result &= sampleImporter.setup_from_xml(_node, _lc.get_library_source() == LibrarySource::Assets);
			result &= sampleImporter.setup_from_xml(node, _lc.get_library_source() == LibrarySource::Assets); // allow overriding per node (file names will be loaded always from node, not _node)
			if (!sampleImporter.import())
			{
#ifdef AN_OUTPUT_IMPORT_ERRORS
				error_loading_xml(_node, TXT("can't import"));
#endif
				result = false;
			}
			++sampleIdx;
		}
	}

	// remove samples that were not loaded (for any reason)
	for (int idx = 0; idx < samples.get_size(); ++idx)
	{
		if (samples[idx] == nullptr)
		{
			samples.remove_at(idx);
			--idx;
		}
	}
	
#ifdef AN_DEVELOPMENT_OR_PROFILER
	{
		bool isLooped = false;
		for_every_ptr(sample, samples)
		{
			isLooped |= sample->is_looped();
		}
		if (! isLooped)
		{
			if (fadeInTime == 0.0f &&
				!_node->get_attribute(TXT("fadeIn")) &&
				!_node->get_attribute(TXT("fadeInTime")) &&
				!_node->get_attribute(TXT("fadeTime")))
			{
				warn_loading_xml(_node, TXT("not looped and fade in time is 0, not set explicitly"));
				fadeInTime = 0.001f;
			}
			if (fadeOutTime == 0.0f &&
				!_node->get_attribute(TXT("fadeOut")) &&
				!_node->get_attribute(TXT("fadeOutTime")) &&
				!_node->get_attribute(TXT("fadeTime")))
			{
				warn_loading_xml(_node, TXT("not looped and fade out time is 0, not set explicitly"));
				fadeOutTime = 0.001f;
			}
		}
	}
#endif

	return result;
}

bool Framework::Sample::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	result &= LibraryStored::prepare_for_game(_library, _pfgContext);
	return result;
}

float Framework::Sample::get_start_at_pt(Random::Generator* _useRandomGenerator) const
{
	return startAtPt.max <= startAtPt.min ? startAtPt.min : (_useRandomGenerator ? _useRandomGenerator->get_float(startAtPt.min, startAtPt.max) : Random::get_float(startAtPt.min, startAtPt.max));
}

::Sound::Playback Framework::Sample::play(Optional<float> const& _fadeIn, Optional<::Sound::PlaybackSetup> const& _playbackSetup, Optional<int> const& _sampleIdx, bool _keepPaused) const
{
	if (!samples.is_empty())
	{
		int sampleIdx = Random::get_int(samples.get_size());
		if (auto* sample = get_sample(sampleIdx))
		{
			::Sound::PlaybackSetup playbackSetup = _playbackSetup.get(::Sound::PlaybackSetup());
			::Sound::Playback playback = sample->set_to_play_remain_paused(playbackSetup.useOwnFading);

#ifdef INVESTIGATE_SOUND_SYSTEM
			playback.set_dev_info(get_name().to_string());
#endif

			playback.set_at(sample->get_length() * get_start_at_pt());
			playback.fade_in_on_start(_fadeIn.get(get_fade_in_time()));

			if (!_keepPaused)
			{
				playback.resume();
			}

			return playback;
		}
	}

	return ::Sound::Playback();
}
