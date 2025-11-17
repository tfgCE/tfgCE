#include "colourClashSprite.h"
#include "..\pipelines\colourClashPipeline.h"
#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\library\usedLibraryStored.h"
#include "..\..\core\serialisation\importerHelper.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

using namespace Framework;
LIBRARY_STORED_DEFINE_TYPE(ColourClashSprite, colourClashSprite);

REGISTER_FOR_FAST_CAST(ColourClashSprite);

ColourClashSprite::ColourClashSprite(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, uv0(Vector2(0.0f, 0.0f))
, uv1(Vector2(1.0f, 1.0f))
, uvAnchor(Vector2(0.5f, 0.5f))
, size(Vector2(32.0f, 32.0f))
{
}

bool ColourClashSprite::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	result &= texture.load_from_xml(_node, TXT("texture"), _lc);
	result &= uv0.load_from_xml_child_node(_node, TXT("uv0"), nullptr, nullptr, true);
	result &= uv1.load_from_xml_child_node(_node, TXT("uv1"), nullptr, nullptr, true);
	result &= uvAnchor.load_from_xml_child_node(_node, TXT("uvAnchor"));

	uv0.y = 1.0f - uv0.y;
	uv1.y = 1.0f - uv1.y;
	uvAnchor.y = 1.0f - uvAnchor.y;
	result &= uv0.load_from_xml_child_node(_node, TXT("uv0"), TXT("x"), TXT("ty"), true);
	result &= uv1.load_from_xml_child_node(_node, TXT("uv1"), TXT("x"), TXT("ty"), true);
	result &= uvAnchor.load_from_xml_child_node(_node, TXT("uvAnchor"), TXT("x"), TXT("ty"), true);
	result &= uv0.load_from_xml_child_node(_node, TXT("uv0"), TXT("x"), TXT("topy"), true);
	result &= uv1.load_from_xml_child_node(_node, TXT("uv1"), TXT("x"), TXT("topy"), true);
	result &= uvAnchor.load_from_xml_child_node(_node, TXT("uvAnchor"), TXT("x"), TXT("topy"), true);
	uv0.y = 1.0f - uv0.y;
	uv1.y = 1.0f - uv1.y;
	uvAnchor.y = 1.0f - uvAnchor.y;

	foregroundColour.load_from_xml_child_node(_node, TXT("foregroundColour"));
	backgroundColour.load_from_xml_child_node(_node, TXT("backgroundColour"));
	brightColour = _node->first_child_named(TXT("brightColour")) ? Colour::white : brightColour;
	brightColour = _node->first_child_named(TXT("noBrightColour")) ? Colour::black : brightColour;
	brightColour = _node->first_child_named(TXT("notBrightColour")) ? Colour::black : brightColour;
	brightColour = _node->first_child_named(TXT("dimmColour")) ? Colour::black : brightColour;

	return result;
}

bool ColourClashSprite::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	//uvAnchor.x = clamp(uvAnchor.x, uv0.x, uv1.x);
	//uvAnchor.y = clamp(uvAnchor.y, uv0.y, uv1.y);

	result &= texture.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::TexturesLoaded)
	{
		if (texture.get() &&
			texture.get()->get_texture())
		{
			Vector2 textureSize = texture.get()->get_texture()->get_size().to_vector2();
			if (auto const * setup = texture.get()->get_texture()->get_setup())
			{
				if (setup->filteringMag == ::System::TextureFiltering::nearest ||
					setup->filteringMin == ::System::TextureFiltering::nearest)
				{
					uv0 = (uv0 * textureSize + Vector2(0.001f, 0.001f)).to_vector_int2_cells().to_vector2() / textureSize;
					uv1 = (uv1 * textureSize + Vector2(1.001f, 1.001f)).to_vector_int2_cells().to_vector2() / textureSize;
					uvAnchor = (uvAnchor * textureSize + Vector2(0.001f, 0.001f)).to_vector_int2_cells().to_vector2() / textureSize;
				}
			}
			size = (uv1 - uv0) * textureSize;
			anchor = uvAnchor * textureSize;
			leftBottomOffset = (uv0 - uvAnchor) * textureSize;
			rightTopOffset = (uv1 - uvAnchor) * textureSize;
			size.x = round(size.x);
			size.y = round(size.y);
			anchor.x = round(anchor.x);
			anchor.y = round(anchor.y);
			leftBottomOffset.x = round(leftBottomOffset.x);
			leftBottomOffset.y = round(leftBottomOffset.y);
			rightTopOffset.x = round(rightTopOffset.x);
			rightTopOffset.y = round(rightTopOffset.y);
			relAnchor = -leftBottomOffset;
		}
	}

	return result;
}

