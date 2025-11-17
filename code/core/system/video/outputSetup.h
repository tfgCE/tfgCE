#pragma once

#include "..\..\globalDefinitions.h"
#include "..\..\math\math.h"

#include "video.h"

#include "videoFormat.h"

#include "texture.h"

namespace System
{
	struct OutputTextureDefinition
	: protected TextureCommonSetup
	{
		typedef TextureCommonSetup base;
	public:
		OutputTextureDefinition(Name const & _name = Name::invalid())
		: videoFormat(VideoFormat::rgba)
		, baseVideoFormat(BaseVideoFormat::rgb)
		, videoFormatData(VideoFormatData::from(videoFormat))
		{
			withMipMaps = false;
			wrapU = TextureWrap::clamp;
			wrapV = TextureWrap::clamp;
			filteringMag = TextureFiltering::linear;
			filteringMin = TextureFiltering::linear; // consistent with below!
		}

		OutputTextureDefinition(Name const & _name,
								VideoFormat::Type _videoFormat,
								BaseVideoFormat::Type _baseVideoFormat,
								bool _withMipMaps = false,
								Optional<TextureFiltering::Type> _filteringMag = NP,
								Optional<TextureFiltering::Type> _filteringMin = NP)
		: name(_name)
		, videoFormat(_videoFormat)
		, baseVideoFormat(_baseVideoFormat)
		, videoFormatData(VideoFormatData::from(videoFormat))
		{
			withMipMaps = _withMipMaps;
			wrapU = TextureWrap::clamp;
			wrapV = TextureWrap::clamp;
			filteringMag = withMipMaps ? TextureFiltering::linearMipMapLinear : TextureFiltering::linear;
			filteringMin = withMipMaps ? TextureFiltering::linearMipMapLinear : TextureFiltering::linear;
			if (_filteringMag.is_set())
			{
				filteringMag = _filteringMag.get();
			}
			if (_filteringMin.is_set())
			{
				filteringMin = _filteringMin.get();
			}
		}

		void set_with_mip_maps(bool _withMipMaps)
		{
			withMipMaps = _withMipMaps;
			// alter filtering properly
			if (filteringMag == TextureFiltering::linearMipMapLinear ||
				filteringMag == TextureFiltering::linear)
			{
				filteringMag = withMipMaps ? TextureFiltering::linearMipMapLinear : TextureFiltering::linear;
			}
			if (filteringMin == TextureFiltering::linearMipMapLinear ||
				filteringMin == TextureFiltering::linear)
			{
				filteringMin = withMipMaps ? TextureFiltering::linearMipMapLinear : TextureFiltering::linear;
			}
		}

		bool does_match(OutputTextureDefinition const & _other, bool _ignoreMipMaps = false) const
		{
			return videoFormat == _other.videoFormat &&
				   baseVideoFormat == _other.baseVideoFormat &&
				   videoFormatData == _other.videoFormatData &&
				   (withMipMaps == _other.withMipMaps || _ignoreMipMaps) &&
				   wrapU == _other.wrapU &&
				   wrapV == _other.wrapV &&
				   TextureFiltering::does_match(filteringMag, _other.filteringMag) &&
				   TextureFiltering::does_match(filteringMin, _other.filteringMin);
		}

		Name const & get_name() const { return name; }
		VideoFormat::Type get_video_format() const { return videoFormat; }
		BaseVideoFormat::Type get_base_video_format() const { return baseVideoFormat; }
		VideoFormatData::Type get_video_format_data() const { return videoFormatData; }
		bool is_with_mip_maps() const { return withMipMaps; }
		TextureWrap::Type get_wrap_u() const { return wrapU; }
		TextureWrap::Type get_wrap_v() const { return wrapV; }
		TextureFiltering::Type get_filtering_mag() const { return filteringMag; }
		TextureFiltering::Type get_filtering_min() const { return filteringMin; }

		void set_format(VideoFormat::Type _vf, Optional<BaseVideoFormat::Type> _bvf = NP, Optional<VideoFormatData::Type> _vfd = NP);
		void set_filtering_mag(TextureFiltering::Type);
		void set_filtering_min(TextureFiltering::Type);

		bool load_from_xml(IO::XML::Node const * _node);

	protected:
		Name name;
		VideoFormat::Type videoFormat;
		BaseVideoFormat::Type baseVideoFormat;
		VideoFormatData::Type videoFormatData;
	};
	
	template <typename OutputTextureDefinitionClass = OutputTextureDefinition>
	struct OutputSetup
	{
	public:
		void clear_output_textures() { outputs.clear(); }
		void copy_output_textures_from(OutputSetup const & _source);

		void add_output_texture(OutputTextureDefinitionClass const & _outputTexture) { outputs.push_back(_outputTexture); }
		int get_output_texture_count() const { return outputs.get_size(); }
		OutputTextureDefinitionClass & access_output_texture_definition(int _index) { return outputs[_index]; }
		OutputTextureDefinitionClass const & get_output_texture_definition(int _index) const { return outputs[_index]; }

		bool load_from_xml(IO::XML::Node const * _node, OutputTextureDefinitionClass const & _defaultOutputTextureDefinition) { return load_from_xml(_node, TXT("output"), _defaultOutputTextureDefinition); }

	protected:
		Array<OutputTextureDefinitionClass> outputs;

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _outputNodeName, OutputTextureDefinitionClass const & _defaultOutputTextureDefinition);
	};

	#include "outputSetup.inl"

};
