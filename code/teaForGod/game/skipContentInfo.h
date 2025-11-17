#pragma once

#include "..\..\core\tags\tag.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace TeaForGodEmperor
{
	struct SkipContentInfo
	{
		Name generalProgressTag;
		Optional<float> gameSlotThreshold;
		Optional<float> profileThreshold;

		bool load_from_xml(IO::XML::Node const* _node);
	};

	struct SkipContentSet
	{
		Array<SkipContentInfo> skipContentInfos;

		void process(OUT_ Tags & _skipContentTags) const;

		bool load_from_xml(IO::XML::Node const* _node);
	};
};
