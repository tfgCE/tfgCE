#include "spriteGrid.h"

#include "basicDrawRenderingBuffer.h"

#include "..\editor\editors\editorSpriteGrid.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// tags
DEFINE_STATIC_NAME(spriteGridPriority);

//

int grid_content_index(VectorInt2 const& _coord, RangeInt2 const& _range)
{
	if (!_range.does_contain(_coord))
	{
		return NONE;
	}
	int const rangex = _range.x.length();
	int const index
		/*
		=	(_coord.z - _range.z.min) * rangex * rangey
		+	(_coord.y - _range.y.min) * rangex
		+	(_coord.x - _range.x.min)
		*/
		= (_coord.y - _range.y.min) * rangex
		+ (_coord.x - _range.x.min)
		;
	return index;
}


//--

REGISTER_FOR_FAST_CAST(SpriteGrid);
LIBRARY_STORED_DEFINE_TYPE(SpriteGrid, spriteGrid);

SpriteGrid::SpriteGrid(Library* _library, LibraryName const& _name)
: base(_library, _name)
, palette(this)
, content(this)
{
}

SpriteGrid::~SpriteGrid()
{
}

#ifdef AN_DEVELOPMENT
void SpriteGrid::ready_for_reload()
{
	base::ready_for_reload();

	palette.reset();
	content.reset();
}
#endif

void SpriteGrid::prepare_to_unload()
{
	base::prepare_to_unload();

	palette.reset();
	content.reset();
}

bool SpriteGrid::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset
	//
	palette.reset();
	content.reset();
	spriteSize = VectorInt2(8, 8);

	// load
	//
	spriteSize.load_from_xml_child_node(_node, TXT("spriteSize"));
	
	for_every(node, _node->children_named(TXT("palette")))
	{
		result &= palette.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("content")))
	{
		result &= content.load_from_xml(node, _lc);
	}

	return result;
}

bool SpriteGrid::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= palette.prepare_for_game(_library, _pfgContext);

	return result;
}

bool SpriteGrid::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	spriteSize.save_to_xml_child_node(_node, TXT("spriteSize"));

	palette.save_to_xml(_node->add_node(TXT("palette")));
	content.save_to_xml(_node->add_node(TXT("content")));

	return result;
}

void SpriteGrid::debug_draw() const
{
}

bool SpriteGrid::editor_render() const
{
	content.editor_render();
	return true;
}

Editor::Base* SpriteGrid::create_editor(Editor::Base* _current) const
{
	if (fast_cast<Editor::SpriteGrid>(_current))
	{
		return _current;
	}
	else
	{
		return new Editor::SpriteGrid();
	}
}

int SpriteGrid::pixel_to_grid_x(int const& _pixel) const
{
	int coord = _pixel;
	coord = round_down_to(coord, spriteSize.x);
	coord /= spriteSize.x;
	return coord;
}

int SpriteGrid::pixel_to_grid_y(int const& _pixel) const
{
	int coord = _pixel;
	coord = round_down_to(coord, spriteSize.y);
	coord /= spriteSize.y;
	return coord;
}

VectorInt2 SpriteGrid::pixel_to_grid(VectorInt2 const& _pixel) const
{
	VectorInt2 coord = _pixel;
	coord.x = round_down_to(coord.x, spriteSize.x);
	coord.y = round_down_to(coord.y, spriteSize.y);
	coord /= spriteSize;
	return coord;
}

VectorInt2 SpriteGrid::grid_to_pixel(VectorInt2 const& _gridCoord) const
{
	return _gridCoord * spriteSize;
}

RangeInt2 SpriteGrid::grid_to_pixel_range(VectorInt2 const& _gridCoord) const
{
	RangeInt2 r = RangeInt2::empty;

	VectorInt2 p = grid_to_pixel(_gridCoord);

	r.include(p);
	r.include(p + spriteSize - VectorInt2::one);

	return r;
}

RangeInt2 SpriteGrid::grid_to_pixel_range(RangeInt2 const& _gridRange) const
{
	RangeInt2 r = RangeInt2::empty;

	r.include(_gridRange.bottom_left() * spriteSize);
	r.include((_gridRange.top_right() + VectorInt2::one) * spriteSize - VectorInt2::one);

	return r;
}

