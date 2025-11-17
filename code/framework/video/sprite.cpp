#include "sprite.h"

#include "spriteTextureProcessor.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\other\parserUtils.h"

#include "..\editor\editors\editorSprite.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define RENDER_PASS_NORMAL			0
#define RENDER_PASS_EDITED_LAYER	1
#define RENDER_PASS_COUNT			2

#define CONTENT_ACCESS SpriteContentAccess

//

using namespace Framework;

//

// layer types
DEFINE_STATIC_NAME_STR(layerType_transform, TXT("transform"));
DEFINE_STATIC_NAME_STR(layerType_fullTransform, TXT("fullTransform"));
DEFINE_STATIC_NAME_STR(layerType_include, TXT("include"));

// shader params
DEFINE_STATIC_NAME(inTexture);

//

int pixel_index(VectorInt2 const& _coord, RangeInt2 const& _range)
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

REGISTER_FOR_FAST_CAST(Sprite);
LIBRARY_STORED_DEFINE_TYPE(Sprite, sprite);

::System::VertexFormat* Sprite::s_vertexFormat = nullptr;

void Sprite::initialise_static()
{
	todo_note(TXT("update after reading main settings?"));
	s_vertexFormat = new ::System::VertexFormat();
	s_vertexFormat->with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float).no_padding().calculate_stride_and_offsets();
}

void Sprite::close_static()
{
	delete_and_clear(s_vertexFormat);
}

Sprite::Sprite(Library* _library, LibraryName const& _name)
: base(_library, _name)
{
	resultLayer = new SpriteLayer();

	shaderProgramInstance = new ::System::ShaderProgramInstance();

	an_assert(::System::Video3D::get());
	shaderProgramInstance->set_shader_program(::System::Video3D::get()->get_fallback_shader_program(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::WithTexture | ::System::Video3DFallbackShader::DiscardsBlending));
}

Sprite::~Sprite()
{
	delete_and_clear(shaderProgramInstance);
}

#ifdef AN_DEVELOPMENT
void Sprite::ready_for_reload()
{
	base::ready_for_reload();

	if (auto* sl = fast_cast<SpriteLayer>(resultLayer.get()))
	{
		sl->reset();
	}
}
#endif

void Sprite::prepare_to_unload()
{
	base::prepare_to_unload();

	if (auto* sl = fast_cast<SpriteLayer>(resultLayer.get()))
	{
		sl->reset();
	}
}

bool Sprite::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset
	//
	colourPalette.clear();
	pixelSize = Vector2::one;
	zeroOffset = Vector2::zero;
	spriteGridDrawPriority = 0;
	layers.clear();
	resultLayer->clear();
	editedLayerIndex = NONE;

	// load
	//
	result &= colourPalette.load_from_xml(_node, TXT("colourPalette"), _lc);
	pixelSize.load_from_xml_child_node(_node, TXT("pixelSize"));
	zeroOffset.load_from_xml_child_node(_node, TXT("zeroOffset"));
	for_every(node, _node->children_named(TXT("spriteGrid")))
	{
		spriteGridDrawPriority = node->get_int_attribute(TXT("drawPriority"), spriteGridDrawPriority);
	}

	for_every(node, _node->children_named(TXT("layer")))
	{
		RefCountObjectPtr<ISpriteLayer> layer;
		layer = ISpriteLayer::create_layer_for(node);
		if (layer.get())
		{
			if (layer->load_from_xml(node, _lc))
			{
				layers.push_back(layer);
			}
			else
			{
				error_loading_xml(node, TXT("unable to load sprite layer"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("unable to create a sprite layer when loading"));
			result = false;
		}
	}

	{
		for_every(node, _node->children_named(TXT("editedLayer")))
		{
			editedLayerIndex = node->get_int_attribute(TXT("index"), editedLayerIndex);
		}
	}

	return result;
}

bool Sprite::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= colourPalette.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	for_every_ref(layer, layers)
	{
		result &= layer->prepare_for_game(_library, _pfgContext);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CombineSpriteLayers)
	{
		CONTENT_ACCESS::combine_pixels(this);
		_library->access_sprite_texture_processor().add(this);
	}

	return result;
}

bool Sprite::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	CONTENT_ACCESS::prune(cast_to_nonconst(this));

	colourPalette.save_to_xml(_node, TXT("colourPalette"));

	pixelSize.save_to_xml_child_node(_node, TXT("pixelSize"));
	zeroOffset.save_to_xml_child_node(_node, TXT("zeroOffset"));

	if (auto* node = _node->add_node(TXT("spriteGrid")))
	{
		node->set_int_attribute(TXT("drawPriority"), spriteGridDrawPriority);
	}

	{
		if (editedLayerIndex != NONE)
		{
			auto* n = _node->add_node(TXT("editedLayer"));
			n->set_int_attribute(TXT("index"), editedLayerIndex);
		}
	}

	for_every_ref(layer, layers)
	{
		if (auto* n = ISpriteLayer::create_node_for(_node, layer))
		{
			result &= layer->save_to_xml(n);
		}
	}

	return result;
}

void Sprite::debug_draw() const
{
	CONTENT_ACCESS::RenderContext context;
	CONTENT_ACCESS::render(this, context);
}

bool Sprite::editor_render() const
{
	debug_draw();
	return true;
}

Editor::Base* Sprite::create_editor(Editor::Base* _current) const
{
	if (fast_cast<Editor::Sprite>(_current))
	{
		return _current;
	}
	else
	{
		return new Editor::Sprite();
	}
}

RangeInt2 Sprite::get_range() const
{
	return resultLayer->get_range();
}

VectorInt2 Sprite::to_pixel_coord(Vector2 _coord) const
{
	_coord /= pixelSize;
	_coord += zeroOffset;
	VectorInt2 c = TypeConversions::Normal::f_i_cells(_coord);
	return c;
}

Vector2 Sprite::to_full_coord(VectorInt2 _coord, Optional<Vector2> const& _insideVoxel) const
{
	Vector2 c = _coord.to_vector2();
	c += _insideVoxel.get(Vector2::half);
	c -= zeroOffset;
	c *= pixelSize;
	return c;
}

RangeInt2 Sprite::to_pixel_range(Range2 const& _range, bool _fullyInlcuded) const
{
	Vector2 rmin = _range.bottom_left();
	Vector2 rmax = _range.top_right();
	rmin /= pixelSize;
	rmax /= pixelSize;
	rmin += zeroOffset;
	rmax += zeroOffset;
	RangeInt2 result = RangeInt2::empty;
	float const extra = 0.001f;
	if (_fullyInlcuded)
	{
		result.x.min = TypeConversions::Normal::f_i_cells(rmin.x - extra) + 1;
		result.y.min = TypeConversions::Normal::f_i_cells(rmin.y - extra) + 1;
		result.x.max = TypeConversions::Normal::f_i_cells(rmax.x + extra) - 1;
		result.y.max = TypeConversions::Normal::f_i_cells(rmax.y + extra) - 1;
	}
	else
	{
		result.x.min = TypeConversions::Normal::f_i_cells(rmin.x + extra);
		result.y.min = TypeConversions::Normal::f_i_cells(rmin.y + extra);
		result.x.max = TypeConversions::Normal::f_i_cells(rmax.x - extra);
		result.y.max = TypeConversions::Normal::f_i_cells(rmax.y - extra);
	}

	return result;
}

Range2 Sprite::to_full_range(RangeInt2 const& _range) const
{
	if (_range.is_empty())
	{
		return Range2::empty;
	}
	Vector2 rmin = _range.bottom_left().to_vector2();
	Vector2 rmax = (_range.top_right() + VectorInt2::one).to_vector2();
	rmin -= zeroOffset;
	rmax -= zeroOffset;
	rmin *= pixelSize;
	rmax *= pixelSize;
	Range2 result = Range2::empty;
	result.include(rmin);
	result.include(rmax);
	return result;
}

void Sprite::prepare_to_draw_at(OUT_ SpriteDrawingData& _data, VectorInt2 const& _at, SpriteDrawingContext const& _context) const
{
	_data.texture = textureUsage.texture.get();
	if (!_data.texture)
	{
		return;
	}

	Vector2 at = _at.to_vector2();
	Vector2 size = textureUsage.size.to_vector2();
	Vector2 scale = _context.scale.to_vector2();
	if (_context.stretchTo.is_set())
	{
		scale = _context.stretchTo.get().to_vector2() / size;
		if (_context.stretchTo.get().x == 0)
		{
			scale.x = 1.0f;
		}
		if (_context.stretchTo.get().y == 0)
		{
			scale.y = 1.0f;
		}
	}
	Vector2 uvLB = textureUsage.uvRange.bottom_left();
	Vector2 uvRT = textureUsage.uvRange.top_right();

	{
		Vector2 lb = textureUsage.leftBottomOffset.to_vector2();
		Vector2 rt = lb + size;
		/*
			cases:

			size = 3	lb = -1
				|   |   |   |					|   |   |   |
				|-1 | 0 |+1 |					|-1 | 0 |+1 |
				|   |   |   |					|   |   |   |
					^								  ^
			|   |   |   |						|   |   |   |
			|+1 | 0 |-1 |						|+1 | 0 |-1 |
			|   |   |   |						|   |   |   |
			lb = -2								lb = -1

			size = 4	lb = -2
			|   |   |   |   |				|   |   |   |   |
			|-2 |-1 | 0 |+1 |				|-2 |-1 | 0 |+1 |
			|   |   |   |   |				|   |   |   |   |
					^								  ^
			|   |   |   |   |					|   |   |   |   |
			|+1 | 0 |-1 |-2 |					|+1 | 0 |-1 |-2 |
			|   |   |   |   |					|   |   |   |   |
			lb = -2								lb = -1
		 */
		for_count(int, e, 2)
		{
			float zeroOffsetE = zeroOffset.get_element(e);
			float scaleE = scale.get_element(e);
			float rtE = rt.access_element(e);
			float pivot = zeroOffsetE * abs(scaleE);
			float& lbE = lb.access_element(e);
			float& sizeE = size.access_element(e);
			if (scaleE >= 0.0f)
			{
				lbE = pivot + (lbE - pivot) * scaleE;
				lbE -= 0.0001f;
				lbE = round(lbE);
				sizeE *= scaleE;
			}
			else
			{
				swap(uvLB.access_element(e), uvRT.access_element(e));

				lbE = pivot - (rtE - pivot) * (-scaleE);
				lbE -= 0.0001f;
				lbE = round(lbE);
				sizeE *= -scaleE;
			}
		}

		at += lb;
	}

	Vector2 lbl = at;
	Vector2 ltr = at + size;

	Vector2 uvSC = uvLB;
	Vector2 uvUp = Vector2(0.0f, uvRT.y - uvLB.y);
	Vector2 uvRt = Vector2(uvRT.x - uvLB.x, 0.0f);

	_data.vertices.clear();
	_data.vertices.push_back(SpriteVertex(uvSC,					Vector3(lbl.x, lbl.y, 0.0f)));
	_data.vertices.push_back(SpriteVertex(uvSC + uvUp,			Vector3(lbl.x, ltr.y, 0.0f)));
	_data.vertices.push_back(SpriteVertex(uvSC + uvRt + uvUp,	Vector3(ltr.x, ltr.y, 0.0f)));
	_data.vertices.push_back(SpriteVertex(uvSC + uvRt,			Vector3(ltr.x, lbl.y, 0.0f)));
}

