#include "outputSetup.h"

#include "..\..\other\parserUtils.h"

//

using namespace ::System;

//

bool OutputTextureDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("name")))
	{
		name = attr->get_as_name();
	}
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no valid name provided for output texture definition"));
		result = false;
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("videoFormat")))
	{
		videoFormat = VideoFormat::parse(attr->get_as_string());
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("baseVideoFormat")))
	{
		baseVideoFormat = BaseVideoFormat::parse(attr->get_as_string());
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("videoFormatData")))
	{
		videoFormatData = VideoFormatData::parse(attr->get_as_string());
	}

	return result;
}

void OutputTextureDefinition::set_format(VideoFormat::Type _vf, Optional<BaseVideoFormat::Type> _bvf, Optional<VideoFormatData::Type> _vfd)
{
	videoFormat = _vf;
	if (!_bvf.is_set())
	{
		_bvf = VideoFormat::to_base_video_format(_vf);
	}
	baseVideoFormat = _bvf.get();
	if (!_vfd.is_set())
	{
		_vfd = VideoFormatData::from(_vf);
	}
	videoFormatData = _vfd.get();
}

void OutputTextureDefinition::set_filtering_mag(TextureFiltering::Type _filtering)
{
	filteringMag = _filtering;
	if (filteringMag == TextureFiltering::linearMipMapLinear ||
		filteringMag == TextureFiltering::linear)
	{
		filteringMag = withMipMaps ? TextureFiltering::linearMipMapLinear : TextureFiltering::linear;
	}
}

void OutputTextureDefinition::set_filtering_min(TextureFiltering::Type _filtering)
{
	filteringMin = _filtering;
	if (filteringMin == TextureFiltering::linearMipMapLinear ||
		filteringMin == TextureFiltering::linear)
	{
		filteringMin = withMipMaps ? TextureFiltering::linearMipMapLinear : TextureFiltering::linear;
	}
}
