#pragma once

#include "math\math.h"
#include "globalSettings.h"
#include "tags\tag.h"
#include "system\sound\soundCompressorConfig.h"
#include "system\video\videoEnums.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

struct JobListDefinition
{
	float percentage;
	RangeInt cores;
	bool allowOnMainThread;
	bool preferMainThread;
	bool preferExtraSeparateThread;
	ArrayStatic<Name, 8> bundleWithJobList;
	ArrayStatic<Name, 8> avoidBeingWithJobList;
	JobListDefinition(float _percentage, RangeInt _cores = RangeInt(0, 999999), bool _allowOnMainThread = false, bool _preferMainThread = false, bool _preferExtraSeparateThread = false);
	bool load_from_xml(IO::XML::Node const * _node);
};

struct JobSystemDefinition
{
	bool doSyncWorldJobsAsAsynchronousGameJobs = true;

	JobListDefinition workerJobs;
	JobListDefinition asyncWorkerJobs;
	JobListDefinition loadingWorkerJobs;
	JobListDefinition preparingWorkerJobs;
	JobListDefinition prepareRenderJobs;
	JobListDefinition gameJobs;
	JobListDefinition backgroundJobs;
	JobListDefinition gameBackgroundJobs;
	JobListDefinition soundJobs;
	JobListDefinition renderJobs; // this should be left as it is - they should run on main thread!
	JobListDefinition systemJobs; // this should be left as it is - they should run on main thread!

	JobSystemDefinition();
	bool load_from_xml(IO::XML::Node const * _node);
};

struct SoundChannelDefinition
{
	Name name;
	Tags tags;
	bool reverb = true;
	float volume = 1.0f;
	Optional<SoundCompressorConfig> compressor;
};

struct VertexTextureColourMapping
{
	Tags shaderOptions; // from MainSettings, all have to be present
	Name fragmentSamplerInput; // texture in fragment sampler to base on
	Name customVertexData; // to fill with texture colour
	float atV = 1.0f;
	::System::DataFormatValueType::Type dataType = ::System::DataFormatValueType::Float;
	// attrib type is always Float
};

struct MainSettingsLoadingOptions
{
	bool ignoreJobSystemForOtherSystem = false;
};

/**
 *	Main Settings
 *
 *	General engine/game settings. Should be not modified by user/player.
 *	Importing, tools, rendering/post process pipelines.
 *
 */
struct MainSettings
{
	static MainSettings const & global() { return s_global; }
	static MainSettings & access_global() { return s_global; }

	static void initialise_static();
	static void reset_static();
	static void close_static();

	MainSettings();

	bool load_from_xml(IO::XML::Node const * _node, MainSettingsLoadingOptions const & _loadOptions = MainSettingsLoadingOptions());

	bool should_allow_importing() const { return allowImporting; }
	bool should_allow_tools() const { return allowTools; }
	bool should_disallow_no_import_files_present() const { return disallowNoImportFilesPresent; }
	bool should_force_reimporting() const { return forceReimporting; }
	bool should_force_reimporting_using_tools() const { return forceReimportingUsingTools; }
	bool should_discard_data_after_building_buffer_objects() const { return discardDataAfterBuildingBufferObjects; }
	bool should_render_fence_be_at_end_of_frame() const { return renderFenceAtEndOfFrame; }
	//
	bool rendering_pipeline_should_use_separate_buffers_for_position_and_normal() const { return renderingPipelineUsesSeparateBuffersForPositionAndNormal; }
	bool rendering_pipeline_should_use_alpha_channel_for_depth() const { return renderingPipelineUsesAlphaChannelForDepth; }
	bool rendering_pipeline_should_use_colour_vertex_attributes() const { return renderingPipelineUsesColourVertexAttributes; }
	//
	::System::TextureWrap::Type get_default_texture_wrap_u() const { return defaultTextureWrapU; }
	::System::TextureWrap::Type get_default_texture_wrap_v() const { return defaultTextureWrapV; }
	::System::TextureFiltering::Type get_default_texture_filtering_mag() const { return defaultTextureFilteringMag; }
	::System::TextureFiltering::Type get_default_texture_filtering_min() const { return defaultTextureFilteringMin; }
	//
	int get_vertex_data_padding() const { return vertexDataPadding; }
	bool is_vertex_data_normal_packed() const { return vertexDataNormalPacked; }
	::System::DataFormatValueType::Type get_vertex_data_uv_data_type() const { return vertexDataUVDataType; }
	//
	JobSystemDefinition const & get_job_system_definition() const { return jobSystemDefinition; }
	bool should_do_sound_on_game_thread() const { return doSoundOnGameThread; }
	//
	Array<SoundChannelDefinition> const & get_sound_channels() const { return soundChannels; }
	Name const & get_default_sound_channel() const { return defaultSoundChannel.is_valid() ? defaultSoundChannel : (!soundChannels.is_empty() ? soundChannels.get_first().name : Name::invalid());  }
	Name const & get_audio_duck_source_channel() const { return audioDuckSourceChannel;  }
	//
	float get_audio_duck_level() const { return audioDuckLevel; }
	float get_audio_duck_fade_time() const { return audioDuckFadeTime; }
	//
	bool get_mesh_gen_smoothing() const { return meshGenSmoothing; }
	//
	Tags & access_shader_options() { return shaderOptions; }
	Tags const& get_shader_options() const { return shaderOptions; }
	Array<VertexTextureColourMapping> const& get_vertex_texture_colour_mapping() const { return vertexTextureColourMappings; }