void Sprite::draw_at(::System::Video3D* _v3d, VectorInt2 const& _at, SpriteDrawingContext const& _context) const
{
	SpriteDrawingData data;
	prepare_to_draw_at(OUT_ data, _at, _context);
	
	if (!data.texture)
	{
		return;
	}


	// actual rendering

	draw(_v3d, data);
}

void Sprite::draw_stretch_at(::System::Video3D* _v3d, Range2 const & _r, bool _maintainAspectRatio) const
{
	SpriteDrawingData data;
	data.texture = textureUsage.texture.get();
	if (!data.texture)
	{
		return;
	}
	Vector2 lb = _r.bottom_left();
	Vector2 rt = _r.top_right();

	if (_maintainAspectRatio)
	{
		Vector2 renderSize = get_range().length().to_vector2();
		Vector2 availableSize = _r.length();

		renderSize *= availableSize.y / renderSize.y;
		if (renderSize.x > availableSize.x)
		{
			renderSize *= availableSize.x / renderSize.x;
		}

		Vector2 c = (lb + rt) * 0.5f;
		Vector2 rsh = renderSize * 0.5f;
		lb = c - rsh;
		rt = c + rsh;
	}
		
	Vector2 uvLB = textureUsage.uvRange.bottom_left();
	Vector2 uvRT = textureUsage.uvRange.top_right();

	data.vertices.push_back(SpriteVertex(Vector2(uvLB.x, uvLB.y),	Vector3(lb.x, lb.y, 0.0f)));
	data.vertices.push_back(SpriteVertex(Vector2(uvLB.x, uvRT.y),	Vector3(lb.x, rt.y, 0.0f)));
	data.vertices.push_back(SpriteVertex(Vector2(uvRT.x, uvRT.y),	Vector3(rt.x, rt.y, 0.0f)));
	data.vertices.push_back(SpriteVertex(Vector2(uvRT.x, uvLB.y),	Vector3(rt.x, lb.y, 0.0f)));


	// actual rendering

	draw(_v3d, data);
}

void Sprite::draw(::System::Video3D* _v3d, SpriteDrawingData const& data) const
{
	{
		_v3d->set_fallback_colour(Colour::white);
		{
			auto tId = data.texture->get_texture()->get_texture_id();
			_v3d->set_fallback_texture(tId);
			shaderProgramInstance->set_uniform(NAME(inTexture), tId);
		}

		Meshes::Mesh3D::render_data(_v3d, shaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext::none(), data.vertices.get_data(), ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat);

		//-

		_v3d->set_fallback_colour();
		_v3d->set_fallback_texture();
	}
}

//--

REGISTER_FOR_FAST_CAST(ISpriteLayer);

Optional<int> ISpriteLayer::get_child_depth(ISpriteLayer const* _for, Array<RefCountObjectPtr<ISpriteLayer>> const& _layers)
{
	auto iL = _layers.begin();
	int iLIdx = 0;
	if (_for)
	{
		for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
		{
			if (iL->get() == _for)
			{
				++iLIdx;
				++iL;
				break;
			}
		}
	}

	Optional<int> forDepth;
	if (_for)
	{
		forDepth = _for->depth;
	}

	Optional<int> childDepth;
	for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
	{
		if (forDepth.is_set() && iL->get()->depth <= forDepth.get())
		{
			break;
		}
		int lDepth = iL->get()->depth;
		childDepth = min(lDepth, childDepth.get(lDepth));
	}

	return childDepth;
}

bool ISpriteLayer::for_every_child_combining(ISpriteLayer const* _of, Array<RefCountObjectPtr<ISpriteLayer>> const& _layers, std::function<bool(ISpriteLayer* layer)> _do)
{
	bool result = true;

	Optional<int> childDepth = get_child_depth(_of, _layers);

	if (childDepth.is_set())
	{
		auto iL = _layers.begin();
		int iLIdx = 0;
		if (_of)
		{
			for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
			{
				if (iL->get() == _of)
				{
					++iLIdx;
					++iL;
					break;
				}
			}
		}

		RangeInt childrenRange = RangeInt::empty;
		for (; iLIdx < _layers.get_size(); ++iLIdx, ++iL)
		{
			if (iL->get()->depth < childDepth.get())
			{
				break;
			}
			if (iL->get()->depth == childDepth.get())
			{
				childrenRange.include(iLIdx);
			}
		}

		if (!childrenRange.is_empty())
		{
			for_range_reverse(int, iLIdx, childrenRange.max, childrenRange.min)
			{
				if (auto* l = _layers[iLIdx].get())
				{
					if (l->depth == childDepth.get())
					{
						result &= _do(l);
					}
				}
			}
		}
	}

	return result;
}

ISpriteLayer::ISpriteLayer()
{
}

ISpriteLayer::~ISpriteLayer()
{
}

ISpriteLayer* ISpriteLayer::create_layer_for(IO::XML::Node const* _node)
{
	Name type = _node->get_name_attribute(TXT("type"));
	if (type == NAME(layerType_transform))
	{
		return new SpriteTransformLayer();
	}
	if (type == NAME(layerType_fullTransform))
	{
		return new SpriteFullTransformLayer();
	}
	if (type == NAME(layerType_include))
	{
		return new IncludeSpriteLayer();
	}
	return new SpriteLayer();
}

IO::XML::Node* ISpriteLayer::create_node_for(IO::XML::Node* _parentNode, ISpriteLayer const* _layer)
{
	auto* n = _parentNode->add_node(TXT("layer"));

	if (fast_cast<SpriteTransformLayer>(_layer))
	{
		n->set_attribute(TXT("type"), NAME(layerType_transform).to_char());
	}
	if (fast_cast<SpriteFullTransformLayer>(_layer))
	{
		n->set_attribute(TXT("type"), NAME(layerType_fullTransform).to_char());
	}
	if (fast_cast<IncludeSpriteLayer>(_layer))
	{
		n->set_attribute(TXT("type"), NAME(layerType_include).to_char());
	}

	return n;
}

bool ISpriteLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	// clear/reset
	//
	id = Name::invalid();
	depth = 0;
	visible = true;
	visibilityOnly = false;

	// load
	//
	id = _node->get_name_attribute(TXT("id"), id);
	depth = _node->get_int_attribute(TXT("depth"), depth);
	visible = _node->get_bool_attribute(TXT("visible"), visible);
	visibilityOnly = _node->get_bool_attribute(TXT("visibilityOnly"), visibilityOnly);

	return result;
}

bool ISpriteLayer::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

bool ISpriteLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	if (id.is_valid())
	{
		_node->set_attribute(TXT("id"), id.to_char());
	}
	_node->set_int_attribute(TXT("depth"), depth);
	_node->set_bool_attribute(TXT("visible"), visible);
	_node->set_bool_attribute(TXT("visibilityOnly"), visibilityOnly);

	return result;
}

bool ISpriteLayer::combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const
{
	bool result = true;
	
	bool isVisibleOrVisibilityOnly = is_visible(_for);
	if (!isVisibleOrVisibilityOnly)
	{
		for_every_ref(l, _allLayers)
		{
			if (l->get_visibility_only())
			{
				isVisibleOrVisibilityOnly = true;
				break;
			}
		}
	}
	if (isVisibleOrVisibilityOnly)
	{
		result &= ISpriteLayer::for_every_child_combining(this, _allLayers, [_for, _resultLayer, &_allLayers, &_context](ISpriteLayer* _layer)
			{
				return _layer->combine(_for, _resultLayer, _allLayers, _context);
			});
	}

	return true;
}

void ISpriteLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	an_assert(_moreVoxels.is_set() ^ _lessVoxels.is_set(), TXT("one has to be set and only one"));
}

bool ISpriteLayer::is_visible(Sprite* _partOf) const
{
	if (visibilityOnly)
	{
		return true;
	}
	else if (fast_cast<SpriteTransformLayer>(this) == nullptr)
	{
		for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
		{
			if (l->visibilityOnly)
			{
				return false;
			}
		}
	}
	else if (fast_cast<SpriteFullTransformLayer>(this) == nullptr)
	{
		for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
		{
			if (l->visibilityOnly)
			{
				return false;
			}
		}
	}
	if (visible)
	{
		if (auto* parent = get_parent(_partOf))
		{
			return parent->is_visible(_partOf);
		}
	}
	return visible;
}

void ISpriteLayer::set_visibility_only(bool _only, Sprite* _partOf)
{
	if (_only)
	{
		an_assert(_partOf, TXT("requires _partOf parameter to disable in other layers"));
		for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
		{
			l->visibilityOnly = false;
		}
	}
	visibilityOnly = _only;
}

bool ISpriteLayer::is_parent_of(Sprite* _partOf, ISpriteLayer const* _of) const
{
	while (_of)
	{
		if (_of == this)
		{
			return true;
		}
		_of = _of->get_parent(_partOf);
	}
	return false;
}

ISpriteLayer* ISpriteLayer::get_parent(Sprite* _partOf) const
{
	int ourDepth = get_depth();
	ISpriteLayer* lastLessShallow = nullptr;
	for_every_ref(l, CONTENT_ACCESS::get_layers(_partOf))
	{
		if (l == this)
		{
			return lastLessShallow;
		}
		if (l->get_depth() < ourDepth)
		{
			lastLessShallow = l;
		}
	}
	return nullptr;
}

ISpriteLayer* ISpriteLayer::get_top_parent(Sprite* _partOf) const
{
	ISpriteLayer* parent = cast_to_nonconst(this);
	ISpriteLayer* goUp = parent;
	while (goUp)
	{
		parent = goUp;
		goUp = goUp->get_parent(_partOf);
	}
	return parent;
}

VectorInt2 ISpriteLayer::to_local(Sprite* _partOf, VectorInt2 const& _world, bool _ignoreMirrors) const
{
	if (auto* parent = get_parent(_partOf))
	{
		return parent->to_local(_partOf, _world, _ignoreMirrors);
	}
	return _world;
}

