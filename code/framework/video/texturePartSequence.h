#pragma once

#include "texturePart.h"

namespace Framework
{
	struct TexturePartSequence
	{
		TexturePart* get(int _idx) const { return elements[_idx].get(); }
 		int length() const { return elements.get_size(); }
		float get_interval() const { return interval; }

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		float interval = 0.1f;
		Array<UsedLibraryStored<TexturePart>> elements;
	};
};
