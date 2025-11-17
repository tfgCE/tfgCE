#pragma once

#include "..\library\usedLibraryStored.h"
#include "..\..\core\system\video\videoFormat.h"

namespace Framework
{
	class Sprite;
	class Texture;
	class TexturePart;

	class SpriteTextureProcessor
	{
	public:
		struct GeneralSetup
		{
			VectorInt2 textureSize;
			System::VideoFormat::Type videoFormat;

			GeneralSetup() {}

			GeneralSetup(VectorInt2 const& _textureSize, System::VideoFormat::Type _videoFormat)
			: textureSize(_textureSize)
			, videoFormat(_videoFormat)
			{}
		};

		static GeneralSetup & access_general_setup() { return s_generalSetup; }
		static GeneralSetup const& get_general_setup() { return s_generalSetup; }

	public:
		// these methods are not required if we're just loading a game, this is all handled by library
		// but if we want to redo sprite-textures, we have to clear, add all sprites, prepare textures and then load them into gpu
		void clear(); // removes everything
		bool add(Sprite* _sprite); // adds to prepare, finds a right spot
		void add_all_sprites(Library* _library);
		void prepare_actual_textures();
		void load_textures_into_gpu(); // has to be called from the render thread

		bool update(Sprite* _sprite); // updates data for a single sprite, returns true if ok, and false if regeneration is required (clear, load all sprites), requires prepare actual textures and loading them into gpu

	public:
		static void add_job_update_and_load_if_required(); // as below, for all sprites, only if required
		static void add_job_update_and_load(Sprite* _sprite = nullptr); // adds background job to update and load sprite, if no sprite, does for all

	private:
		Concurrency::SpinLock prepareLock;

		bool requiresPrepare = false;
		bool requiresLoadIntoGPU = false;

		struct SpriteInfo
		{
			UsedLibraryStored<Sprite> forSprite;
			VectorInt2 at; // where 0,0 is
			RangeInt2 occupies;
		};

		struct TextureInfo
		{
			Name groupId;
			bool prepareTextureRequired = true;
			bool loadTextureRequired = true;
			GENERATED_ UsedLibraryStored<Texture> texture;
			VectorInt2 size;
			Array<SpriteInfo> sprites;

			Array<Colour> pixels; // all the pixels (empty by default), from bottom left corner

			void clear();
			bool add(Sprite* _sprite); // returns true if added

			void fill_pixels(Sprite* _sprite);
			void fill_pixels(int _spriteIdx);

			void prepare_actual_texture();

			void load_texture_into_gpu(); // has to be called from the render thread

			bool update(Sprite* _sprite); // prepares actual t
		};
	
		List<TextureInfo> textures;

		static GeneralSetup s_generalSetup;
	};
};