VectorInt2 ISpriteLayer::to_world(Sprite* _partOf, VectorInt2 const& _local, bool _ignoreMirrors) const
{
	if (auto* parent = get_parent(_partOf))
	{
		return parent->to_world(_partOf, _local, _ignoreMirrors);
	}
	return _local;
}

ISpriteLayer* ISpriteLayer::create_copy() const
{
	ISpriteLayer* copy = create_for_copy();
	an_assert(copy);
	fill_copy(copy);
	return copy;
}

void ISpriteLayer::fill_copy_basics(ISpriteLayer* _copy) const
{
	_copy->id = id;
	_copy->depth = depth;
	_copy->visible = visible;
}

void ISpriteLayer::fill_copy(ISpriteLayer* _copy) const
{
	fill_copy_basics(_copy);
}

//--

REGISTER_FOR_FAST_CAST(SpriteLayer);

SpriteLayer::SpriteLayer()
{
}

SpriteLayer::~SpriteLayer()
{
}

bool SpriteLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset
	//
	range = RangeInt2::empty;
	pixels.clear();

	// load
	//
	range.load_from_xml_child_node(_node, TXT("range"));

	pixels.make_space_for(range.x.length() * range.y.length());

	for_every(node, _node->children_named(TXT("pixelsRaw")))
	{
		int chPerPixel = 1;
		chPerPixel = node->get_int_attribute(TXT("chPerVoxel"), chPerPixel);
		chPerPixel = node->get_int_attribute(TXT("chPerPixel"), chPerPixel);
		int pixelOff = node->get_int_attribute(TXT("pixelOff"), 1);
		String content = node->get_text();
		String pixelsContent;
		int readCh = 0;
		for_every(ch, content.get_data())
		{
			if (*ch == 0 || readCh >= chPerPixel)
			{
				pixels.push_back();
				auto& v = pixels.get_last();
				v.pixel = ParserUtils::parse_hex(pixelsContent.to_char()) - pixelOff;
				pixelsContent = String::empty();
				readCh = 0;
				if (*ch == 0)
				{
					break;
				}
			}
			{
				pixelsContent += *ch;
				++readCh;
			}
		}
		an_assert(pixelsContent.is_empty());
	}


	return result;
}

bool SpriteLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	range.save_to_xml_child_node(_node, TXT("range"));

	{
		int storedInNode = 0;
		int storedInNodeLimit = 256;
		int lastPixelIdx = pixels.get_size() - 1;
		struct PixelsToSave
		{
			Array<SpritePixel> pixels;
			String pixelsContent;
			String pixelFormat;
			bool save_to_xml(IO::XML::Node* _node)
			{
				if (pixels.is_empty())
				{
					return true;
				}
				int pixelOff = 1;
				for_every(v, pixels)
				{
					pixelOff = max(pixelOff, -v->pixel);
				}
				int maxValue = 0;
				for_every(v, pixels)
				{
					maxValue = max(maxValue, v->pixel + pixelOff);
				}
				String maxValueAsHex = String::printf(TXT("%x"), maxValue);
				int chPerPixel = maxValueAsHex.get_length();

				auto* pixelsRawNode = _node->add_node(TXT("pixelsRaw"));
				pixelsRawNode->set_int_attribute(TXT("chPerPixel"), chPerPixel);
				pixelsRawNode->set_int_attribute(TXT("pixelOff"), pixelOff);

				pixelsContent = String::empty();
				pixelFormat = String::printf(TXT("%%0%ix"), chPerPixel);
				for_every(v, pixels)
				{
					pixelsContent += String::printf(pixelFormat.to_char(), v->pixel + pixelOff); // offset, see above
				}
				pixelsRawNode->add_text(pixelsContent);

				clear();

				return true;
			}
			void clear()
			{
				pixels.clear();
				pixelsContent = String::empty();
			}
		} pixelsToSave;
		for_every(v, pixels)
		{
			pixelsToSave.pixels.push_back(*v);
			++storedInNode;
			if (storedInNode >= storedInNodeLimit ||
				for_everys_index(v) == lastPixelIdx)
			{
				result &= pixelsToSave.save_to_xml(_node);
				storedInNode = 0;
			}
		}
		result &= pixelsToSave.save_to_xml(_node);

		an_assert(storedInNode == 0);
	}

	return result;
}

void SpriteLayer::reset()
{
	depth = 0;
	clear();
}

void SpriteLayer::clear()
{
	range = RangeInt2::empty;
	pixels.clear();
}

void SpriteLayer::prune()
{
	RangeInt2 occupiedRange = RangeInt2::empty;

	for_range(int, y, range.y.min, range.y.max)
	{
		VectorInt2 startAt(range.x.min, y);
		SpritePixel const* v = &pixels[pixel_index(startAt, range)];
		for_range(int, x, range.x.min, range.x.max)
		{
			if (SpritePixel::is_anything(v->pixel))
			{
				occupiedRange.include(VectorInt2(x, y));
			}
			++v;
		}
	}

	if (occupiedRange != range)
	{
		Array<SpritePixel> oldPixels = pixels;
		RangeInt2 oldRange = range;

		range = occupiedRange;

		pixels.set_size(range.x.length() * range.y.length());
		for_every(v, pixels)
		{
			// clear
			*v = SpritePixel();
		}

		if (!oldPixels.is_empty() && !oldRange.is_empty())
		{
			copy_data_from(oldRange, oldPixels);
		}
	}
}

SpritePixel& SpriteLayer::access_at(VectorInt2 const& _at)
{
	if (!range.does_contain(_at))
	{
		Array<SpritePixel> oldPixels = pixels;
		RangeInt2 oldRange = range;

		range.include(_at);

		pixels.set_size(range.x.length() * range.y.length());
		for_every(v, pixels)
		{
			// clear
			*v = SpritePixel();
		}

		if (!oldPixels.is_empty() && !oldRange.is_empty())
		{
			copy_data_from(oldRange, oldPixels);
		}
	}

	return pixels[pixel_index(_at, range)];
}

SpritePixel const * SpriteLayer::get_at(VectorInt2 const& _at) const
{
	if (!range.does_contain(_at))
	{
		return nullptr;
	}

	return &pixels[pixel_index(_at, range)];
}

void SpriteLayer::fill_at(Sprite* _for, SpriteLayer const* _combinedResultLayer, VectorInt2 const& _worldAt, std::function<bool(SpritePixel& _pixel)> _doForPixel)
{
	RangeInt2 r = CONTENT_ACCESS::get_combined_range(_for);

	struct FilledVoxel
	{
		VectorInt2 worldAt;
		bool processed = false;
		Optional<SpritePixel> pixel;
		FilledVoxel() {}
		explicit FilledVoxel(VectorInt2 const& _worldAt) : worldAt(_worldAt) {}
		static bool add(REF_ Array<FilledVoxel>& _filledVoxels, VectorInt2 const& _worldAt)
		{
			for_every(fv, _filledVoxels)
			{
				if (fv->worldAt == _worldAt)
				{
					return false;
				}
			}
			_filledVoxels.push_back(FilledVoxel(_worldAt));
			return true;
		}
	};
	Array<FilledVoxel> filledVoxels;
	FilledVoxel::add(REF_ filledVoxels, _worldAt);
	int i = 0;
	bool validFill = true;
	while (filledVoxels.is_empty() || i < filledVoxels.get_size())
	{
		auto& fv = filledVoxels[i];
		if (! fv.processed)
		{
			VectorInt2 fillAt = fv.worldAt;
			bool filled = false;
			SpritePixel v;
			if (auto* cv = _combinedResultLayer->get_at(fillAt))
			{
				v = *cv;
			}
			if (_doForPixel(v))
			{
				filled = true;
				fv.pixel = v;
			}
			fv.processed = true;
			if (filled)
			{
				if (!r.does_contain(fillAt))
				{
					validFill = false;
					break;
				}
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt2( 1,  0));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt2(-1,  0));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt2( 0,  1));
				FilledVoxel::add(REF_ filledVoxels, fillAt + VectorInt2( 0, -1));
			}
		}
		++i;
	}
	if (validFill)
	{
		for_every(fv, filledVoxels)
		{
			if (fv->processed && fv->pixel.is_set())
			{
				VectorInt2 local = to_local(_for, fv->worldAt);
				access_at(local) = fv->pixel.get();
			}
		}
	}
}

void SpriteLayer::copy_data_from(RangeInt2 const& _otherRange, Array<SpritePixel> const& _otherPixels)
{
	// copy old pixels
	RangeInt2 commonRange = range;
	commonRange.intersect_with(_otherRange);
	for_range(int, y, commonRange.y.min, commonRange.y.max)
	{
		VectorInt2 startAt(commonRange.x.min, y);
		SpritePixel const* src = &_otherPixels[pixel_index(startAt, _otherRange)];
		SpritePixel* dst = &pixels[pixel_index(startAt, range)];
		for_range(int, x, commonRange.x.min, commonRange.x.max)
		{
			*dst = *src;
			++dst;
			++src;
		}
	}
}

void SpriteLayer::combine_data_from(RangeInt2 const& _otherRange, Array<SpritePixel> const& _otherPixels)
{
	// copy old pixels
	RangeInt2 commonRange = range;
	commonRange.intersect_with(_otherRange);
	for_range(int, y, commonRange.y.min, commonRange.y.max)
	{
		VectorInt2 startAt(commonRange.x.min, y);
		SpritePixel const* src = &_otherPixels[pixel_index(startAt, _otherRange)];
		SpritePixel* dst = &pixels[pixel_index(startAt, range)];
		for_range(int, x, commonRange.x.min, commonRange.x.max)
		{
			if (SpritePixel::is_anything(src->pixel) &&
				(!SpritePixel::is_anything(dst->pixel) || src->pixel != SpritePixel::Invisible))
			{
				*dst = *src;
			}
			++dst;
			++src;
		}
	}
}

void SpriteLayer::merge_data_from(RangeInt2 const& _otherRange, Array<SpritePixel> const& _otherPixels)
{
	// copy old pixels
	RangeInt2 commonRange = range;
	commonRange.intersect_with(_otherRange);
	for_range(int, y, commonRange.y.min, commonRange.y.max)
	{
		VectorInt2 startAt(commonRange.x.min, y);
		SpritePixel const* src = &_otherPixels[pixel_index(startAt, _otherRange)];
		SpritePixel* dst = &pixels[pixel_index(startAt, range)];
		for_range(int, x, commonRange.x.min, commonRange.x.max)
		{
			if (SpritePixel::is_anything(src->pixel) &&
				SpritePixel::can_be_replaced_in_merge(dst->pixel))
			{
				*dst = *src;
			}
			++dst;
			++src;
		}
	}
}

