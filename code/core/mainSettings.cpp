#include "mainSettings.h"
#include "io/xml.h"

#include "tags\tagCondition.h"
#include "system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

MainSettings default_settings()
{
	return MainSettings::defaults()
		.allow_importing() // block for final? but modding...
		.allow_tools() // block for final? but modding...
		.disallow_no_import_files_present() // put that into release mode?
		//.force_reimporting() // after something has changed - but before first public release
		.discard_data_after_building_buffer_objects() // this might be problematic when switching back to full screen - serialise to temp files? unserialise? keep in memory?
		//.render_fence_at_end_of_frame() // putting render fence at end of frame drops performance - as without that we can prepare data for rendering while waiting to display frame
		//
		//.rendering_pipeline_use_separate_buffers_for_position_and_normal()
		.rendering_pipeline_use_alpha_channel_for_depth()
		//.rendering_pipeline_use_colour_vertex_attributes()
		.default_texture_wrap_u(::System::TextureWrap::repeat)
		.default_texture_wrap_v(::System::TextureWrap::repeat)
		//
		.set_vertex_data_padding(16)
		//
		.add_sound_channel(Name(TXT("sound")))
		.add_sound_channel(Name(TXT("music")))
		.set_default_sound_channel(Name(TXT("sound")))
		//
		.set_mesh_gen_smoothing(true)
		;
}

MainSettings MainSettings::s_global = default_settings();

void MainSettings::initialise_static()
{
}

void MainSettings::reset_static()
{
	s_global = default_settings();
}

void MainSettings::close_static()
{
	// to clear arrays etc
	s_global = default_settings();
}

MainSettings::MainSettings()
: allowImporting(false)
, allowTools(false)
, disallowNoImportFilesPresent(false)
, forceReimporting(false)
, forceReimportingUsingTools(false)
, discardDataAfterBuildingBufferObjects(false)
, renderFenceAtEndOfFrame(false)
//
, renderingPipelineUsesSeparateBuffersForPositionAndNormal(false)
, renderingPipelineUsesAlphaChannelForDepth(false)
, renderingPipelineUsesColourVertexAttributes(false)
//
, defaultTextureWrapU(::System::TextureWrap::repeat)
, defaultTextureWrapV(::System::TextureWrap::repeat)
, defaultTextureFilteringMag(::System::TextureFiltering::nearest)
, defaultTextureFilteringMin(::System::TextureFiltering::linear)
//
, vertexDataPadding(16)
, vertexDataNormalPacked(false)
, vertexDataUVDataType(::System::DataFormatValueType::Float)
//
, meshGenSmoothing(true)
{
	add_sound_channel(Name(TXT("sound")));
	add_sound_channel(Name(TXT("music")));
}

#define LOAD_BOOL_SETTING(variable) variable = node->get_bool_from_child(TXT(#variable), variable);
#define LOAD_INT_SETTING(variable) variable = node->get_int_from_child(TXT(#variable), variable);

bool MainSettings::check_system_tag_required(IO::XML::Node const* _node, OPTIONAL_ OUT_ bool * _otherSystem) const
{
	assign_optional_out_param(_otherSystem, false);
	if (_node->has_attribute(TXT("systemTagRequired")))
	{
		TagCondition systemTagRequired;
		if (systemTagRequired.load_from_xml_attribute(_node, TXT("systemTagRequired")))
		{
			assign_optional_out_param(_otherSystem, true);
			if (!systemTagRequired.check(System::Core::get_system_tags()))
			{
				return false;
			}
		}
	}
	return true;
}