RangeInt2 SpriteGrid::pixel_to_grid_range(RangeInt2 const& _pixelRange) const
{
	VectorInt2 lb = _pixelRange.bottom_left();
	VectorInt2 rt = _pixelRange.top_right();

	lb = pixel_to_grid(lb);
	rt = pixel_to_grid(rt);

	RangeInt2 r = RangeInt2(lb, rt);

	return r;
}

void SpriteGrid::add_to(BasicDrawRenderingBuffer& _renderingBuffer, VectorInt2 const& _zeroAt, RangeInt2 const& _clipTo) const
{
	RangeInt2 renderRange = content.range;

	if (renderRange.is_empty())
	{
		return;
	}

	if (!_clipTo.is_empty())
	{
		// in pixels
		RangeInt2 r = _clipTo;

		// take it to local coordinates
		r.offset(-_zeroAt);

		r = pixel_to_grid_range(r);

		renderRange.intersect_with(r);
	}

	Optional<int> currentPriority;
	Optional<int> nextPriority;

	while (true)
	{
		for_range(int, y, renderRange.y.min, renderRange.y.max)
		{
			int const* sIdx = &content.content[grid_content_index(VectorInt2(renderRange.x.min, y), content.range)];
			for_range(int, x, renderRange.x.min, renderRange.x.max)
			{
				if (auto* s = palette.get(*sIdx))
				{
					int p = s->get_sprite_grid_draw_priority();
					if (currentPriority.is_set())
					{
						if (currentPriority.get() == p)
						{
							_renderingBuffer.add(s, _zeroAt + spriteSize * VectorInt2(x, y));
						}
						else if (p > currentPriority.get())
						{
							if (!nextPriority.is_set() || nextPriority.get() > p)
							{
								nextPriority = p;
							}
						}
					}
					else
					{
						if (!nextPriority.is_set() || nextPriority.get() > p)
						{
							nextPriority = p;
						}
					}
				}
				++sIdx;
			}
		}

		if (nextPriority.is_set())
		{
			currentPriority = nextPriority;
			nextPriority.clear();
			continue;
		}
		else
		{
			break;
		}
	}
}

Sprite const* SpriteGrid::get_at(VectorInt2 const& _at) const
{
	int idx = content.get_at(_at);
	return palette.get(idx);
}

//--

SpriteGridContent::SpriteGridContent(SpriteGrid* _forGrid)
: grid(_forGrid)
{
}

SpriteGridContent::~SpriteGridContent()
{
}

bool SpriteGridContent::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	// clear/reset
	//
	range = RangeInt2::empty;
	content.clear();

	// load
	//
	range.load_from_xml_child_node(_node, TXT("range"));

	content.make_space_for(range.x.length() * range.y.length());

	for_every(node, _node->children_named(TXT("contentRaw")))
	{
		int chPerNode = node->get_int_attribute(TXT("chPerNode"), 1);
		int contentOff = node->get_int_attribute(TXT("contentOff"), 1);
		String contentText = node->get_text();
		String readContent;
		int readCh = 0;
		for_every(ch, contentText.get_data())
		{
			if (*ch == 0 || readCh >= chPerNode)
			{
				content.push_back();
				auto& v = content.get_last();
				v = ParserUtils::parse_hex(readContent.to_char()) - contentOff;
				readContent = String::empty();
				readCh = 0;
				if (*ch == 0)
				{
					break;
				}
			}
			{
				readContent += *ch;
				++readCh;
			}
		}
		an_assert(readContent.is_empty());
	}

	return result;
}