bool SpriteLayer::combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const
{
	bool result = base::combine(_for, _resultLayer, _allLayers, _context);

	if (is_visible(_for) &&
		!range.is_empty())
	{
		// make space
		_resultLayer->access_at(range.bottom_left());
		_resultLayer->access_at(range.top_right());

		_resultLayer->combine_data_from(range, pixels);
	}

	return result;
}

void SpriteLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	base::rescale(_moreVoxels, _lessVoxels);

	if (_moreVoxels.is_set())
	{
		int scale = _moreVoxels.get();
		auto oldRange = range;
		auto oldPixels = pixels;

		clear();

		for_range(int, y, oldRange.y.min, oldRange.y.max)
		{
			for_range(int, x, oldRange.x.min, oldRange.x.max)
			{
				VectorInt2 srcXY(x, y);
				VectorInt2 dstXY = srcXY * scale;

				SpritePixel srcV = oldPixels[pixel_index(srcXY, oldRange)];
				VectorInt2 xy = dstXY;
				for_range(int, sy, 0, scale - 1)
				{
					xy.y = dstXY.y + sy;
					for_range(int, sx, 0, scale - 1)
					{
						xy.x = dstXY.x + sx;
						access_at(xy) = srcV;
					}
				}
			}
		}
	}

	if (_lessVoxels.is_set())
	{
		int scale = _lessVoxels.get();
		auto oldRange = range;
		auto oldPixels = pixels;

		RangeInt2 newRange;
		newRange.x.min = floor_to(range.x.min, scale) / scale;
		newRange.y.min = floor_to(range.y.min, scale) / scale;
		newRange.x.max = ceil_to(range.x.max, scale) / scale;
		newRange.y.max = ceil_to(range.y.max, scale) / scale;
		
		clear();

		struct MostCommon
		{
			struct CountedVoxel
			{
				SpritePixel pixel;
				int count = 0;
			};
			Array<CountedVoxel> countedVoxels;

			void reset()
			{
				countedVoxels.clear();
			}

			void add(SpritePixel const& v)
			{
				for_every(c, countedVoxels)
				{
					if (c->pixel == v)
					{
						++c->count;
						return;
					}
				}
				countedVoxels.push_back();
				auto& c = countedVoxels.get_last();
				c.pixel = v;
				c.count = 1;
			}

			SpritePixel get_most_common()
			{
				SpritePixel result;
				int best = NONE;
				for_every(c, countedVoxels)
				{
					if (c->count > best)
					{
						best = c->count;
						result = c->pixel;
					}
				}
				return result;
			}
		} mostCommon;
		for_range(int, y, newRange.y.min, newRange.y.max)
		{
			for_range(int, x, newRange.x.min, newRange.x.max)
			{
				VectorInt2 dstXY(x, y);
				VectorInt2 srcXY = dstXY * scale;

				mostCommon.reset();
				VectorInt2 xy = srcXY;
				for_range(int, sy, 0, scale - 1)
				{
					xy.y = srcXY.y + sy;
					for_range(int, sx, 0, scale - 1)
					{
						xy.x = srcXY.x + sx;
						if (oldRange.does_contain(xy))
						{
							SpritePixel srcV = oldPixels[pixel_index(xy, oldRange)];
							if (SpritePixel::is_anything(srcV.pixel))
							{
								mostCommon.add(srcV);
							}
						}
					}
				}

				access_at(dstXY) = mostCommon.get_most_common();
			}
		}
	}
}

void SpriteLayer::merge(SpriteLayer const* _layer)
{
	if (!_layer->range.is_empty())
	{
		access_at(_layer->range.bottom_left());
		access_at(_layer->range.top_right());

		merge_data_from(_layer->range, _layer->pixels);
	}
}

void SpriteLayer::offset_placement(VectorInt2 const& _by)
{
	if (!range.is_empty())
	{
		range.offset(_by);
	}
}

ISpriteLayer* SpriteLayer::create_for_copy() const
{
	return new SpriteLayer();
}

void SpriteLayer::fill_copy(ISpriteLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<SpriteLayer>(_copy);
	an_assert(copy);
	copy->range = range;
	copy->pixels = pixels;
}

//--

REGISTER_FOR_FAST_CAST(SpriteTransformLayer);

SpriteTransformLayer::SpriteTransformLayer()
{
	tempALayer = new SpriteLayer();
	tempBLayer = new SpriteLayer();
}

SpriteTransformLayer::~SpriteTransformLayer()
{
}

bool SpriteTransformLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	rotate = Rotate();
	mirror = Mirror();
	offset = VectorInt2::zero;

	// load
	//
	result &= rotate.load_from_xml_child_node(_node, TXT("rotate"));
	result &= mirror.load_from_xml_child_node(_node, TXT("mirror"));
	result &= offset.load_from_xml_child_node(_node, TXT("offset"));

	return result;
}

bool SpriteTransformLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	result &= rotate.save_to_xml_child_node(_node, TXT("rotate"));
	result &= mirror.save_to_xml_child_node(_node, TXT("mirror"));
	result &= offset.save_to_xml_child_node(_node, TXT("offset"));

	return result;
}

