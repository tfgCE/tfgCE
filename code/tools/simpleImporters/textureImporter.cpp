#include "textureImporter.h"
#include "..\..\core\system\video\texture.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace SimpleImporters;

//

System::Texture* import_from_file(String const & _fileName, System::TextureImportOptions const & _options)
{
	System::Texture* texture = new ::System::Texture();
	System::TextureImportSetup setup;
	setup.sourceFile = _fileName;
	setup.autoMipMaps = _options.autoMipMaps;
	*((System::TextureCommonSetup*)&setup) = *((System::TextureCommonSetup*)&_options);
	if (texture->init_with_setup_dont_create(setup))
	{
		// everything is ok
	}
	else
	{
		delete_and_clear(texture);
		error(TXT("problem importing texture from file \"%S\""), _fileName.to_char());
	}
	return texture;
}

void TextureImporter::initialise_static()
{
	// bmp, PNM(PPM / PGM / PBM), XPM, LBM, pcx, gif, jpeg, png, tga
	System::Texture::importer().register_file_type_with_options(String(TXT("tga")), [](String const & _fileName, System::TextureImportOptions const & _options) { return import_from_file(_fileName, _options); });
	System::Texture::importer().register_file_type_with_options(String(TXT("bmp")), [](String const & _fileName, System::TextureImportOptions const & _options) { return import_from_file(_fileName, _options); });
	System::Texture::importer().register_file_type_with_options(String(TXT("png")), [](String const & _fileName, System::TextureImportOptions const & _options) { return import_from_file(_fileName, _options); });
	System::Texture::importer().register_file_type_with_options(String(TXT("gif")), [](String const & _fileName, System::TextureImportOptions const & _options) { return import_from_file(_fileName, _options); });
	System::Texture::importer().register_file_type_with_options(String(TXT("jpg")), [](String const & _fileName, System::TextureImportOptions const & _options) { return import_from_file(_fileName, _options); });
	System::Texture::importer().register_file_type_with_options(String(TXT("pcx")), [](String const & _fileName, System::TextureImportOptions const & _options) { return import_from_file(_fileName, _options); });
}

void TextureImporter::close_static()
{
	System::Texture::importer().unregister(String(TXT("tga")));
	System::Texture::importer().unregister(String(TXT("bmp")));
	System::Texture::importer().unregister(String(TXT("png")));
	System::Texture::importer().unregister(String(TXT("gif")));
	System::Texture::importer().unregister(String(TXT("jpg")));
	System::Texture::importer().unregister(String(TXT("pcx")));
}

void TextureImporter::finished_importing()
{

}