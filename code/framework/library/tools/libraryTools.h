#pragma once

#include "..\libraryTool.h"

namespace Framework
{
	class LibraryTools
	{
	public:
		static bool generate_texture(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		static bool generate_font_texture(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		static bool modify_current_environment(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	};

};