bool SpriteTransformLayer::combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const
{
	if (!is_visible(_for))
	{
		return base::combine(_for, _resultLayer, _allLayers, _context);
	}

	tempALayer->clear();
	tempBLayer->clear();

	auto* dst = tempALayer.get();
	auto* src = tempBLayer.get();
	bool result = true;

	// read children result
	{
		result &= base::combine(_for, dst, _allLayers, _context);
		// swap and prune
		swap(src, dst);	dst->clear();
		src->prune();
	}

	// actual transforms
	{
		// rotate
		{
			// build axes from rotation
			VectorInt2 xAxis, yAxis;
			rotate.prepare_axes(OUT_ xAxis, OUT_ yAxis);
			// read all pixels and put them into dst
			{
				RangeInt2 srcRange = src->get_range();
				dst->clear();
				if (!srcRange.is_empty())
				{
					VectorInt2 bl = srcRange.bottom_left();
					VectorInt2 tr = srcRange.top_right();
					dst->access_at(xAxis * bl.x + yAxis * bl.y);
					dst->access_at(xAxis * tr.x + yAxis * tr.y);
					for_range(int, y, srcRange.y.min, srcRange.y.max)
					{
						SpritePixel const * srcV = &src->pixels[pixel_index(VectorInt2(srcRange.x.min, y), srcRange)];
						VectorInt2 dstY = yAxis * y;
						for_range(int, x, srcRange.x.min, srcRange.x.max)
						{
							VectorInt2 dstXY = dstY + xAxis * x;
							dst->access_at(dstXY) = *srcV;
							++srcV;
						}
					}
				}
			}
			// swap and prune
			swap(src, dst);	dst->clear();
			src->prune();
		}

		// mirror
		{
			// for each active mirror have axes (mirror axis + two other)...
			// ...read all required pixels, flip them as required and put them into dst
			if (mirror.mirrorAxis != NONE)
			{
				RangeInt2 srcRange = src->get_range();
				if (!srcRange.is_empty())
				{
					struct MirrorWorker
					{
						int at;
						bool axis = false;
						int dir = 0;

						// A,B = other two axes
						uint elementMirror;
						uint elementA;

						VectorInt2 axisMirror;
						VectorInt2 axisA;

						RangeInt rangeMirror; // result (!)
						RangeInt rangeA;
					}
					mirrorWorker;
					mirrorWorker.at = mirror.at;
					mirrorWorker.axis = mirror.axis;
					mirrorWorker.dir = mirror.dir;

					if (mirror.mirrorAxis == 0)
					{
						mirrorWorker.elementMirror = 0;
						mirrorWorker.elementA = 1;
					}
					else if (mirror.mirrorAxis == 1)
					{
						mirrorWorker.elementMirror = 1;
						mirrorWorker.elementA = 0;
					}
					else 
					{
						an_assert(mirror.mirrorAxis == 2);
						mirrorWorker.elementMirror = 2;
						mirrorWorker.elementA = 0;
					}

					mirrorWorker.axisMirror = VectorInt2::zero;
					mirrorWorker.axisA = VectorInt2::zero;

					mirrorWorker.axisMirror.access_element(mirrorWorker.elementMirror) = 1;
					mirrorWorker.axisA.access_element(mirrorWorker.elementA) = 1;


					mirrorWorker.rangeA = srcRange.access_element(mirrorWorker.elementA);

					RangeInt srcRangeMirror = srcRange.access_element(mirrorWorker.elementMirror);
					if (mirrorWorker.axis)
					{
						if (mirrorWorker.dir == 0)
						{
							// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
							mirrorWorker.rangeMirror.min = mirrorWorker.at - (srcRangeMirror.max - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = mirrorWorker.at + (mirrorWorker.at - srcRangeMirror.min);
						}
						else if (mirrorWorker.dir < 0)
						{
							// at = 2	0123 -> 01:2:3 -> 1:2:3 -> 123
							mirrorWorker.rangeMirror.min = mirrorWorker.at - (srcRangeMirror.max - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = srcRangeMirror.max; // keep it
						}
						else
						{
							// at = 2	0123 -> 01:2:3 -> 01:2:34 -> 01234
							mirrorWorker.rangeMirror.min = srcRangeMirror.min; // keep it
							mirrorWorker.rangeMirror.max = mirrorWorker.at + (mirrorWorker.at - srcRangeMirror.min);
						}
					}
					else
					{
						if (mirrorWorker.dir == 0)
						{
							// at = 3	0123 -> 012:3 -> 2:345 -> 2345
							mirrorWorker.rangeMirror.min = mirrorWorker.at - ((srcRangeMirror.max + 1) - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = mirrorWorker.at + ((mirrorWorker.at - srcRangeMirror.min) - 1);
						}
						else if (mirrorWorker.dir < 0)
						{
							// at = 3	01234 -> 012:34 -> 12:34 -> 1234
							mirrorWorker.rangeMirror.min = mirrorWorker.at - ((srcRangeMirror.max + 1) - mirrorWorker.at);
							mirrorWorker.rangeMirror.max = srcRangeMirror.max; // keep it
						}
						else
						{
							// at = 3	01234 -> 012:34 -> 012:345 -> 012345
							mirrorWorker.rangeMirror.min = srcRangeMirror.min; // keep it
							mirrorWorker.rangeMirror.max = mirrorWorker.at + ((mirrorWorker.at - srcRangeMirror.min) - 1);
						}
					}

					for_range(int, a, mirrorWorker.rangeA.min, mirrorWorker.rangeA.max)
					{
						VectorInt2 coordAB = VectorInt2::zero;
						coordAB.access_element(mirrorWorker.elementA) = a;
						for_range(int, dm, mirrorWorker.rangeMirror.min, mirrorWorker.rangeMirror.max)
						{
							VectorInt2 dstABM = coordAB;
							dstABM.access_element(mirrorWorker.elementMirror) = dm;
							VectorInt2 srcABM = coordAB;
							{
								int sm = dm;
								if (mirrorWorker.axis)
								{
									if (mirrorWorker.dir == 0)
									{
										// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
										sm = mirrorWorker.at - (sm - mirrorWorker.at);
									}
									else if (mirrorWorker.dir < 0)
									{
										// at = 2	0123 -> 01:2:3 -> 1:2:3 -> 123
										if (sm < mirrorWorker.at)
										{
											sm = mirrorWorker.at - (sm - mirrorWorker.at);
										}
										// otherwise keep it
									}
									else
									{
										// at = 2	0123 -> 01:2:3 -> 01:2:34 -> 01234
										if (sm > mirrorWorker.at)
										{
											sm = mirrorWorker.at - (sm - mirrorWorker.at);
										}
										// otherwise keep it
									}
								}
								else
								{
									if (mirrorWorker.dir == 0)
									{
										// at = 3	0123 -> 012:3 -> 2:345 -> 2345
										sm = mirrorWorker.at - (sm - mirrorWorker.at) - 1;
									}
									else if (mirrorWorker.dir < 0)
									{
										// at = 3	01234 -> 012:34 -> 12:34 -> 1234
										if (sm < mirrorWorker.at)
										{
											sm = mirrorWorker.at - (sm - mirrorWorker.at) - 1;
										}
										// otherwise keep it
									}
									else
									{
										// at = 3	01234 -> 012:34 -> 012:345 -> 012345
										if (sm >= mirrorWorker.at)
										{
											sm = mirrorWorker.at - (sm - mirrorWorker.at) - 1;
										}
										// otherwise keep it
									}
								}
								srcABM.access_element(mirrorWorker.elementMirror) = sm;
							}
							auto& dstV = dst->access_at(dstABM);
							if (srcRange.does_contain(srcABM))
							{
								auto const & srcV = src->access_at(srcABM);
								dstV = srcV;
							}
						}
					}
					// swap and prune
					swap(src, dst);	dst->clear();
					src->prune();
				}
			}
		}

		// offset
		{
			if (!src->range.is_empty())
			{
				src->range.offset(offset);
			}
			// no swap, no need to prune
		}
	}

	// combine into result layer
	if (!src->range.is_empty())
	{
		_resultLayer->access_at(src->range.bottom_left());
		_resultLayer->access_at(src->range.top_right());

		_resultLayer->combine_data_from(src->range, src->pixels);
	}

	return result;
}

void SpriteTransformLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	base::rescale(_moreVoxels, _lessVoxels);

	if (_moreVoxels.is_set())
	{
		int scale = _moreVoxels.get();
		offset *= scale;
		mirror.at *= scale;
		if (mirror.axis)
		{
			mirror.at += scale / 2;
			mirror.axis = mod(scale, 2) == 1;
		}
	}
	if (_lessVoxels.is_set())
	{
		int scale = _lessVoxels.get();
		offset /= scale;
		if (mod(mirror.at, scale) >= scale / 2)
		{
			mirror.axis = true;
		}
		mirror.at /= scale;
	}
}

VectorInt2 SpriteTransformLayer::to_local(Sprite* _partOf, VectorInt2 const& _world, bool _ignoreMirrors) const
{
	VectorInt2 coord = base::to_local(_partOf, _world, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		// in reverse
	
		// offset
		{
			coord = coord - offset;
		}

		// mirror
		if (!_ignoreMirrors)
		{
			if (mirror.mirrorAxis != NONE)
			{
				int& c = coord.access_element(mirror.mirrorAxis);

				if (mirror.axis)
				{
					if (mirror.dir == 0)
					{
						// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
						c = mirror.at - (c - mirror.at);
					}
					else if (mirror.dir < 0)
					{
						// at = 2	0123 -> 01:2:3 -> 1:2:3 -> 123
						if (c < mirror.at)
						{
							c = mirror.at - (c - mirror.at);
						}
						// otherwise keep it
					}
					else
					{
						// at = 2	0123 -> 01:2:3 -> 01:2:34 -> 01234
						if (c > mirror.at)
						{
							c = mirror.at - (c - mirror.at);
						}
						// otherwise keep it
					}
				}
				else
				{
					if (mirror.dir == 0)
					{
						// at = 3	0123 -> 012:3 -> 2:345 -> 2345
						c = mirror.at - (c - mirror.at) - 1;
					}
					else if (mirror.dir < 0)
					{
						// at = 3	01234 -> 012:34 -> 12:34 -> 1234
						if (c < mirror.at)
						{
							c = mirror.at - (c - mirror.at) - 1;
						}
						// otherwise keep it
					}
					else
					{
						// at = 3	01234 -> 012:34 -> 012:345 -> 012345
						if (c >= mirror.at)
						{
							c = mirror.at - (c - mirror.at) - 1;
						}
						// otherwise keep it
					}
				}
			}
		}

		// rotate
		{
			VectorInt2 xAxis, yAxis;
			rotate.prepare_axes(OUT_ xAxis, OUT_ yAxis);

			VectorInt2 xAxisInv, yAxisInv;
			xAxisInv.x = xAxis.x;
			xAxisInv.y = yAxis.x;
			yAxisInv.x = xAxis.y;
			yAxisInv.y = yAxis.y;
			
			coord = xAxisInv * coord.x + yAxisInv * coord.y;
		}
	}

	return coord;
}

VectorInt2 SpriteTransformLayer::to_world(Sprite* _partOf, VectorInt2 const& _local, bool _ignoreMirrors) const
{
	VectorInt2 coord = base::to_world(_partOf, _local, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		// rotate
		{
			VectorInt2 xAxis, yAxis;
			rotate.prepare_axes(OUT_ xAxis, OUT_ yAxis);

			coord = xAxis * coord.x + yAxis * coord.y;
		}

		// mirror
		if (!_ignoreMirrors)
		{
			if (mirror.mirrorAxis != NONE)
			{
				int& c = coord.access_element(mirror.mirrorAxis);

				if (mirror.axis)
				{
					if (mirror.dir == 0)
					{
						// at = 2	0123 -> 01:2:3 -> 1:2:34 -> 1234
						c = mirror.at - (c - mirror.at);
					}
					// otherwise keep it (can be both, keep original)
				}
				else
				{
					if (mirror.dir == 0)
					{
						// at = 3	0123 -> 012:3 -> 2:345 -> 2345
						c = mirror.at - (c - mirror.at) - 1;
					}
					// otherwise keep it (can be both, keep original)
				}
			}
		}

		// offset
		{
			coord = coord + offset;
		}
	}

	return coord;
}

ISpriteLayer* SpriteTransformLayer::create_for_copy() const
{
	return new SpriteTransformLayer();
}

void SpriteTransformLayer::fill_copy(ISpriteLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<SpriteTransformLayer>(_copy);
	an_assert(copy);
	copy->rotate = rotate;
	copy->mirror = mirror;
	copy->offset = offset;
}

//--

void SpriteTransformLayer::Rotate::prepare_axes(OUT_ VectorInt2& xAxis, OUT_ VectorInt2& yAxis) const
{
	xAxis = VectorInt2(1, 0);
	yAxis = VectorInt2(0, 1);
	int useYaw = mod(yaw, 4);
	struct RotateAxis
	{
		static VectorInt2 yaw(VectorInt2 axis)
		{
			swap(axis.x, axis.y); axis.y *= -1;
			return axis;
		}
	};
	{
		an_assert(useYaw >= 0);
		while (useYaw > 0)
		{
			xAxis = RotateAxis::yaw(xAxis);
			yAxis = RotateAxis::yaw(yAxis);
			--useYaw;
		}
	}
}

bool SpriteTransformLayer::Rotate::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;
	for_every(n, _node->children_named(_childName))
	{
		yaw = n->get_int_attribute(TXT("yaw"), yaw);
	}
	return result;
}

bool SpriteTransformLayer::Rotate::save_to_xml_child_node(IO::XML::Node * _node, tchar const* _childName) const
{
	bool result = true;
	if (auto* n = _node->add_node(_childName))
	{
		n->set_int_attribute(TXT("yaw"), yaw);
	}
	return result;
}

//--

bool SpriteTransformLayer::Mirror::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;
	for_every(n, _node->children_named(_childName))
	{
		mirrorAxis = n->get_int_attribute(TXT("mirrorAxis"), mirrorAxis);
		at = n->get_int_attribute(TXT("at"), at);
		axis = n->get_bool_attribute(TXT("axis"), axis);
		dir = n->get_int_attribute(TXT("dir"), dir);
	}
	return result;
}

bool SpriteTransformLayer::Mirror::save_to_xml_child_node(IO::XML::Node * _node, tchar const* _childName) const
{
	bool result = true;
	if (auto* n = _node->add_node(_childName))
	{
		n->set_int_attribute(TXT("mirrorAxis"), mirrorAxis);
		n->set_int_attribute(TXT("at"), at);
		n->set_bool_attribute(TXT("axis"), axis);
		n->set_int_attribute(TXT("dir"), dir);
	}
	return result;
}

//--

REGISTER_FOR_FAST_CAST(SpriteFullTransformLayer);

SpriteFullTransformLayer::SpriteFullTransformLayer()
{
	tempALayer = new SpriteLayer();
	tempBLayer = new SpriteLayer();
}

SpriteFullTransformLayer::~SpriteFullTransformLayer()
{
}

bool SpriteFullTransformLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	rotate = Rotator3();
	offset = Vector2::zero;

	// load
	//
	result &= rotate.load_from_xml_child_node(_node, TXT("rotate"));
	result &= offset.load_from_xml_child_node(_node, TXT("offset"));

	return result;
}

bool SpriteFullTransformLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	result &= rotate.save_to_xml_child_node(_node, TXT("rotate"));
	result &= offset.save_to_xml_child_node(_node, TXT("offset"));

	return result;
}

