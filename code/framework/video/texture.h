#pragma once

#include "..\..\core\system\video\texture.h"

#include "..\library\libraryStored.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	struct TextureImporter;

	class Texture
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Texture);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Texture(Library * _library, LibraryName const & _name);

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		::System::Texture * get_texture() const { return texture; }

		static void add_defaults(Library* _library);
		static Texture * get_default(Library* _library);

	protected:
		virtual ~Texture();

		void defer_texture_delete();

	private:
		::System::Texture* texture;

		friend struct TextureImporter;

		static void load_texture_into_gpu(Concurrency::JobExecutionContext const * _executionContext, void* _data);
	};
};
