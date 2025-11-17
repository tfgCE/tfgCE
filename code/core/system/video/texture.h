#pragma once

#include "video.h"

#include "videoFormat.h"
#include "..\..\serialisation\serialisableResource.h"
#include "..\..\serialisation\importer.h"
#include "videoEnums.h"

namespace System
{
	class Surface;

	struct TextureCommonSetup
	{
		bool withMipMaps;
		TextureWrap::Type wrapU;
		TextureWrap::Type wrapV;
		TextureFiltering::Type filteringMag;
		TextureFiltering::Type filteringMin;

		TextureCommonSetup();

		bool load_from_xml(IO::XML::Node const * _node);
		bool serialise(Serialiser & _serialiser, int _version);
	};

	struct TextureImportOptions
	: public TextureCommonSetup
	{
		typedef TextureCommonSetup base;

		bool autoMipMaps = false;

		bool load_from_xml(IO::XML::Node const* _node);
		// not serialisable
	};

	struct TextureImportSetup
	: public TextureCommonSetup
	{
		typedef TextureCommonSetup base;

		bool ignoreIfNotAbleToLoad = false; // to skip error messages on loading
		String sourceFile;
		String forceType; // "tga"
		bool autoMipMaps = false; // if set to true, will generate all mip maps

		TextureImportSetup();
	};

	struct TextureSetup
	: public TextureCommonSetup
	{
		typedef TextureCommonSetup base;

		// data to load to gpu/opengl texture
		VideoFormat::Type format;
		BaseVideoFormat::Type mode;
		VectorInt2 size;
		int mipMapLevels; // 1 to have only one! 0 to have unlimited, auto generated
		Array<Array<int8>> pixels;
		Array<VectorInt2> levelSize;

		TextureSetup();
		TextureSetup(Colour const & _colour);

		bool load_from_xml(IO::XML::Node const * _node);
		bool serialise(Serialiser & _serialiser, int _version);

		void create_pixels(bool _clear = true);
		void draw_pixel(int _level, VectorInt2 const & _pos, Colour const & _colour);
		Colour get_pixel(int _level, VectorInt2 const & _pos) const;

		bool is_pos_valid(int _level, VectorInt2 const& _pos) const; // clamping etc
		VectorInt2 validate_pos(int _level, VectorInt2 const& _pos) const; // clamping etc
	};

	class Texture
	: public SerialisableResource
	{
	public:
		Texture();
		~Texture();

		static Importer<Texture, TextureImportOptions> & importer() { return s_importer; }

		inline bool is_valid() const { return isValid; }

		void close();
		bool init_with_setup_dont_create(TextureImportSetup const & _setup);
		bool init_with_setup(TextureImportSetup const & _setup);
		bool init_as_stored(); // load imported/serialised data into gpu

		void use_setup_to_init(TextureSetup const & _setup);
		TextureSetup const * get_setup() const { return setup.is_set()? &setup.get() : nullptr; }

		TextureID const get_texture_id() const { return textureObject; }
		
		VectorInt2 get_size() const { return size; }

	public: // SerialisableResource
		virtual bool serialise(Serialiser & _serialiser);

	private:
		static Importer<Texture, TextureImportOptions> s_importer;

	private:
		TextureID textureObject;
		bool isValid;
		VectorInt2 size;

		Optional<TextureSetup> setup;
	};
};