bool SpriteGridContent::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	range.save_to_xml_child_node(_node, TXT("range"));

	{
		int storedInNode = 0;
		int storedInNodeLimit = 256;
		int lastContentIdx = content.get_size() - 1;
		struct ContentToSave
		{
			Array<int> content;
			String contentText;
			String contentFormat;
			bool save_to_xml(IO::XML::Node* _node)
			{
				if (content.is_empty())
				{
					return true;
				}
				int contentOff = -content.get_first();
				for_every(v, content)
				{
					contentOff = max(contentOff, -(*v));
				}
				int maxValue = 0;
				for_every(v, content)
				{
					maxValue = max(maxValue, (*v) + contentOff);
				}
				String maxValueAsHex = String::printf(TXT("%x"), maxValue);
				int chPerNode = maxValueAsHex.get_length();

				auto* contentRawNode = _node->add_node(TXT("contentRaw"));
				contentRawNode->set_int_attribute(TXT("chPerNode"), chPerNode);
				contentRawNode->set_int_attribute(TXT("contentOff"), contentOff);

				contentText = String::empty();
				contentFormat = String::printf(TXT("%%0%ix"), chPerNode);
				for_every(v, content)
				{
					contentText += String::printf(contentFormat.to_char(), (*v) + contentOff); // offset, see above
				}
				contentRawNode->add_text(contentText);

				clear();

				return true;
			}
			void clear()
			{
				content.clear();
				contentText = String::empty();
			}
		} contentToSave;
		for_every(v, content)
		{
			contentToSave.content.push_back(*v);
			++storedInNode;
			if (storedInNode >= storedInNodeLimit ||
				for_everys_index(v) == lastContentIdx)
			{
				result &= contentToSave.save_to_xml(_node);
				storedInNode = 0;
			}
		}
		result &= contentToSave.save_to_xml(_node);

		an_assert(storedInNode == 0);
	}

	return result;
}

void SpriteGridContent::reset()
{
	clear();
}

void SpriteGridContent::clear()
{
	range = RangeInt2::empty;
	content.clear();
}

void SpriteGridContent::prune()
{
	RangeInt2 occupiedRange = RangeInt2::empty;

	for_range(int, y, range.y.min, range.y.max)
	{
		VectorInt2 startAt(range.x.min, y);
		int const* v = &content[grid_content_index(startAt, range)];
		for_range(int, x, range.x.min, range.x.max)
		{
			if (*v != NONE)
			{
				occupiedRange.include(VectorInt2(x, y));
			}
			++v;
		}
	}

	if (occupiedRange != range)
	{
		Array<int> oldContent = content;
		RangeInt2 oldRange = range;

		range = occupiedRange;

		content.set_size(range.x.length() * range.y.length());
		for_every(v, content)
		{
			// clear
			*v = NONE;
		}

		if (!oldContent.is_empty() && !oldRange.is_empty())
		{
			copy_intersecting_data_from(oldRange, oldContent);
		}
	}
}

int& SpriteGridContent::access_at(VectorInt2 const& _at)
{
	if (!range.does_contain(_at))
	{
		Array<int> oldContent = content;
		RangeInt2 oldRange = range;

		range.include(_at);

		content.set_size(range.x.length() * range.y.length());
		for_every(v, content)
		{
			// clear
			*v = NONE;
		}

		if (!oldContent.is_empty() && !oldRange.is_empty())
		{
			copy_intersecting_data_from(oldRange, oldContent);
		}
	}

	return content[grid_content_index(_at, range)];
}

void SpriteGridContent::clear_at(VectorInt2 const& _at)
{
	if (range.does_contain(_at))
	{
		content[grid_content_index(_at, range)] = NONE;
	}
}

int SpriteGridContent::get_at(VectorInt2 const& _at) const
{
	if (!range.does_contain(_at))
	{
		return NONE;
	}

	return content[grid_content_index(_at, range)];
}

void SpriteGridContent::get_all(int _idx, OUT_ Array<VectorInt2>& _at, bool _remove)
{
	for_range(int, y, range.y.min, range.y.max)
	{
		int* sIdx = &content[grid_content_index(VectorInt2(range.x.min, y), range)];
		for_range(int, x, range.x.min, range.x.max)
		{
			if (*sIdx == _idx)
			{
				_at.push_back(VectorInt2(x, y));
				if (_remove)
				{
					*sIdx = NONE;
				}
			}
			++sIdx;
		}
	}
}

void SpriteGridContent::copy_intersecting_data_from(RangeInt2 const& _otherRange, Array<int> const& _otherContent)
{
	// copy old content
	RangeInt2 commonRange = range;
	commonRange.intersect_with(_otherRange);
	for_range(int, y, commonRange.y.min, commonRange.y.max)
	{
		VectorInt2 startAt(commonRange.x.min, y);
		int const* src = &_otherContent[grid_content_index(startAt, _otherRange)];
		int* dst = &content[grid_content_index(startAt, range)];
		for_range(int, x, commonRange.x.min, commonRange.x.max)
		{
			*dst = *src;
			++dst;
			++src;
		}
	}
}

