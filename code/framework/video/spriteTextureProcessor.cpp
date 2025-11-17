#include "spriteTextureProcessor.h"

#include "colourPalette.h"
#include "sprite.h"

#include "..\game\game.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\concurrency\scopedSpinLock.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

SpriteTextureProcessor::GeneralSetup SpriteTextureProcessor::s_generalSetup(VectorInt2(1024, 1024), System::VideoFormat::rgba8);

void SpriteTextureProcessor::clear()
{
	Concurrency::ScopedSpinLock lock(prepareLock, true);

	requiresPrepare = true;

	// clear texture parts being used by sprites to free (temporary) texture parts
	for_every(t, textures)
	{
		t->clear();
	}

	textures.clear();
}

void SpriteTextureProcessor::add_all_sprites(Library* _library)
{
	Concurrency::ScopedSpinLock lock(prepareLock, true);

	requiresPrepare = true;

	_library->get_sprites().do_for_every(
		[this](LibraryStored* _libraryStored)
		{
			if (Sprite* s = fast_cast<Sprite>(_libraryStored))
			{
				add(s);
			}
		});
}

bool SpriteTextureProcessor::add(Sprite * _sprite)
{
	Concurrency::ScopedSpinLock lock(prepareLock, true);

	requiresPrepare = true;

	Name groupId = _sprite->textureUsage.groupId;

	for_every(t, textures)
	{
		for_every(s, t->sprites)
		{
			if (s->forSprite.get() == _sprite)
			{
				// already added, no need to add
				return true;
			}
		}
	}

	for_every(t, textures)
	{
		if (t->groupId == groupId)
		{
			if (t->add(_sprite))
			{
				return true;
			}
		}
	}

	textures.push_back();
	auto& t = textures.get_last();

	t.groupId = groupId;
	t.size = get_general_setup().textureSize;
	t.pixels.set_size(t.size.x * t.size.y);
	for_every(p, t.pixels)
	{
		*p = Colour::none;
	}

	bool result = t.add(_sprite);
	
	an_assert(result);

	return result;
}

void SpriteTextureProcessor::prepare_actual_textures()
{
	Concurrency::ScopedSpinLock lock(prepareLock, true);

	for_every(t, textures)
	{
		t->prepare_actual_texture();
	}

	requiresPrepare = false;
	requiresLoadIntoGPU = true;
}

void SpriteTextureProcessor::load_textures_into_gpu()
{
	assert_rendering_thread();

	Concurrency::ScopedSpinLock lock(prepareLock, true);

	for_every(t, textures)
	{
		t->load_texture_into_gpu();
	}

	requiresLoadIntoGPU = false;
}

bool SpriteTextureProcessor::update(Sprite* _sprite)
{
	Concurrency::ScopedSpinLock lock(prepareLock, true);

	for_every(t, textures)
	{
		if (t->update(_sprite))
		{
			return true;
		}
	}

	return false;
}

void SpriteTextureProcessor::add_job_update_and_load_if_required()
{
	auto& stp = Library::get_current()->access_sprite_texture_processor();
	if (stp.requiresLoadIntoGPU || stp.requiresPrepare)
	{
		add_job_update_and_load();
	}
}

void SpriteTextureProcessor::add_job_update_and_load(Sprite* _sprite)
{
	Game::async_background_job(Game::get(), [_sprite]()
		{
			auto& stp = Library::get_current()->access_sprite_texture_processor();
			bool updated = _sprite && stp.update(_sprite);

			if (!updated)
			{
				stp.clear();
				Library::get_current()->remove_unused_temporary_objects(); // remove all unused as we could just release them in clear
				stp.add_all_sprites(Library::get_current());
			}

			stp.prepare_actual_textures();

			Game::async_system_job(Game::get(), []() {
				auto& stp = Library::get_current()->access_sprite_texture_processor(); 
				stp.load_textures_into_gpu();
				});
		});
}

//--

void SpriteTextureProcessor::TextureInfo::clear()
{
	sprites.clear();
	texture.clear();

	prepareTextureRequired = true;
	loadTextureRequired = true;
}

bool SpriteTextureProcessor::TextureInfo::add(Sprite* _sprite)
{
	RangeInt2 textureRange = RangeInt2::empty;
	RangeInt2 spriteRange = SpriteContentAccess::get_combined_range(_sprite);
	VectorInt2 spriteSize = spriteRange.length();
	
	textureRange.include(VectorInt2::zero);
	textureRange.include(size - VectorInt2::one);

	int bottomY = 0;
	while (bottomY <= size.y - spriteSize.y)
	{
		int leftX = 0;
		while (leftX <= size.x - spriteSize.x)
		{
			VectorInt2 placedAt(leftX, bottomY);
			RangeInt2 placedSprite = RangeInt2::empty;
			placedSprite.include(placedAt);
			placedSprite.include(placedAt + spriteSize - VectorInt2::one);

			bool isOk = false;
			if (textureRange.does_contain(placedSprite))
			{
				isOk = true;
				for_every(s, sprites)
				{
					if (s->occupies.overlaps(placedSprite))
					{
						isOk = false;
						break;
					}
				}
			}

			if (isOk)
			{
				VectorInt2 spriteAt = placedAt - spriteRange.bottom_left();

				sprites.push_back();
				auto& s = sprites.get_last();

				s.forSprite = _sprite;
				s.at = spriteAt;
				s.occupies = placedSprite;
				
				fill_pixels(sprites.get_size() - 1);

				return true;
			}

			++leftX;
		}

		++bottomY;
	}

	return false;
}