	static MainSettings defaults();
	MainSettings& allow_importing(bool _value = true);
	MainSettings& allow_tools(bool _value = true);
	MainSettings& disallow_no_import_files_present(bool _value = true);
	MainSettings& force_reimporting(bool _value = true);
	MainSettings& force_reimporting_using_tools(bool _value = true);
	MainSettings& discard_data_after_building_buffer_objects(bool _value = true);
	MainSettings& render_fence_at_end_of_frame(bool _value = true);
	//
	MainSettings& rendering_pipeline_use_separate_buffers_for_position_and_normal(bool _value = true);
	MainSettings& rendering_pipeline_use_alpha_channel_for_depth(bool _value = true);
	MainSettings& rendering_pipeline_use_colour_vertex_attributes(bool _value = true);
	//
	MainSettings& default_texture_wrap_u(::System::TextureWrap::Type _wrap);
	MainSettings& default_texture_wrap_v(::System::TextureWrap::Type _wrap);
	MainSettings& default_texture_filtering_min(::System::TextureFiltering::Type _filtering);
	MainSettings& default_texture_filtering_mag(::System::TextureFiltering::Type _filtering);
	//
	MainSettings& set_vertex_data_padding(int _vertexDataPadding);
	MainSettings& set_vertex_data_normal_packed(bool _vertexDataNormalPacked);
	MainSettings& set_vertex_data_uv_data_type(::System::DataFormatValueType::Type _vertexDataUVDataType);
	//
	MainSettings& add_sound_channel(Name const & _name, Tags const * _tags = nullptr);
	MainSettings& set_default_sound_channel(Name const & _name);
	//
	MainSettings& set_mesh_gen_smoothing(bool _meshGenSmoothing);

	bool is_ok_to_save(tchar const* _what) const;

private:
	static MainSettings s_global;
	bool allowImporting;
	bool allowTools;
	bool disallowNoImportFilesPresent; // disallow situation where there are some missing import files
	bool forceReimporting; // to force reimporting - before first version is released, any modification should not create new version
	bool forceReimportingUsingTools;
	bool discardDataAfterBuildingBufferObjects;
	bool renderFenceAtEndOfFrame;
	//
	bool renderingPipelineUsesSeparateBuffersForPositionAndNormal;
	bool renderingPipelineUsesAlphaChannelForDepth;
	bool renderingPipelineUsesColourVertexAttributes;
	//
	::System::TextureWrap::Type defaultTextureWrapU;
	::System::TextureWrap::Type defaultTextureWrapV;
	::System::TextureFiltering::Type defaultTextureFilteringMag;
	::System::TextureFiltering::Type defaultTextureFilteringMin;
	//
	int vertexDataPadding;
	bool vertexDataNormalPacked;
	::System::DataFormatValueType::Type vertexDataUVDataType;
	//
	JobSystemDefinition jobSystemDefinition;
	bool doSoundOnGameThread = false; // on game thread!
	//
	Array<SoundChannelDefinition> soundChannels;
	Name audioDuckSourceChannel; // if its volume is set to 0, audioDuckLevel is at 1 (full audio)
	Name defaultSoundChannel;
	float audioDuckLevel = 0.6f;
	float audioDuckFadeTime = 0.5f;
	//
	bool meshGenSmoothing;
	//
	Name useShaderOptionsId;
	Tags shaderOptions;
	Tags forcedShaderOptions; // additional, always used
	struct ShaderOptions
	{
		Name id;
		Tags shaderOptions;
	};
	Array<ShaderOptions> allShaderOptions;
	Array<VertexTextureColourMapping> vertexTextureColourMappings;

	struct ConfigSave
	{
		struct Element
		{
			Optional<Name> changedExtraRequestedSystemTag;
			String dontSave;
		};
		Array<Element> elements;
	} configSave;

	bool check_system_tag_required(IO::XML::Node const* _node, OPTIONAL_ OUT_ bool* _otherSystem = nullptr) const; // returns true if should proceed, can be used by other configs too
};