bool SpriteFullTransformLayer::combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const
{
	if (!is_visible(_for))
	{
		return base::combine(_for, _resultLayer, _allLayers, _context);
	}

	tempALayer->clear();
	tempBLayer->clear();

	auto* dst = tempALayer.get();
	auto* src = tempBLayer.get();
	bool result = true;

	// read children result
	{
		result &= base::combine(_for, dst, _allLayers, _context);
		// swap and prune
		swap(src, dst);	dst->clear();
		src->prune();
	}

	// actual transform
	{
		// read all pixels and put them into dst
		{
			RangeInt2 srcRange = src->get_range();
			if (!srcRange.is_empty())
			{
				Range2 srcRangeFull = _for->to_full_range(srcRange);

				Range2 dstRangeFull = Range2::empty;

				{
					Transform transform((offset * _for->get_pixel_size()).to_vector3(), rotate.to_quat());

					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(0.0f, 0.0f)).to_vector3()).to_vector2());
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(0.0f, 0.0f)).to_vector3()).to_vector2());
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(0.0f, 1.0f)).to_vector3()).to_vector2());
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(0.0f, 1.0f)).to_vector3()).to_vector2());
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(1.0f, 0.0f)).to_vector3()).to_vector2());
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(1.0f, 0.0f)).to_vector3()).to_vector2());
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(1.0f, 1.0f)).to_vector3()).to_vector2());
					dstRangeFull.include(transform.location_to_world(srcRangeFull.get_at(Vector2(1.0f, 1.0f)).to_vector3()).to_vector2());
				}
				
				RangeInt2 dstRange = _for->to_pixel_range(dstRangeFull, true);
				dstRange.expand_by(VectorInt2::one);

				Transform transform(offset.to_vector3(), rotate.to_quat());

				for_range(int, y, dstRange.y.min, dstRange.y.max)
				{
					for_range(int, x, dstRange.x.min, dstRange.x.max)
					{
						VectorInt2 dstXY(x, y);
						// this is not full, this is float (!)
						Vector2 dstXYf = dstXY.to_vector2() + Vector2::half;
						Vector2 srcXYf = transform.location_to_local(dstXYf.to_vector3()).to_vector2();
						SpritePixel const* srcV = nullptr;
						if (!srcV)
						{
							VectorInt2 srcXY = TypeConversions::Normal::f_i_cells(srcXYf);
							srcV = src->get_at(srcXY);
							if (srcV && !SpritePixel::is_anything(srcV->pixel))
							{
								srcV = nullptr;
							}
						}
						/*
						if (!srcV)
						{
							ArrayStatic< SpritePixel const*, 10> srcvs;
							float radius = 0.2f;
							Vector2 offsets[] = {
								Vector2( 1.0f,  0.0f,  0.0f),
								Vector2(-1.0f,  0.0f,  0.0f),
								Vector2( 0.0f,  1.0f,  0.0f),
								Vector2( 0.0f, -1.0f,  0.0f),
								Vector2( 0.0f,  0.0f,  1.0f),
								Vector2( 0.0f,  0.0f, -1.0f),
							};
							for_count(int, i, 6)
							{
								VectorInt2 srcXY = TypeConversions::Normal::f_i_cells(srcXYf + offsets[i] * radius);
								auto* sv = src->get_at(srcXY);
								if (sv && ! SpritePixel::is_anything(sv->pixel))
								{
									srcvs.push_back(sv);
								}
							}
							int amount = 0;
							for_count(int, i, srcvs.get_size())
							{
								int amountThis = 0;
								auto* checksv = srcvs[i];
								for_every_ptr(sv, srcvs)
								{
									if (checksv && checksv->pixel == sv->pixel)
									{
										++amountThis;
									}
								}
								if (amountThis > amount || !srcV)
								{
									amount = amountThis;
									srcV = checksv;
								}
							}
						}
						*/
						if (srcV)
						{
							dst->access_at(dstXY) = *srcV;
						}
					}
				}
			}
		}
		// swap and prune
		swap(src, dst);	dst->clear();
		src->prune();
	}

	// combine into result layer
	if (!src->range.is_empty())
	{
		_resultLayer->access_at(src->range.bottom_left());
		_resultLayer->access_at(src->range.top_right());

		_resultLayer->combine_data_from(src->range, src->pixels);
	}

	return result;
}

void SpriteFullTransformLayer::rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels)
{
	base::rescale(_moreVoxels, _lessVoxels);

	if (_moreVoxels.is_set())
	{
		int scale = _moreVoxels.get();
		offset *= (float)scale;
	}
	if (_lessVoxels.is_set())
	{
		int scale = _lessVoxels.get();
		offset /= (float)scale;
	}
}

VectorInt2 SpriteFullTransformLayer::to_local(Sprite* _partOf, VectorInt2 const& _world, bool _ignoreMirrors) const
{
	VectorInt2 coord = base::to_local(_partOf, _world, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		Transform transform(offset.to_vector3(), rotate.to_quat());

		Vector2 worldF = coord.to_vector2() + Vector2::half;
		
		Vector2 localF = transform.location_to_local(worldF.to_vector3()).to_vector2();

		VectorInt2 localV = TypeConversions::Normal::f_i_cells(localF);

		coord = localV;
	}

	return coord;
}

VectorInt2 SpriteFullTransformLayer::to_world(Sprite* _partOf, VectorInt2 const& _local, bool _ignoreMirrors) const
{
	VectorInt2 coord = base::to_world(_partOf, _local, _ignoreMirrors);

	if (is_visible(_partOf))
	{
		Transform transform(offset.to_vector3(), rotate.to_quat());

		Vector2 localF = coord.to_vector2() + Vector2::half;

		Vector2 worldF = transform.location_to_world(localF.to_vector3()).to_vector2();

		VectorInt2 worldV = TypeConversions::Normal::f_i_cells(worldF);

		coord = worldV;
	}

	return coord;
}

ISpriteLayer* SpriteFullTransformLayer::create_for_copy() const
{
	return new SpriteFullTransformLayer();
}

void SpriteFullTransformLayer::fill_copy(ISpriteLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<SpriteFullTransformLayer>(_copy);
	an_assert(copy);
	copy->rotate = rotate;
	copy->offset = offset;
}

//--

REGISTER_FOR_FAST_CAST(IncludeSpriteLayer);

IncludeSpriteLayer::IncludeSpriteLayer()
{
	includedLayer = new SpriteLayer();
}

IncludeSpriteLayer::IncludeSpriteLayer(Sprite* vm)
{
	includedLayer = new SpriteLayer();
	set_included(vm);
}

IncludeSpriteLayer::~IncludeSpriteLayer()
{
}

bool IncludeSpriteLayer::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	included.clear();

	// load
	//
	result &= included.load_from_xml(_node, TXT("sprite"), _lc);
	
	return result;
}

bool IncludeSpriteLayer::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= included.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CombineSpriteLayers)
	{
		if (auto* ivm = included.get())
		{
			if (ivm->resultLayer->get_range().is_empty()) // most likely not combined, combine now
			{
				CONTENT_ACCESS::combine_pixels(ivm);
			}
		}
	}

	return result;
}

bool IncludeSpriteLayer::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	result &= included.save_to_xml(_node, TXT("sprite"));

	return result;
}

bool IncludeSpriteLayer::combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const
{
	bool result = base::combine(_for, _resultLayer, _allLayers, _context);

	an_assert(includedLayer.get(), TXT("was it removed or constructor did not create it?"));

	includedLayer->clear();

	if (is_visible(_for))
	{
		if (auto* ivm = included.get())
		{
			if (auto* l = fast_cast<SpriteLayer>(ivm->resultLayer.get()))
			{
				if (!l->range.is_empty())
				{
					// make space
					includedLayer->access_at(l->range.bottom_left());
					includedLayer->access_at(l->range.top_right());

					includedLayer->combine_data_from(l->range, l->pixels);

					// and into the actual _resultLayer
					_resultLayer->access_at(l->range.bottom_left());
					_resultLayer->access_at(l->range.top_right());

					_resultLayer->combine_data_from(l->range, l->pixels);
				}
			}
		}
	}

	return result;
}

ISpriteLayer* IncludeSpriteLayer::create_for_copy() const
{
	return new IncludeSpriteLayer();
}

void IncludeSpriteLayer::fill_copy(ISpriteLayer* _copy) const
{
	base::fill_copy(_copy);
	auto* copy = fast_cast<IncludeSpriteLayer>(_copy);
	an_assert(copy);
	copy->included = included;
}

//--

#define ACCESS(what) auto& what = _this->what;
#define GET(what) auto const & what = _this->what;

void SpriteContentAccess::clear(Sprite* _this)
{
	_this->layers.clear();
}

void SpriteContentAccess::prune(Sprite* _this)
{
	for_every_ref(layer, _this->layers)
	{
		if (auto* sl = fast_cast<SpriteLayer>(layer))
		{
			sl->prune();
		}
	}
}

RangeInt2 SpriteContentAccess::get_combined_range(Sprite* _this)
{
	return _this->resultLayer->get_range();
}

void SpriteContentAccess::combine_pixels(Sprite* _this)
{
	CombineSpritePixelsContext context;

	combine_pixels(_this, context);
}

void SpriteContentAccess::combine_pixels(Sprite* _this, CombineSpritePixelsContext const& _context)
{
	ACCESS(resultLayer);
	ACCESS(layers);

	resultLayer->reset();

	if (layers.is_empty())
	{
		layers.push_back();
		layers.get_last() = new SpriteLayer();
	}

	ISpriteLayer::for_every_child_combining(nullptr, layers, [_this, &_context](ISpriteLayer* _layer)
		{
			return _layer->combine(_this, _this->resultLayer.get(), _this->layers, _context);
		});
}

void SpriteContentAccess::fill_at(Sprite* _this, SpriteLayer* _inLayer, VectorInt2 const& _worldAt, std::function<bool(SpritePixel& _pixel)> _doForPixel)
{
	_inLayer->fill_at(_this, _this->resultLayer.get(), _worldAt, _doForPixel);
}

bool SpriteContentAccess::merge_temporary_layers(Sprite* _this, ISpriteLayer* _exceptLayer)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	bool anyMerged = false;

	int keepEditedLayerIndex = editedLayerIndex;

	for (int i = 0; i < layers.get_size() - 1; ++i)
	{
		auto* l = layers[i].get();
		auto* nl = layers[i + 1].get();
		if (l->is_temporary() &&
			l != _exceptLayer &&
			nl != _exceptLayer)
		{
			anyMerged = true;
			editedLayerIndex = i;
			String unused;
			merge_with_next(_this, unused);
			if (i < keepEditedLayerIndex)
			{
				--keepEditedLayerIndex;
			}
			--i;
		}
	}

	editedLayerIndex = keepEditedLayerIndex;

	return anyMerged;
}