void SpriteTextureProcessor::TextureInfo::fill_pixels(Sprite* _sprite)
{
	for_every(s, sprites)
	{
		if (s->forSprite.get() == _sprite)
		{
			fill_pixels(for_everys_index(s));
			return;
		}
	}
	an_assert(false, TXT("sprite not found"));
}

void SpriteTextureProcessor::TextureInfo::fill_pixels(int _idx)
{
	an_assert(sprites.is_index_valid(_idx));

	auto& s = sprites[_idx];

	for_range(int, y, s.occupies.y.min, s.occupies.y.max)
	{
		Colour* p = &pixels[size.x * y + s.occupies.x.min];
		for_range(int, x, s.occupies.x.min, s.occupies.x.max)
		{
			*p = Colour::none;
			++p;
		}
	}

	if (auto* sprite = s.forSprite.get())
	{
		if (auto* colourPalette = sprite->get_colour_palette())
		{
			if (auto* sl = sprite->resultLayer.get())
			{
				RangeInt2 srcRange = sl->get_range();
				RangeInt2 srcOccupies = s.occupies;
				srcOccupies.offset(-s.at);

				srcRange.intersect_with(srcOccupies);

				for_range(int, sy, srcRange.y.min, srcRange.y.max)
				{
					Colour* dest = &pixels[size.x * (s.at.y + sy) + (s.at.x + srcRange.x.min)];
					SpritePixel const* srcSP = sl->get_at(VectorInt2(srcRange.x.min, sy));
					an_assert(srcSP);
					for_range(int, x, srcRange.x.min, srcRange.x.max)
					{
						if (SpritePixel::is_solid(srcSP->pixel))
						{
							an_assert(colourPalette->get_colours().is_index_valid(srcSP->pixel));
							*dest = colourPalette->get_colours()[srcSP->pixel];
						}
						++dest;
						++srcSP;
					}
				}
			}
		}
		else
		{
			error(TXT("no colours for sprite"));
		}
	}

	prepareTextureRequired = true;
	loadTextureRequired = true;
}

void SpriteTextureProcessor::TextureInfo::prepare_actual_texture()
{
	if (!prepareTextureRequired)
	{
		return;
	}
	prepareTextureRequired = false;

	if (!texture.get())
	{
		texture = Library::get_current()->get_textures().create_temporary();
	}

	if (auto* t = texture->get_texture())
	{
		System::TextureSetup textureSetup;
		
		// common
		textureSetup.withMipMaps = false;
		textureSetup.wrapU = System::TextureWrap::clamp;
		textureSetup.wrapV = System::TextureWrap::clamp;
		textureSetup.filteringMag = System::TextureFiltering::get_sane_mag(System::TextureFiltering::nearest);
		textureSetup.filteringMin = System::TextureFiltering::get_sane_min(System::TextureFiltering::nearest);

		textureSetup.format = SpriteTextureProcessor::get_general_setup().videoFormat;
		textureSetup.mode = System::VideoFormat::to_base_video_format(textureSetup.format);
		textureSetup.size = size;
		textureSetup.mipMapLevels = 0;
		
		// create pixels
		textureSetup.create_pixels();
		for_count(int, y, size.y)
		{
			Colour const* src = &pixels[y * size.x];
			for_count(int, x, size.x)
			{
				textureSetup.draw_pixel(0, VectorInt2(x, y), *src);
				++src;
			}
		}

		t->use_setup_to_init(textureSetup);
	}

	for_every(s, sprites)
	{
		if (auto* sprite = s->forSprite.get())
		{
			Vector2 uv0;
			Vector2 uv1;

			// first prepare in pixels
			uv0 = s->occupies.bottom_left().to_vector2();
			uv1 = s->occupies.top_right().to_vector2() + Vector2::one;

			Vector2 textureSize = size.to_vector2();
			uv0 /= textureSize;
			uv1 /= textureSize;

			sprite->textureUsage.texture = texture.get();
			sprite->textureUsage.uvRange = Range2::empty;
			sprite->textureUsage.uvRange.include(uv0);
			sprite->textureUsage.uvRange.include(uv1);
			sprite->textureUsage.size = s->occupies.length();
			sprite->textureUsage.leftBottomOffset = s->occupies.bottom_left() - s->at;
		}
	}
}

void SpriteTextureProcessor::TextureInfo::load_texture_into_gpu()
{
	if (!loadTextureRequired)
	{
		return;
	}
	loadTextureRequired = false;

	assert_rendering_thread();

	if (auto* t = texture.get())
	{
		auto* st = t->get_texture();
		st->init_as_stored();
	}
}

bool SpriteTextureProcessor::TextureInfo::update(Sprite* _sprite)
{
	for_every(s, sprites)
	{
		if (s->forSprite.get() == _sprite)
		{
			RangeInt2 spriteRange = SpriteContentAccess::get_combined_range(_sprite);
			spriteRange.offset(-s->at); // move to the right position
			if (s->occupies.does_contain(spriteRange))
			{
				fill_pixels(for_everys_index(s));
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}