bool MainSettings::is_ok_to_save(tchar const* _what) const
{
	for_every(e, configSave.elements)
	{
		if (e->dontSave == _what)
		{
			if (e->changedExtraRequestedSystemTag.is_set())
			{
				if (::System::Core::has_extra_requested_system_tag_changed(e->changedExtraRequestedSystemTag.get()))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

bool MainSettings::load_from_xml(IO::XML::Node const * _node, MainSettingsLoadingOptions const & _loadingOptions)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("configSave")))
	{
		for_every(forNode, node->children_named(TXT("for")))
		{
			for_every(dsNode, forNode->children_named(TXT("dontSave")))
			{
				ConfigSave::Element e;
				e.dontSave = dsNode->get_text().trim();
				if (auto* attr = forNode->get_attribute(TXT("changedExtraRequestedSystemTag")))
				{
					e.changedExtraRequestedSystemTag = attr->get_as_name();
				}
				configSave.elements.push_back(e);
			}
		}
	}

	for_every(node, _node->children_named(TXT("settings")))
	{
		bool otherSystem = false;
		if (!check_system_tag_required(node, OPTIONAL_ OUT_ &otherSystem))
		{
			continue;
		}

		LOAD_BOOL_SETTING(allowImporting);
		LOAD_BOOL_SETTING(allowTools);
		LOAD_BOOL_SETTING(disallowNoImportFilesPresent);
		LOAD_BOOL_SETTING(forceReimporting);
		LOAD_BOOL_SETTING(forceReimportingUsingTools);
		LOAD_BOOL_SETTING(discardDataAfterBuildingBufferObjects);
		LOAD_BOOL_SETTING(renderFenceAtEndOfFrame);
		//
		LOAD_BOOL_SETTING(renderingPipelineUsesSeparateBuffersForPositionAndNormal);
		LOAD_BOOL_SETTING(renderingPipelineUsesAlphaChannelForDepth);
		LOAD_BOOL_SETTING(renderingPipelineUsesColourVertexAttributes);
		//
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("defaultTextureWrap")))
		{
			defaultTextureWrapU = ::System::TextureWrap::parse(subNode->get_text().trim());
			defaultTextureWrapV = defaultTextureWrapU;
		}
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("defaultTextureWrapU")))
		{
			defaultTextureWrapU = ::System::TextureWrap::parse(subNode->get_text().trim());
		}
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("defaultTextureWrapV")))
		{
			defaultTextureWrapV = ::System::TextureWrap::parse(subNode->get_text().trim());
		}
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("defaultTextureFiltering")))
		{
			defaultTextureFilteringMag = ::System::TextureFiltering::parse(subNode->get_text().trim());
			defaultTextureFilteringMin = defaultTextureFilteringMag;
		}
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("defaultTextureFilteringMag")))
		{
			defaultTextureFilteringMag = ::System::TextureFiltering::parse(subNode->get_text().trim());
		}
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("defaultTextureFilteringMin")))
		{
			defaultTextureFilteringMin = ::System::TextureFiltering::parse(subNode->get_text().trim());
		}
		//
		LOAD_INT_SETTING(vertexDataPadding);
		LOAD_BOOL_SETTING(vertexDataNormalPacked);
		if (IO::XML::Node const* subNode = node->first_child_named(TXT("vertexDataUVDataType")))
		{
			vertexDataUVDataType = ::System::DataFormatValueType::parse(subNode->get_text().trim());
		}
		//
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("jobSystem")))
		{
			if (!otherSystem || !_loadingOptions.ignoreJobSystemForOtherSystem)
			{
				jobSystemDefinition = JobSystemDefinition();
				jobSystemDefinition.load_from_xml(subNode);
			}
		}
		LOAD_BOOL_SETTING(doSoundOnGameThread);
		//
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("soundChannels")))
		{
			soundChannels.clear();
			for_every(child, subNode->all_children())
			{
				SoundChannelDefinition channel;
				channel.name = Name(child->get_name());
				channel.tags.load_from_xml_attribute_or_child_node(child);
				channel.reverb = child->get_bool_attribute_or_from_child_presence(TXT("reverb"), channel.reverb);
				channel.reverb = ! child->get_bool_attribute_or_from_child_presence(TXT("noReverb"), ! channel.reverb);
				channel.volume = child->get_float_attribute_or_from_child(TXT("volume"), channel.volume);
				for_every(n, child->children_named(TXT("compressor")))
				{
					channel.compressor.set_if_not_set(SoundCompressorConfig());
					auto& c = channel.compressor.access();
					c.load_from_xml(n);
				}
				soundChannels.push_back(channel);
			}
			if (!soundChannels.is_empty())
			{
				defaultSoundChannel = soundChannels.get_first().name;
			}
			else
			{
				defaultSoundChannel = Name::invalid();
			}
		}
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("defaultSoundChannel")))
		{
			defaultSoundChannel = Name(subNode->get_text());
		}
		if (IO::XML::Node const * subNode = node->first_child_named(TXT("audioDuckSourceChannel")))
		{
			audioDuckSourceChannel = Name(subNode->get_text());
		}
		//
		if (IO::XML::Node const* subNode = node->first_child_named(TXT("audioDuck")))
		{
			audioDuckLevel = subNode->get_float_attribute_or_from_child(TXT("level"), audioDuckLevel);
			audioDuckFadeTime = subNode->get_float_attribute_or_from_child(TXT("fadeTime"), audioDuckFadeTime);
		}
		//
		LOAD_BOOL_SETTING(meshGenSmoothing);
		//
		for_every(n, node->children_named(TXT("setShaderOptions")))
		{
			ShaderOptions so;
			so.id = n->get_name_attribute(TXT("id"));
			so.shaderOptions.load_from_xml(n);
			for_every(eso, allShaderOptions)
			{
				if (eso->id == so.id)
				{
					// remove/replace
					allShaderOptions.remove_at(for_everys_index(eso));
					break;
				}
			}
			allShaderOptions.push_back(so);
		}
		{
			DEFINE_STATIC_NAME(forceLowShaderSettings);
			if (::System::Core::get_system_tags().get_tag(NAME(forceLowShaderSettings)))
			{
				DEFINE_STATIC_NAME(low);
				useShaderOptionsId = NAME(low);
			}
		}