struct ColourClashSpriteVertex
{
	Vector3 location;
	Vector2 uv;

	ColourClashSpriteVertex() {}
	ColourClashSpriteVertex(Vector2 const & _uv, Vector2 const & _loc) : location(Vector3(_loc.x, _loc.y, 0.0f)), uv(_uv) {}
};

void ColourClashSprite::draw_graphics_at(::System::Video3D* _v3d, VectorInt2 const & _at) const
{
	Vector2 at = _at.to_vector2();

	at += leftBottomOffset;

	at = (floor(at)).to_vector_int2_cells().to_vector2();

	_v3d->set_fallback_texture(texture.get()->get_texture()->get_texture_id());
	_v3d->set_fallback_colour(Colour::white);

	ColourClashPipeline::get_graphics_shader_program_instance()->set_uniform(ColourClashPipeline::get_graphics_shader_program_in_texture_index(), texture.get()->get_texture()->get_texture_id());

	_v3d->mark_enable_blend();
	ArrayStatic<ColourClashSpriteVertex, 4> vertices;
	Range2 range(Range(at.x, at.x + size.x), Range(at.y, at.y + size.y));
	Range2 uvRange(Range(uv0.x, uv1.x), Range(uv0.y, uv1.y));

	vertices.push_back(ColourClashSpriteVertex(Vector2(uvRange.x.min, uvRange.y.min), Vector2(range.x.min, range.y.min)));
	vertices.push_back(ColourClashSpriteVertex(Vector2(uvRange.x.min, uvRange.y.max), Vector2(range.x.min, range.y.max)));
	vertices.push_back(ColourClashSpriteVertex(Vector2(uvRange.x.max, uvRange.y.max), Vector2(range.x.max, range.y.max)));
	vertices.push_back(ColourClashSpriteVertex(Vector2(uvRange.x.max, uvRange.y.min), Vector2(range.x.max, range.y.min)));

	System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
	modelViewStack.push_set(Matrix44::identity);
	Meshes::Mesh3D::render_data(_v3d, ColourClashPipeline::get_graphics_shader_program_instance(), nullptr, Meshes::Mesh3DRenderingContext::none(),
		vertices.get_data(), ::Meshes::Primitive::TriangleFan, 2, ColourClashPipeline::get_vertex_format());
	modelViewStack.pop();

	_v3d->mark_disable_blend();

	_v3d->set_fallback_colour();
	_v3d->set_fallback_texture();
}

void ColourClashSprite::draw_colour_grid_at(::System::Video3D* _v3d, VectorInt2 const & _at, Optional<Colour> const & _overrideForegroundColour, Optional<Colour> const & _overrideBackgroundColour, Optional<Colour> const & _overrideBrightColour) const
{
	VectorInt2 at = _at + leftBottomOffset.to_vector_int2_cells();

	::Framework::ColourClashPipeline::draw_rect_on_colour_grid(at, (at + size.to_vector_int2_cells() - VectorInt2::one),
		_overrideForegroundColour.is_set() ? _overrideForegroundColour.get() : foregroundColour,
		_overrideBackgroundColour.is_set() ? _overrideBackgroundColour.get() : backgroundColour,
		_overrideBrightColour.is_set() ? _overrideBrightColour.get() : brightColour);
}
