#include "texture.h"

#include "..\..\core\types\names.h"
#include "..\..\core\concurrency\asynchronousJob.h"

#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

#include "..\..\core\serialisation\importerHelper.h"
#include "..\..\core\system\video\textureUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Texture);
LIBRARY_STORED_DEFINE_TYPE(Texture, texture);

Texture::Texture(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, texture(new ::System::Texture())
{
}

struct Texture_DeleteTexture
: public Concurrency::AsynchronousJobData
{
	::System::Texture* texture;
	Texture_DeleteTexture(::System::Texture* _texture)
	: texture(_texture)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		Texture_DeleteTexture* data = (Texture_DeleteTexture*)_data;
		delete data->texture;
	}
};

Texture::~Texture()
{
	defer_texture_delete();
}

void Texture::defer_texture_delete()
{
	if (texture)
	{
		Game::async_system_job(get_library()->get_game(), Texture_DeleteTexture::perform, new Texture_DeleteTexture(texture));
		texture = nullptr;
	}
}

struct ::Framework::TextureImporter
: public ImporterHelper<::System::Texture, ::System::TextureImportOptions>
{
	typedef ImporterHelper<::System::Texture, ::System::TextureImportOptions> base;

	TextureImporter(Framework::Texture* _texture)
	: base(_texture->texture)
	, texture(_texture)
	{}

protected:
	override_ void delete_object()
	{
		texture->defer_texture_delete();
	}

	override_ bool on_setup_from_xml(IO::XML::Node const* _node)
	{
		bool result = base::on_setup_from_xml(_node);

		result &= addBorder.load_from_xml(_node);

		return result;
	}

	override_ void import_object()
	{
		object = ::System::Texture::importer().import(importFromFileName, importOptions);
	}

	override_ void on_import_pre_serialisation_to_file()
	{
		base::on_import_pre_serialisation_to_file();
		if (addBorder.is_required())
		{
			if (auto* s = object->get_setup())
			{
				::System::TextureSetup setup = *s;

				addBorder.process(setup);

				object->use_setup_to_init(setup);
			}
		}
	}

private:
	Framework::Texture* texture;

	::System::TextureUtils::AddBorder addBorder;
};

bool Texture::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	TextureImporter textureImporter(this);
	result &= textureImporter.setup_from_xml(_node, _lc.get_library_source() == LibrarySource::Assets);
	if (!textureImporter.import())
	{
#ifdef AN_OUTPUT_IMPORT_ERRORS
		error_loading_xml(_node, TXT("can't import"));
#endif
		result = false;
	}

	return result;
}

struct LoadTextureData
: public Concurrency::AsynchronousJobData
{
	Texture* texture;
	Library* library;
	LoadTextureData(Texture * _texture, Library* _library)
	: texture(_texture)
	, library(_library)
	{}
};

bool Texture::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadTexture)
	{
		_library->preparing_asynchronous_job_added();
		Game::async_system_job(get_library()->get_game(), load_texture_into_gpu, new LoadTextureData(this, _library));
	}

	return result;
}

void Texture::load_texture_into_gpu(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{ 
	LoadTextureData * loadData = (LoadTextureData*)_data;
	Texture* self = loadData->texture;
#ifdef AN_DEVELOPMENT
	an_assert(self->texture->get_setup(), TXT("texture \"%S\" has no setup, loaded from file %S"), self->get_name().to_string().to_char(), self->get_loaded_from_file().to_char());
#endif
	self->texture->init_as_stored();
	loadData->library->preparing_asynchronous_job_done();
}

void Texture::add_defaults(Library* _library)
{
	if (!_library->get_textures().find(LibraryName(Names::default_.to_string(), Names::default_.to_string())))
	{
		Texture* texture = _library->get_textures().find_or_create(LibraryName(Names::default_.to_string(), Names::default_.to_string()));

		texture->texture->use_setup_to_init(::System::TextureSetup(Colour::white));
	}
}

Texture * Texture::get_default(Library* _library)
{
	return _library->get_textures().find(LibraryName(Names::default_.to_string(), Names::default_.to_string()));
}