#ifdef FORCE_RENDERING_LOW
		{
			DEFINE_STATIC_NAME(low);
			useShaderOptionsId = NAME(low);
		}
#endif
#ifdef FORCE_RENDERING_ULTRA_LOW
		{
			DEFINE_STATIC_NAME_STR(ultraLow, TXT("ultra low"));
			useShaderOptionsId = NAME(ultraLow);
		}
#endif
		{
			Optional<int> selected;
			for_every(so, allShaderOptions)
			{
				if ((!selected.is_set() && !so->id.is_valid()) ||
					so->id == useShaderOptionsId)
				{
					selected = for_everys_index(so);
				}
			}
			if (selected.is_set())
			{
				shaderOptions = allShaderOptions[selected.get()].shaderOptions;
			}
		}
		shaderOptions.load_from_xml_attribute_or_child_node(node, TXT("shaderOptions"));
		forcedShaderOptions.load_from_xml_attribute_or_child_node(node, TXT("forcedShaderOptions"));
		shaderOptions.set_tags_from(forcedShaderOptions);
		//
		for_every(subNode, node->children_named(TXT("vertexTextureColourMapping")))
		{
			VertexTextureColourMapping mapping;
			mapping.shaderOptions.load_from_xml_attribute_or_child_node(subNode, TXT("shaderOption"));
			mapping.shaderOptions.load_from_xml_attribute_or_child_node(subNode, TXT("shaderOptions"));
			mapping.fragmentSamplerInput = subNode->get_name_attribute(TXT("fragmentSamplerInput"));
			mapping.customVertexData = subNode->get_name_attribute(TXT("customVertexData"));
			mapping.atV = subNode->get_float_attribute(TXT("atV"), mapping.atV);
			if (auto * attr = subNode->get_attribute(TXT("dataType")))
			{
				mapping.dataType = ::System::DataFormatValueType::parse(attr->get_as_string(), mapping.dataType);
			}
			error_loading_xml_on_assert(! mapping.shaderOptions.is_empty(), result, subNode, TXT("\"shaderOptions\" not provided"));
			error_loading_xml_on_assert(mapping.fragmentSamplerInput.is_valid(), result, subNode, TXT("\"fragmentSamplerInput\" not provided"));
			error_loading_xml_on_assert(mapping.customVertexData.is_valid(), result, subNode, TXT("\"customVertexData\" not provided"));
			if (! mapping.shaderOptions.is_empty() &&
				mapping.fragmentSamplerInput.is_valid() &&
				mapping.customVertexData.is_valid())
			{
				vertexTextureColourMappings.push_back(mapping);
			}
		}
	}

	return result;
}