void SpriteGridContent::offset_placement(VectorInt2 const& _by)
{
	if (!range.is_empty())
	{
		range.offset(_by);
	}
}

void SpriteGridContent::editor_render() const
{
	if (range.is_empty())
	{
		return;
	}
	auto* v3d = ::System::Video3D::get();
	auto spriteSize = grid->spriteSize;

	Optional<int> currentPriority;
	Optional<int> nextPriority;

	while (true)
	{
		for_range(int, y, range.y.min, range.y.max)
		{
			int const * sIdx = &content[grid_content_index(VectorInt2(range.x.min, y), range)];
			for_range(int, x, range.x.min, range.x.max)
			{
				if (*sIdx != NONE)
				{
					if (auto* s = grid->palette.get(*sIdx))
					{
						int p = s->get_sprite_grid_draw_priority();
						if (currentPriority.is_set())
						{
							if (currentPriority.get() == p)
							{
								s->draw_at(v3d, (VectorInt2(x, y) * spriteSize));
							}
							else if (p > currentPriority.get())
							{
								if (!nextPriority.is_set() || nextPriority.get() > p)
								{
									nextPriority = p;
								}
							}
						}
						else
						{
							if (!nextPriority.is_set() || nextPriority.get() > p)
							{
								nextPriority = p;
							}
						}
					}
				}
				++sIdx;
			}
		}

		if (nextPriority.is_set())
		{
			currentPriority = nextPriority;
			nextPriority.clear();
			continue;
		}
		else
		{
			break;
		}
	}
}

//--

SpriteGridPalette::SpriteGridPalette(SpriteGrid* _forGrid)
: grid(_forGrid)
{
}

int SpriteGridPalette::find_index(Sprite const* _s) const
{
	for_every(s, sprites)
	{
		if (s->get() == _s)
		{
			return for_everys_index(s);
		}
	}
	return NONE;
}

Sprite const* SpriteGridPalette::get(int _idx) const
{
	if (sprites.is_index_valid(_idx))
	{
		return sprites[_idx].get();
	}
	return nullptr;
}

SpriteGridPalette::Entry const * SpriteGridPalette::get_entry(int _idx) const
{
	if (sprites.is_index_valid(_idx))
	{
		return &sprites[_idx];
	}
	return nullptr;
}

void SpriteGridPalette::set(int _idx, Sprite const* _sprite)
{
	if (sprites.get_size() <= _idx)
	{
		sprites.push_back();
	}

	sprites[_idx].set(_sprite);
	sprites[_idx].cache();
}

void SpriteGridPalette::clear_at_and_in_grid(int _idx)
{
	if (!sprites.is_index_valid(_idx))
	{
		return;
	}

	for_every(c, grid->content.content)
	{
		if (*c == _idx)
		{
			*c = NONE;
		}
	}

	sprites[_idx].clear();
}

int SpriteGridPalette::add(Sprite const* _sprite)
{
	for_every(s, sprites)
	{
		if (s->get() == _sprite)
		{
			return for_everys_index(s);
		}
	}

	for_every(s, sprites)
	{
		if (!s->get())
		{
			s->set(_sprite);
			return for_everys_index(s);
		}
	}

	sprites.push_back();
	sprites.get_last().set(_sprite);
	return sprites.get_size() - 1;
}

void SpriteGridPalette::swap(int _aIdx, int _bIdx)
{
	if (!sprites.is_index_valid(_aIdx) ||
		!sprites.is_index_valid(_bIdx))
	{
		return;
	}

	::swap(sprites[_aIdx], sprites[_bIdx]);

	for_every(c, grid->content.content)
	{
		if (*c == _aIdx) *c = _bIdx; else
		if (*c == _bIdx) *c = _aIdx;
	}
}

void SpriteGridPalette::prune()
{
	Array<int> remap;
	for (int i = 0; i < sprites.get_size(); ++ i)
	{
		if (sprites[i].get())
		{
			remap.push_back(i);
		}
		else
		{
			remap.push_back(NONE);
			sprites.remove_at(i);
			--i;
		}
	}

	for_every(c, grid->content.content)
	{
		*c = remap.is_index_valid(*c) ? remap[*c] : NONE;
	}
}

void SpriteGridPalette::clear_and_clear_grid()
{
	sprites.clear();
	grid->content.clear();
}

