#include "soundCompressorConfig.h"

#include "..\..\io\xml.h"

//

bool SoundCompressorConfig::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	threshold = _node->get_float_attribute_or_from_child(TXT("threshold"), threshold);
	ratio = _node->get_float_attribute_or_from_child(TXT("ratio"), ratio);
	attack = _node->get_float_attribute_or_from_child(TXT("attack"), attack);
	release = _node->get_float_attribute_or_from_child(TXT("release"), release);
	threshold = clamp(threshold, -60.0f, 0.0f);
	ratio = clamp(ratio, 1.0f, 50.0f);
	attack = clamp(attack, 0.0001f, 0.5f);
	release = clamp(release, 0.01f, 5.0f);

	return result;
}