#undef LOAD_BOOL_SETTING

MainSettings MainSettings::defaults()
{
	return MainSettings();
}

MainSettings& MainSettings::allow_importing(bool _value)
{
	allowImporting = _value;
	return *this;
}

MainSettings& MainSettings::allow_tools(bool _value)
{
	allowTools = _value;
	return *this;
}

MainSettings& MainSettings::disallow_no_import_files_present(bool _value)
{
	disallowNoImportFilesPresent = _value;
	return *this;
}

MainSettings& MainSettings::force_reimporting(bool _value)
{
	forceReimporting = _value;
	return *this;
}

MainSettings& MainSettings::force_reimporting_using_tools(bool _value)
{
	forceReimportingUsingTools = _value;
	return *this;
}

MainSettings& MainSettings::discard_data_after_building_buffer_objects(bool _value)
{
	discardDataAfterBuildingBufferObjects = _value;
	return *this;
}

MainSettings& MainSettings::render_fence_at_end_of_frame(bool _value)
{
	renderFenceAtEndOfFrame = _value;
	return *this;
}

MainSettings& MainSettings::rendering_pipeline_use_separate_buffers_for_position_and_normal(bool _value)
{
	renderingPipelineUsesSeparateBuffersForPositionAndNormal = _value;
	return *this;
}

MainSettings& MainSettings::rendering_pipeline_use_alpha_channel_for_depth(bool _value)
{
	renderingPipelineUsesAlphaChannelForDepth = _value;
	return *this;
}

MainSettings& MainSettings::rendering_pipeline_use_colour_vertex_attributes(bool _value)
{
	renderingPipelineUsesColourVertexAttributes = _value;
	return *this;
}

MainSettings& MainSettings::default_texture_wrap_u(::System::TextureWrap::Type _wrap)
{
	defaultTextureWrapU = _wrap;
	return *this;
}

MainSettings& MainSettings::default_texture_wrap_v(::System::TextureWrap::Type _wrap)
{
	defaultTextureWrapV = _wrap;
	return *this;
}

MainSettings& MainSettings::default_texture_filtering_mag(::System::TextureFiltering::Type _filtering)
{
	defaultTextureFilteringMag = _filtering;
	return *this;
}

MainSettings& MainSettings::default_texture_filtering_min(::System::TextureFiltering::Type _filtering)
{
	defaultTextureFilteringMin = _filtering;
	return *this;
}

MainSettings& MainSettings::set_vertex_data_padding(int _vertexDataPadding)
{
	vertexDataPadding = _vertexDataPadding;
	return *this;
}

MainSettings& MainSettings::set_vertex_data_normal_packed(bool _vertexDataNormalPacked)
{
	vertexDataNormalPacked = _vertexDataNormalPacked;
	return *this;
}

MainSettings& MainSettings::set_vertex_data_uv_data_type(::System::DataFormatValueType::Type _vertexDataUVDataType)
{
	vertexDataUVDataType = _vertexDataUVDataType;
	return *this;
}

MainSettings& MainSettings::add_sound_channel(Name const & _name, Tags const * _tags)
{
	for_every(ch, soundChannels)
	{
		if (ch->name == _name)
		{
			if (_tags)
			{
				ch->tags.set_tags_from(*_tags);
			}
			return *this;
		}
	}
	SoundChannelDefinition channel;
	channel.name = _name;
	if (_tags)
	{
		channel.tags = *_tags;
	}
	soundChannels.push_back(channel);
	return *this;
}

MainSettings& MainSettings::set_default_sound_channel(Name const & _name)
{
	defaultSoundChannel = _name;
	return *this;
}

MainSettings& MainSettings::set_mesh_gen_smoothing(bool _meshGenSmoothing)
{
	meshGenSmoothing = _meshGenSmoothing;
	return *this;
}

//

JobListDefinition::JobListDefinition(float _percentage, RangeInt _cores, bool _allowOnMainThread, bool _preferMainThread, bool _preferExtraSeparateThread)
: percentage(_percentage)
, cores(_cores)
, allowOnMainThread(_allowOnMainThread)
, preferMainThread(_preferMainThread)
, preferExtraSeparateThread(_preferExtraSeparateThread)
{}