bool SpriteContentAccess::merge_with_next(Sprite* _this, OUT_ String& _resultInfo)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (!layers.is_index_valid(editedLayerIndex) ||
		!layers.is_index_valid(editedLayerIndex + 1))
	{
		_resultInfo = TXT("no next layer to merge");
		return false;
	}

	auto* l = layers[editedLayerIndex].get();
	auto* nl = layers[editedLayerIndex + 1].get();
	if (l->get_depth() != nl->get_depth())
	{
		_resultInfo = TXT("no next layer to merge");
		return false;
	}

	if (!l->is_visible(_this) || !nl->is_visible(_this))
	{
		_resultInfo = TXT("both layers should be visible");
		return false;
	}

	auto* sl = fast_cast<SpriteLayer>(l);
	auto* nsl = fast_cast<SpriteLayer>(nl);
	if (!sl || !nsl)
	{
		_resultInfo = TXT("can't merge these two layers");
		return false;
	}

	sl->merge(nsl);

	// keep valid name
	if (!sl->get_id().is_valid() ||
		sl->is_temporary())
	{
		sl->set_id(nsl->get_id());
	}
	sl->be_temporary(sl->is_temporary() && nsl->is_temporary());

	layers.remove_at(editedLayerIndex + 1);

	return true;
}

bool SpriteContentAccess::merge_with_children(Sprite* _this, OUT_ String& _resultInfo)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex); 
	
	if (!layers.is_index_valid(editedLayerIndex))
	{
		_resultInfo = TXT("no next layer to merge");
		return false;
	}

	auto* l = layers[editedLayerIndex].get();

	if (!l->is_visible(_this))
	{
		_resultInfo = TXT("layer not visible");
		return false;
	}

	Name newId = l->get_id();

	int childCount = 0;
	int lDepth = l->get_depth();
	for (int i = editedLayerIndex + 1; i < layers.get_size(); ++i)
	{
		if (layers[i]->get_depth() <= lDepth)
		{
			break;
		}
		if (!newId.is_valid())
		{
			newId = layers[i]->get_id();
		}
		++childCount;
	}

	an_assert(!l->is_temporary());

	// always create a replacement SpriteLayer that will be filled with "combine" result
	RefCountObjectPtr<ISpriteLayer> replacementLayer;
	SpriteLayer* replacementLayerVML = new SpriteLayer();
	replacementLayer = replacementLayerVML;
	l->fill_copy_basics(replacementLayer.get());

	// use new id that could be the original one or taken from a child
	replacementLayer->set_id(newId);

	// combine now
	CombineSpritePixelsContext context;
	l->combine(_this, replacementLayerVML, layers, context);

	// put it where we are and remove old us and children
	layers.insert_at(editedLayerIndex, replacementLayer);
	layers.remove_at(editedLayerIndex + 1, 1 + childCount);

	return true;
}

void SpriteContentAccess::merge_all(Sprite* _this)
{
	// to get the result
	combine_pixels(_this);

	// remove all existing layers
	clear(_this);

	// and replace them with a copy of result
	add_layer(_this, _this->resultLayer->create_copy());
}

void SpriteContentAccess::debug_draw_pixel(Sprite* _this, VectorInt2 const& _at, Colour const& _colour, bool _front)
{
	Vector2 at = _at.to_vector2();
	at -= _this->zeroOffset;
	Vector2 op = at + Vector2::one;
	at *= _this->pixelSize;
	op *= _this->pixelSize;

	debug_draw_box_region(_this, at, op, _colour, _front);
}

void SpriteContentAccess::debug_draw_box_region(Sprite* _this, Vector2 const& _a, Vector2 const& _b, Colour const& _colour, bool _front, float _fillAlpha)
{
	Range3 r = Range3::empty;
	r.include(_a.to_vector3());
	r.include(_b.to_vector3());
	if (r.x.length() == 0.0f)
	{
		r.x.max += 0.001f;
	}
	if (r.y.length() == 0.0f)
	{
		r.y.max += 0.001f;
	}
	r.z.min = 0.000f;
	r.z.max = 0.001f;
	debug_draw_box(_front, false, _colour, _fillAlpha, Box(r));
	/*
	}
	else
	{
		// bottom
		debug_draw_line(_front, _colour, Vector2(_a.x, _a.y, _a.z), Vector2(_b.x, _a.y, _a.z));
		debug_draw_line(_front, _colour, Vector2(_b.x, _a.y, _a.z), Vector2(_b.x, _b.y, _a.z));
		debug_draw_line(_front, _colour, Vector2(_a.x, _a.y, _a.z), Vector2(_a.x, _b.y, _a.z));
		debug_draw_line(_front, _colour, Vector2(_a.x, _b.y, _a.z), Vector2(_b.x, _b.y, _a.z));

		// t_b
		debug_draw_line(_front, _colour, Vector2(_a.x, _a.y, _b.z), Vector2(_b.x, _a.y, _b.z));
		debug_draw_line(_front, _colour, Vector2(_b.x, _a.y, _b.z), Vector2(_b.x, _b.y, _b.z));
		debug_draw_line(_front, _colour, Vector2(_a.x, _a.y, _b.z), Vector2(_a.x, _b.y, _b.z));
		debug_draw_line(_front, _colour, Vector2(_a.x, _b.y, _b.z), Vector2(_b.x, _b.y, _b.z));

		// sides
		debug_draw_line(_front, _colour, Vector2(_a.x, _a.y, _a.z), Vector2(_a.x, _a.y, _b.z));
		debug_draw_line(_front, _colour, Vector2(_b.x, _a.y, _a.z), Vector2(_b.x, _a.y, _b.z));
		debug_draw_line(_front, _colour, Vector2(_a.x, _b.y, _a.z), Vector2(_a.x, _b.y, _b.z));
		debug_draw_line(_front, _colour, Vector2(_b.x, _b.y, _a.z), Vector2(_b.x, _b.y, _b.z));
	}
	*/
}

bool SpriteContentAccess::ray_cast(Sprite const* _this, Vector2 const& _start, OUT_ SpritePixelHitInfo& _hitInfo, bool _onlyActualContent, bool _ignoreZeroOffset)
{
	Vector2 fullOffset = _ignoreZeroOffset ? Vector2::zero : (_this->zeroOffset * _this->pixelSize);
	Vector2 start = _start + fullOffset;
	start /= _this->pixelSize;

	if (auto* rl = fast_cast<SpriteLayer>(_this->resultLayer.get()))
	{
		RangeInt2 range = rl->range;

		VectorInt2 cell = TypeConversions::Normal::f_i_cells(start);

		int pixelIdx = pixel_index(cell, range);
		if (rl->pixels.is_index_valid(pixelIdx))
		{
			auto& pixel = rl->pixels[pixelIdx];
			bool hit = SpritePixel::is_solid(pixel.pixel);
			if (!_onlyActualContent)
			{
				hit |= SpritePixel::is_anything(pixel.pixel);
			}
			if (hit)
			{
				_hitInfo.hit = cell;
				return true;
			}
		}
	}

	return false;
}

bool SpriteContentAccess::ray_cast_xy(Sprite const * _this, Vector2 const& _start, OUT_ SpritePixelHitInfo& _hitInfo)
{
	Vector2 fullOffset = _this->zeroOffset * _this->pixelSize;
	Vector2 start = _start + fullOffset;
	start /= _this->pixelSize;

	{
		VectorInt2 cell = TypeConversions::Normal::f_i_cells(start);

		_hitInfo.hit = cell;

		return true;
	}
	return false;
}

ISpriteLayer* SpriteContentAccess::access_edited_layer(Sprite* _this)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (!layers.is_index_valid(editedLayerIndex))
	{
		editedLayerIndex = 0;
	}
	if (layers.is_index_valid(editedLayerIndex))
	{
		return layers[editedLayerIndex].get();
	}
	return nullptr;
}

int SpriteContentAccess::get_edited_layer_index(Sprite const* _this)
{
	return _this->editedLayerIndex;
}

void SpriteContentAccess::set_edited_layer_index(Sprite* _this, int _index)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	bool visibilityOnly = false;

	if (layers.is_index_valid(editedLayerIndex))
	{
		visibilityOnly = layers[editedLayerIndex]->get_visibility_only();
	}

	editedLayerIndex = clamp(_index, 0, layers.get_size() - 1);

	if (layers.is_index_valid(editedLayerIndex))
	{
		auto* l = layers[editedLayerIndex].get();
		if (visibilityOnly)
		{
			l->set_visibility_only(visibilityOnly, _this);
		}
	}
}

void SpriteContentAccess::move_edited_layer(Sprite* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	while (_by < 0 && editedLayerIndex > 0)
	{
		swap(layers[editedLayerIndex - 1], layers[editedLayerIndex]);
		--editedLayerIndex;
		++_by;
	}
	int maxLayer = layers.get_size() - 1;
	while (_by > 0 && editedLayerIndex < maxLayer)
	{
		swap(layers[editedLayerIndex + 1], layers[editedLayerIndex]);
		++editedLayerIndex;
		--_by;
	}
}

void SpriteContentAccess::change_depth_of_edited_layer(Sprite* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		if (auto* l = layers[editedLayerIndex].get())
		{
			l->set_depth(l->get_depth() + _by);
		}
	}
}

void SpriteContentAccess::change_depth_of_edited_layer_and_its_children(Sprite* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		int depth = layers[editedLayerIndex]->get_depth();
		for (int i = editedLayerIndex; i < layers.get_size(); ++i)
		{
			auto* l = layers[i].get();
			if (i > editedLayerIndex && l->get_depth() <= depth)
			{
				break;
			}
			l->set_depth(l->get_depth() + _by);
		}
	}
}

void SpriteContentAccess::change_depth_of_edited_layers_children(Sprite* _this, int _by)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		int depth = layers[editedLayerIndex]->get_depth();
		for (int i = editedLayerIndex; i < layers.get_size(); ++i)
		{
			auto* l = layers[i].get();
			if (i > editedLayerIndex)
			{
				if (l->get_depth() <= depth)
				{
					break;
				}
				l->set_depth(l->get_depth() + _by);
			}
		}
	}
}

void SpriteContentAccess::remove_edited_layer(Sprite* _this)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		layers.remove_at(editedLayerIndex);
	}
}

void SpriteContentAccess::add_layer(Sprite* _this, ISpriteLayer* _layer, int _offset)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_empty())
	{
		layers.push_back();
		layers.get_last() = _layer;
		editedLayerIndex = 0;
	}
	else
	{
		editedLayerIndex = clamp(editedLayerIndex, 0, layers.get_size() - 1);
		RefCountObjectPtr<ISpriteLayer> nl;
		nl = _layer;
		nl->set_depth(layers[editedLayerIndex]->get_depth());
		layers.insert_at(editedLayerIndex + _offset, nl);
		editedLayerIndex += _offset;
	}
}

void SpriteContentAccess::duplicate_edited_layer(Sprite* _this, int _offset)
{
	ACCESS(layers);
	ACCESS(editedLayerIndex);

	if (layers.is_index_valid(editedLayerIndex))
	{
		add_layer(_this, layers[editedLayerIndex]->create_copy(), _offset);
	}
}

