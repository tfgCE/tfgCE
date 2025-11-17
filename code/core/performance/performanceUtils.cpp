#include "performanceUtils.h"
#include "..\system\video\video3d.h"

//

using namespace Performance;

//

bool enabled = true;

void ::Performance::finish_rendering()
{
	// slows down a lot and info about open gl performance is off (rendering 30k is as fast as rendering cube?)
	//glFinish();
}

BlockTag Performances::get_tag(Name const & _tag, BlockType::Type _type, Colour const & _colour)
{
	return BlockTag(_tag, _type, _colour.a != 0.0f ? _colour : Colour::random_rgb().mul_rgb(0.2f).add_to_rgb(0.4f));
}

BlockTag Performances::get_tag(tchar const * _tag, BlockType::Type _type, Colour const & _colour)
{
	return BlockTag(_tag, _type, _colour.a != 0.0f ? _colour : Colour::random_rgb().mul_rgb(0.2f).add_to_rgb(0.4f));
}

BlockTag Performances::get_tag(String const & _contextInfo, BlockType::Type _type, Colour const & _colour)
{
	return BlockTag(_contextInfo, _type, _colour.a != 0.0f ? _colour : Colour::black.with_alpha(0.5f));
}

bool Performance::is_enabled()
{
	return enabled;
}

void Performance::enable(bool _enable)
{
	enabled = _enable;
}