bool JobListDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (_node)
	{
		percentage = _node->get_float_attribute(TXT("percentage"), percentage);
		preferMainThread = _node->get_bool_attribute(TXT("preferMainThread"), preferMainThread);
		preferExtraSeparateThread = _node->get_bool_attribute(TXT("preferExtraSeparateThread"), preferExtraSeparateThread);
		allowOnMainThread = _node->get_bool_attribute(TXT("allowOnMainThread"), allowOnMainThread);
		bundleWithJobList.clear();
		{
			String bundle = _node->get_string_attribute(TXT("bundleWithJobList"));
			List<String> bundles;
			bundle.split(String::comma(), bundles);
			for_every(a, bundles)
			{
				String aTrimmed = a->trim();
				if (!aTrimmed.is_empty())
				{
					bundleWithJobList.push_back(Name(aTrimmed));
				}
			}
		}
		avoidBeingWithJobList.clear();
		{
			String avoid = _node->get_string_attribute(TXT("avoidBeingWithJobList"));
			List<String> avoids;
			avoid.split(String::comma(), avoids);
			for_every(a, avoids)
			{
				String aTrimmed = a->trim();
				if (!aTrimmed.is_empty())
				{
					avoidBeingWithJobList.push_back(Name(aTrimmed));
				}
			}
		}
		result &= cores.load_from_xml(_node, TXT("cores"));
	}
	return result;
}

//

JobSystemDefinition::JobSystemDefinition()
: workerJobs(1.0f, RangeInt(1, 9999), true, false) // anywhere
, asyncWorkerJobs(1.0f, RangeInt(1, 9999), true, false) // anywhere
, loadingWorkerJobs(1.0f, RangeInt(1, 8), false, false) // anywhere, not on main thread - less is better as we add names a lot
, preparingWorkerJobs(1.0f, RangeInt(1, 9999), false, false) // anywhere, not on main thread
, prepareRenderJobs(1.0f, RangeInt(2, 9999), true, true) // at least two threads but can be anywhere
, gameJobs(1.0f, RangeInt(1, 9999), true, false) // anywhere
, backgroundJobs(0.25f, RangeInt(1, 9999), true, false) // no more than 25%
, gameBackgroundJobs(0.0f, RangeInt(1, 1), true, false, true) // no more than 25%
, soundJobs(1.0f, RangeInt(1, 9999), true, false) // anywhere
, renderJobs(0.0f, RangeInt(1, 1), true, true) // main thread only
, systemJobs(0.0f, RangeInt(1, 1), true, true) // main thread only
{
}

bool JobSystemDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	doSyncWorldJobsAsAsynchronousGameJobs = _node->get_bool_attribute_or_from_child_presence(TXT("doSyncWorldJobsAsAsynchronousGameJobs"), doSyncWorldJobsAsAsynchronousGameJobs);
	doSyncWorldJobsAsAsynchronousGameJobs = ! _node->get_bool_attribute_or_from_child_presence(TXT("doSyncWorldJobsOnCallingThread"), ! doSyncWorldJobsAsAsynchronousGameJobs);
	result &= workerJobs.load_from_xml(_node->first_child_named(TXT("worker")));
	result &= asyncWorkerJobs.load_from_xml(_node->first_child_named(TXT("asyncWorker")));
	result &= loadingWorkerJobs.load_from_xml(_node->first_child_named(TXT("loadingWorker")));
	result &= preparingWorkerJobs.load_from_xml(_node->first_child_named(TXT("preparingWorker")));
	result &= prepareRenderJobs.load_from_xml(_node->first_child_named(TXT("prepareRender")));
	result &= gameJobs.load_from_xml(_node->first_child_named(TXT("game")));
	result &= backgroundJobs.load_from_xml(_node->first_child_named(TXT("background")));
	result &= gameBackgroundJobs.load_from_xml(_node->first_child_named(TXT("gameBackground")));
	result &= renderJobs.load_from_xml(_node->first_child_named(TXT("render")));
	result &= soundJobs.load_from_xml(_node->first_child_named(TXT("sound")));
	result &= systemJobs.load_from_xml(_node->first_child_named(TXT("system")));
	return result;
}

