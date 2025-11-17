#pragma once

#include "..\..\core\system\video\texture.h"

#include "..\library\libraryStored.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	struct TextureArrayLoader;

	/*
	 *	Does not import textures, only loads textures from files designated "path\\file.tex" by adding [idx] -> "path\\file [0].tex" and so on
	 */
	class TextureArray
	: public LibraryStored
	{
		FAST_CAST_DECLARE(TextureArray);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		TextureArray(Library * _library, LibraryName const & _name);

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		::System::Texture * get_texture(int _idx) const { return (textures.is_index_valid(_idx))? textures[_idx] : nullptr; }

	protected:
		virtual ~TextureArray();

		void defer_texture_delete();

	private:
		Array<::System::Texture*> textures;

		friend struct TextureArrayLoader;

		static void load_sub_texture_into_gpu(Concurrency::JobExecutionContext const * _executionContext, void* _data);
	};
};
