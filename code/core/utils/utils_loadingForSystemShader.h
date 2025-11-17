#pragma once

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace CoreUtils
{
	namespace Loading
	{
		bool should_load_for_system_or_shader_option(IO::XML::Node const* _forNode);
	};
};
