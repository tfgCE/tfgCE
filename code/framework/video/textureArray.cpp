#include "textureArray.h"

#include "..\..\core\types\names.h"
#include "..\..\core\concurrency\asynchronousJob.h"

#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

#include "..\..\core\serialisation\importerHelper.h"
#include "..\..\core\system\video\textureUtils.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(TextureArray);
LIBRARY_STORED_DEFINE_TYPE(TextureArray, textureArray);

TextureArray::TextureArray(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
}

struct TextureArray_DeleteTexture
: public Concurrency::AsynchronousJobData
{
	::System::Texture* texture;
	TextureArray_DeleteTexture(::System::Texture* _texture)
	: texture(_texture)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		TextureArray_DeleteTexture* data = (TextureArray_DeleteTexture*)_data;
		delete data->texture;
	}
};

TextureArray::~TextureArray()
{
	defer_texture_delete();
}

void TextureArray::defer_texture_delete()
{
	for_every_ptr(texture, textures)
	{
		Game::async_system_job(get_library()->get_game(), TextureArray_DeleteTexture::perform, new TextureArray_DeleteTexture(texture));
	}
	textures.clear();
}

bool TextureArray::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	String currentPath = IO::Utils::get_directory(_node->get_document_info()->get_original_file_path());
	String mainFileName = _node->get_string_attribute(TXT("file"));
	mainFileName = IO::Utils::make_path(currentPath, mainFileName);
	int idx = 0;
	while (true)
	{
		String fileName = mainFileName;
		String originalFileName = fileName;
		String extraFileInfo = String::printf(TXT(" [%i]"), idx);
		if (!extraFileInfo.is_empty())
		{
			int dotAt = fileName.find_last_of('.');
			if (dotAt != NONE)
			{
				fileName = fileName.get_left(dotAt) + extraFileInfo + fileName.get_sub(dotAt);
			}
			else
			{
				fileName = fileName + extraFileInfo;
			}
		}

		bool readData = false;
#ifdef AN_ANDROID
		// first try asset
		if (!readData)
		{
			if (idx == 0 &&
				! IO::AssetFile::does_exist(IO::get_asset_data_directory() + fileName))
			{
				// for [0] try the main texture
				fileName = originalFileName;
			}
			IO::AssetFile* asset = new IO::AssetFile(IO::get_asset_data_directory() + fileName);
			if (asset->is_open())
			{
				::System::Texture* loadingTexture = new ::System::Texture();
				bool resultThis = serialise_data(Serialiser::reader(asset).temp_ref(), loadingTexture);
				if (resultThis)
				{
					textures.push_back(loadingTexture);
					readData = true;
				}
				else
				{
					delete loadingTexture;
					result = false;
				}
			}
			delete asset;
		}
#endif
		if (!readData)
		{
			if (idx == 0 &&
				!IO::File::does_exist(IO::get_user_game_data_directory() + fileName))
			{
				// for [0] try the main texture
				fileName = originalFileName;
			}
			IO::File* file = new IO::File(IO::get_user_game_data_directory() + fileName);
			if (file->is_open())
			{
				::System::Texture* loadingTexture = new ::System::Texture();
				bool resultThis = serialise_data(Serialiser::reader(file).temp_ref(), loadingTexture);
				if (resultThis)
				{
					textures.push_back(loadingTexture);
					readData = true;
				}
				else
				{
					delete loadingTexture;
					result = false;
				}
			}
			delete file;
		}
		if (readData)
		{
			++idx;
		}
		else
		{
			break;
		}
	}

	return result;
}

struct LoadSubTextureData
: public Concurrency::AsynchronousJobData
{
	TextureArray* textureArray;
	int subTextureIdx;
	Library* library;
	LoadSubTextureData(TextureArray* _textureArray, int _subTextureIdx, Library* _library)
		: textureArray(_textureArray)
		, subTextureIdx(_subTextureIdx)
		, library(_library)
	{}
};

bool TextureArray::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadTexture)
	{
		for_every_ptr(texture, textures)
		{
			_library->preparing_asynchronous_job_added();
			Game::async_system_job(get_library()->get_game(), load_sub_texture_into_gpu, new LoadSubTextureData(this, for_everys_index(texture), _library));
		}
	}

	return result;
}

void TextureArray::load_sub_texture_into_gpu(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{ 
	LoadSubTextureData* loadData = (LoadSubTextureData*)_data;
	TextureArray* self = loadData->textureArray;
	auto* texture = self->textures[loadData->subTextureIdx];
#ifdef AN_DEVELOPMENT
	an_assert(texture->get_setup(), TXT("texture \"%S\" has no setup, loaded from file %S"), self->get_name().to_string().to_char(), self->get_loaded_from_file().to_char());
#endif
	texture->init_as_stored();
	loadData->library->preparing_asynchronous_job_done();
}