void SpriteContentAccess::rescale_more_pixels(Sprite* _this, int _scale)
{
	set_pixel_size(_this, _this->get_pixel_size() / (float)_scale);

	for_every_ref(layer, _this->layers)
	{
		layer->rescale(_scale, NP);
	}
}

void SpriteContentAccess::rescale_less_pixels(Sprite * _this, int _scale)
{
	set_pixel_size(_this, _this->get_pixel_size() * (float)_scale);

	for_every_ref(layer, _this->layers)
	{
		layer->rescale(NP, _scale);
	}
}

void SpriteContentAccess::set_colour_palette(Sprite* _this, ColourPalette const* _colourPalette, bool _convertToNewOne)
{
	ACCESS(layers);
	ACCESS(colourPalette); 

	if (_convertToNewOne && colourPalette.get() && _colourPalette && colourPalette.get() != _colourPalette)
	{
		UsedLibraryStored<ColourPalette> prevPalette = colourPalette;
		Array<int> prevToNew;
		for_every(pc, prevPalette->get_colours())
		{
			int best = NONE;
			float bestDiff = 0.0f;
			for_every(nc, _colourPalette->get_colours())
			{
				float diff = abs(nc->r - pc->r) + abs(nc->g - pc->g) + abs(nc->b - pc->b);
				if (diff < bestDiff || best == NONE)
				{
					bestDiff = diff;
					best = for_everys_index(nc);
				}
			}
			prevToNew.push_back(best);
		}
		for_every_ref(l, layers)
		{
			if (auto* vml = fast_cast<SpriteLayer>(l))
			{
				for_every(v, vml->pixels)
				{
					if (prevToNew.is_index_valid(v->pixel))
					{
						v->pixel = prevToNew[v->pixel];
					}
				}
			}
		}
	}
	colourPalette = _colourPalette;

	combine_pixels(_this);
}

void SpriteContentAccess::remap_colours(Sprite* _this, Array<int> const& _remapColours)
{
	ACCESS(layers);

	for_every_ref(l, layers)
	{
		if (auto* vml = fast_cast<SpriteLayer>(l))
		{
			for_every(v, vml->pixels)
			{
				if (_remapColours.is_index_valid(v->pixel))
				{
					v->pixel = _remapColours[v->pixel];
				}
			}
		}
	}

	combine_pixels(_this);
}

bool SpriteContentAccess::can_include(Sprite const * _this, Sprite const* _other)
{
	struct IncludeCheck
	{
		Array<Sprite const*> used;

		bool setup_for(Sprite const* _vm)
		{
			used.clear();
			used.push_back(_vm);
			int i = 0;
			while (i < used.get_size())
			{
				auto* vm = used[i];
				if (vm)
				{
					for_every_ref(l, vm->layers)
					{
						if (auto* ivml = fast_cast<IncludeSpriteLayer>(l))
						{
							auto* ivm = ivml->get_included();
							used.push_back_unique(ivm);
						}
					}
				}

				if (used.get_size() > 10000)
				{
					an_assert(false, TXT("that's too many?"));
					return false;
				}
				++i;
			}
			return true;
		}
	} includeCheck;

	if (!includeCheck.setup_for(_other))
	{
		return false;
	}
	if (includeCheck.used.does_contain(_this))
	{
		// can't include someone who already included us
		return false;
	}

	return true;
}

void SpriteContentAccess::render(Sprite const* _this, RenderContext const & _context)
{
#ifdef AN_DEBUG_RENDERER
	GET(pixelSize);
	GET(zeroOffset);
	GET(resultLayer);
	GET(layers);
	GET(editedLayerIndex);
	GET(colourPalette);

	debug_push_transform(Transform((pixelSize * _context.pixelOffset.to_vector2()).to_vector3(), Quat::identity));
	for_count(int, renderPass, RENDER_PASS_COUNT)
	{
		ISpriteLayer* layer = nullptr;
		SpriteLayer* pixelLayer = nullptr; // they might be different (we have IncludeSpriteLayer's result layer)
		Optional<Colour> pixelColour;
		if (renderPass == RENDER_PASS_NORMAL)
		{
			layer = fast_cast<SpriteLayer>(resultLayer.get());
			if (_context.wholeColour.is_set())
			{
				pixelColour = _context.wholeColour.get();
			}
		}
		else if (renderPass == RENDER_PASS_EDITED_LAYER && _context.editedLayerColour.is_set())
		{
			if (layers.is_index_valid(editedLayerIndex))
			{
				auto* l = layers[editedLayerIndex].get();
				if (auto* vml = fast_cast<SpriteLayer>(l))
				{
					layer = vml;
					pixelLayer = vml;
				}
				if (auto* ivml = fast_cast<IncludeSpriteLayer>(l))
				{
					layer = ivml;
					pixelLayer = ivml->get_included_layer();
				}
				if (layer)
				{
					pixelColour = _context.editedLayerColour;
				}
			}
		}
		auto* rl = fast_cast<SpriteLayer>(resultLayer.get());
		if (layer && rl)
		{
			RangeInt2 range = rl->range;
			int rangex = range.x.length();
			int rangey = range.y.length();
			int yOffset = rangex;
			auto* cp = colourPalette.get();
			for_range(int, y, range.y.min, range.y.max)
			{
				VectorInt2 startAt(range.x.min, y);
				SpritePixel const* v = &rl->pixels[pixel_index(startAt, range)];
				SpritePixel const* vxp = v - 1;
				SpritePixel const* vxn = v + 1;
				SpritePixel const* vyp = v - yOffset;
				SpritePixel const* vyn = v + yOffset;
				for_range(int, x, range.x.min, range.x.max)
				{
					bool drawVoxel = SpritePixel::is_drawable(v->pixel);
					if (resultLayer.get() != layer && layer && pixelLayer)
					{
						drawVoxel = false;
						VectorInt2 at(x, y);
						VectorInt2 atLocal = layer->to_local(cast_to_nonconst(_this), at);
						if (auto* v = pixelLayer->get_at(atLocal))
						{
							drawVoxel = SpritePixel::is_drawable(v->pixel);
						}
					}
					if (drawVoxel)
					{
						Colour colour = Colour::greyLight;
						if (cp && cp->get_colours().is_index_valid(v->pixel))
						{
							colour = cp->get_colours()[v->pixel];
						}
						Vector2 cmin = VectorInt2(x, y).to_vector2();
						if (! _context.ignoreZeroOffset)
						{
							cmin -= zeroOffset;
						}
						Vector2 cmax = cmin + Vector2::one;
						cmin *= pixelSize;
						cmax *= pixelSize;
						Optional<bool> highlighted;
						float alpha = 1.0f;
						if (_context.highlightPlane.is_set())
						{
							struct AboveBelow
							{
								Plane plane;
								int above = 0;
								int below = 0;
								AboveBelow(Plane const& _plane) : plane(_plane) {}
								void add(Vector2 const& _point)
								{
									float inFront = plane.get_in_front(_point.to_vector3());
									if (inFront <= 0.0f) ++below;
									if (inFront >= 0.0f) ++above;
								}
								bool is_on_both_sides() const { return above > 0 && below > 0; }
							} aboveBelow(_context.highlightPlane.get());
							aboveBelow.add(Vector2(cmin.x, cmin.y));
							aboveBelow.add(Vector2(cmax.x, cmin.y));
							aboveBelow.add(Vector2(cmin.x, cmax.y));
							aboveBelow.add(Vector2(cmax.x, cmax.y));
							aboveBelow.add(Vector2(cmin.x, cmin.y));
							aboveBelow.add(Vector2(cmax.x, cmin.y));
							aboveBelow.add(Vector2(cmin.x, cmax.y));
							aboveBelow.add(Vector2(cmax.x, cmax.y));
							highlighted = aboveBelow.is_on_both_sides();
							if (!highlighted.get())
							{
								Vector2 c = (cmin + cmax) * 0.5f;
								float f = abs(aboveBelow.plane.get_in_front(c.to_vector3()));
								float af = min(1.0f, 1.0f / (0.5f + f / Vector2::dot(aboveBelow.plane.get_normal().to_vector2(), pixelSize)));
								af = lerp(0.5f, af, sqr(af));
								alpha = 0.7f * af;
							}
						}
						if (pixelColour.is_set())
						{
							colour = pixelColour.get();
							alpha = lerp(0.5f, alpha, 1.0f);
						}
						if (SpritePixel::is_solid(v->pixel))
						{
							{
								Colour c = colour;
								debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, 0.0f), Vector3(cmin.x, cmax.y, 0.0f), Vector3(cmax.x, cmax.y, 0.0f));
								debug_draw_triangle(false, c, Vector3(cmin.x, cmin.y, 0.0f), Vector3(cmax.x, cmax.y, 0.0f), Vector3(cmax.x, cmin.y, 0.0f));
							}
							if (renderPass == RENDER_PASS_NORMAL && _context.pixelEdgeColour.is_set())
							{
								debug_draw_line(false, _context.pixelEdgeColour.get(), Vector3(cmin.x, cmin.y, 0.0f), Vector3(cmin.x, cmax.y, 0.0f));
								debug_draw_line(false, _context.pixelEdgeColour.get(), Vector3(cmin.x, cmax.y, 0.0f), Vector3(cmax.x, cmax.y, 0.0f));
								debug_draw_line(false, _context.pixelEdgeColour.get(), Vector3(cmax.x, cmax.y, 0.0f), Vector3(cmax.x, cmin.y, 0.0f));
								debug_draw_line(false, _context.pixelEdgeColour.get(), Vector3(cmax.x, cmin.y, 0.0f), Vector3(cmin.x, cmin.y, 0.0f));
							}
						}
						else if (!_context.onlySolidPixels)
						{
							if (v->pixel == SpritePixel::Invisible)
							{
								Vector2 ccen = (cmin + cmax) * 0.5f;
								colour = Colour::blue;
								debug_draw_line(false, colour, Vector3(cmin.x, ccen.y, 0.0f), Vector3(cmax.x, ccen.y, 0.0f));
								debug_draw_line(false, colour, Vector3(ccen.x, cmin.y, 0.0f), Vector3(ccen.x, cmax.y, 0.0f));
							}
							else if (v->pixel == SpritePixel::ForceEmpty)
							{
								colour = Colour::red;
								debug_draw_line(false, colour, Vector3(cmin.x, cmin.y, 0.0f), Vector3(cmax.x, cmax.y, 0.0f));
								debug_draw_line(false, colour, Vector3(cmin.x, cmax.y, 0.0f), Vector3(cmax.x, cmin.y, 0.0f));
							}
						}
					}

					++v;
					++vxp;
					++vxn;
					++vyp;
					++vyn;
				}
			}
		}
	}
	debug_pop_transform();
#endif
}