void SpriteGridPalette::reset()
{
	sprites.clear();
}

void SpriteGridPalette::remove_unused()
{
	Array<int> usedIndices;
	for_every(c, grid->content.content)
	{
		if (*c != NONE)
		{
			usedIndices.push_back_unique(*c);
		}
	}
	::sort(usedIndices);

	for_count(int, i, sprites.get_size())
	{
		if (!usedIndices.does_contain(i))
		{
			sprites[i].clear();
		}
	}

	prune();
}

void SpriteGridPalette::sort()
{
	struct Element
	{
		int idx;
		Sprite const * sprite;
		bool used = false; // filled later
		Element() {}
		Element(int _idx, Sprite const * _sprite) : idx(_idx), sprite(_sprite) {}

		static Element & add_unique(Array<Element>& _array, int _idx, Sprite const * _sprite)
		{
			for_every(e, _array)
			{
				if (e->idx == _idx)
				{
					an_assert(e->sprite == _sprite);
					return *e;
				}
			}
			_array.push_back(Element(_idx, _sprite));
			return _array.get_last();
		}
		static bool does_contain(Array<Element> const & _array, int _idx)
		{
			for_every(e, _array)
			{
				if (e->idx == _idx)
				{
					return true;
				}
			}
			return false;
		}

		static int compare(void const* _a, void const* _b)
		{
			Element const* a = plain_cast<Element>(_a);
			Element const* b = plain_cast<Element>(_b);
			if (a->used && !b->used) return A_BEFORE_B;
			if (! a->used && b->used) return B_BEFORE_A;
			if (! a->sprite && b->sprite) return A_BEFORE_B;
			if (a->sprite && ! b->sprite) return B_BEFORE_A;
			if (a->sprite && b->sprite)
			{
				return LibraryName::compare(&a->sprite->get_name(), &b->sprite->get_name());
			}
			else
			{
				return A_AS_B;
			}
		}
	};

	Array<Element> elements;

	// add used and not used elements
	{
		for_every(c, grid->content.content)
		{
			if (sprites.is_index_valid(*c))
			{
				Element::add_unique(elements, *c, sprites[*c].get()).used = true;
			}
		}
		for_every_ref(s, sprites)
		{
			int sIdx = for_everys_index(s);
			if (!Element::does_contain(elements, sIdx))
			{
				Element::add_unique(elements, sIdx, s).used = false;				
			}
		}
	}

	::sort(elements);

	Array<int> remap;

	// by default clear all nodes
	remap.set_size(sprites.get_size());
	for_every(r, remap)
	{
		*r = NONE;
	}

	// rebuild sprites and prepare remap
	sprites.clear();
	for_every(e, elements)
	{
		remap[e->idx] = sprites.get_size();
		sprites.push_back();
		sprites.get_last().set(e->sprite);
	}

	// do actual remap
	for_every(c, grid->content.content)
	{
		*c = remap.is_index_valid(*c) ? remap[*c] : NONE;
	}
}

void SpriteGridPalette::fill_with(SpriteGridPalette const& _other)
{
	for_every_ref(s, _other.sprites)
	{
		add(s);
	}
}

bool SpriteGridPalette::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	// clear/reset
	//
	sprites.clear();

	// load
	//
	for_every(node, _node->children_named(TXT("sprite")))
	{
		sprites.push_back();
		auto& s = sprites.get_last();
		s.load_from_xml(node, _lc);
	}

	return result;
}

bool SpriteGridPalette::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	for_every(s, sprites)
	{
		if (auto* node = _node->add_node(TXT("sprite")))
		{
			s->save_to_xml(node);
		}
	}

	return result;
}

bool SpriteGridPalette::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(s, sprites)
	{
		result &= s->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void SpriteGridPalette::cache()
{
	for_every(s, sprites)
	{
		s->cache();
	}
}

//--

void SpriteGridPalette::Entry::cache()
{
}

bool SpriteGridPalette::Entry::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	// reset
	//
	sprite.clear();

	// load
	//
	result &= sprite.load_from_xml(_node, TXT("name"), _lc);

	return result;
}

bool SpriteGridPalette::Entry::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	sprite.save_to_xml(_node, TXT("name"));

	return result;
}

bool SpriteGridPalette::Entry::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= sprite.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		cache();
	}

	return result;
}
