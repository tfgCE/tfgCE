#include "indexFormat.h"

#include "..\..\serialisation\serialiser.h"

//

using namespace System;

//

SIMPLE_SERIALISER_AS_INT32(::System::IndexElementSize::Type);

//

void IndexFormat::calculate_stride()
{
	if (okToUse)
	{
		return;
	}

	stride = 0;
	if (elementSize == IndexElementSize::OneByte)
	{
		stride += 1;
	}
	else if (elementSize == IndexElementSize::TwoBytes)
	{
		stride += 2;
	}
	else if (elementSize == IndexElementSize::FourBytes)
	{
		stride += 4;
	}

	okToUse = true;
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool IndexFormat::serialise(Serialiser & _serialiser)
{
	uint8 version = CURRENT_VERSION;
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid vertex format version"));
		return false;
	}
	serialise_data(_serialiser, version);
	serialise_data(_serialiser, elementSize);
	if (_serialiser.is_reading())
	{
		calculate_stride();
	}
	return true;
}

bool IndexFormat::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("element"))
		{
			if (IO::XML::Attribute const * attr = node->get_attribute(TXT("size")))
			{
				elementSize = IndexElementSize::parse(attr->get_as_string());
			}
		}
		else if (node->get_name() == TXT("notUsed"))
		{
			elementSize = IndexElementSize::None;
		}
		else
		{
			error_loading_xml(node, TXT("not recognised node type \"%S\" for index format"), node->get_name().to_char());
			result = false;
		}
	}

	calculate_stride();

	return result;
}

bool IndexFormat::operator==(IndexFormat const & _other) const
{
	return stride == _other.stride &&
		elementSize == _other.elementSize;
}
