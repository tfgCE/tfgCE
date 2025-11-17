#include "texturePart.h"

#include "..\game\game.h"

#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\library\usedLibraryStored.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\serialisation\importerHelper.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\video\renderTarget.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(inTexture);

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//
 
LIBRARY_STORED_DEFINE_TYPE(TexturePart, texturePart);
REGISTER_FOR_FAST_CAST(TexturePart);

::System::VertexFormat * TexturePart::s_vertexFormat = nullptr;

void TexturePart::initialise_static()
{
	todo_note(TXT("update after reading main settings?"));
	s_vertexFormat = new ::System::VertexFormat();
	s_vertexFormat->with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float).no_padding().calculate_stride_and_offsets();
}

void TexturePart::close_static()
{
	delete_and_clear(s_vertexFormat);
}

TexturePart::TexturePart(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, uv0(Vector2(0.0f, 0.0f))
, uv1(Vector2(1.0f, 1.0f))
, uvAnchor(Vector2(0.5f, 0.5f))
, size(Vector2(32.0f, 32.0f))
{
	fallbackShaderProgramInstance = new ::System::ShaderProgramInstance();
	fallbackNoBlendShaderProgramInstance = new ::System::ShaderProgramInstance();

	an_assert(::System::Video3D::get());
	fallbackShaderProgramInstance->set_shader_program(::System::Video3D::get()->get_fallback_shader_program(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::WithTexture));
	fallbackNoBlendShaderProgramInstance->set_shader_program(::System::Video3D::get()->get_fallback_shader_program(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::WithTexture | ::System::Video3DFallbackShader::DiscardsBlending));
}

struct TexturePart_DropRenderTarget
: public Concurrency::AsynchronousJobData
{
	RefCountObjectPtr<::System::RenderTarget> renderTarget;
	TexturePart_DropRenderTarget(::System::RenderTarget* _renderTarget)
	: renderTarget(_renderTarget)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		TexturePart_DropRenderTarget* data = (TexturePart_DropRenderTarget*)_data;
		data->renderTarget.clear();
	}
};

TexturePart::~TexturePart()
{
	delete_and_clear(fallbackShaderProgramInstance);
	delete_and_clear(fallbackNoBlendShaderProgramInstance);
	if (renderTarget.get())
	{
		// we can't clear render target here, we have to add a system job for that
		Game::async_system_job(get_library()->get_game(), TexturePart_DropRenderTarget::perform, new TexturePart_DropRenderTarget(renderTarget.get()));
		renderTarget.clear();
	}
}

bool TexturePart::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
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

	return result;
}

void TexturePart::setup(Texture* _texture, Vector2 const& _uv0, Vector2 const& _uv1, Vector2 const& _uvAnchor)
{
	texture = _texture;
	renderTarget.clear();
	uv0 = _uv0;
	uv1 = _uv1;
	uvAnchor = _uvAnchor;
	cache_auto();
}

void TexturePart::setup(::System::RenderTarget* _renderTarget, int _renderTargetIdx, Vector2 const& _uv0, Vector2 const& _uv1, Vector2 const& _uvAnchor)
{
	texture.clear();
	renderTarget = _renderTarget;
	renderTargetIdx = _renderTargetIdx;
	uv0 = _uv0;
	uv1 = _uv1;
	uvAnchor = _uvAnchor;
	cache_auto();
}

void TexturePart::cache_auto()
{
	Vector2 textureSize = Vector2::one;
	bool fitCells = false;

	textureID = ::System::texture_id_none();

	if (texture.get() &&
		texture->get_texture())
	{
		textureID = texture->get_texture()->get_texture_id();
		textureSize = texture.get()->get_texture()->get_size().to_vector2();
		if (auto const* setup = texture.get()->get_texture()->get_setup())
		{
			if (setup->filteringMag == ::System::TextureFiltering::nearest ||
				setup->filteringMin == ::System::TextureFiltering::nearest)
			{
				fitCells = true;
			}
		}
	}
	
	if (renderTarget.get())
	{
		textureID = renderTarget->get_data_texture_id(renderTargetIdx);
		textureSize = renderTarget->get_size().to_vector2();
		fitCells = true;
	}

	if (fitCells)
	{
		uv0 = (uv0 * textureSize + Vector2(0.001f, 0.001f)).to_vector_int2_cells().to_vector2() / textureSize;
		uv1 = (uv1 * textureSize + Vector2(1.001f, 1.001f)).to_vector_int2_cells().to_vector2() / textureSize;
		uvAnchor = (uvAnchor * textureSize + Vector2(0.001f, 0.001f)).to_vector_int2_cells().to_vector2() / textureSize;
	}

	{
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
	}
}

bool TexturePart::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	// do not clamp anchor!
	//uvAnchor.x = clamp(uvAnchor.x, uv0.x, uv1.x);
	//uvAnchor.y = clamp(uvAnchor.y, uv0.y, uv1.y);

	result &= texture.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::TexturesLoaded)
	{
		cache_auto();
	}

	return result;
}

struct TexturePartVertex
{
	Vector3 location;
	Vector2 uv;

	TexturePartVertex() {}
	TexturePartVertex(Vector2 const & _uv, Vector3 const & _loc) : location(_loc), uv(_uv) {}
};

void TexturePart::draw_at(::System::Video3D* _v3d, Vector2 const & _at, TexturePartDrawingContext const & _context) const
{
	Vector2 at = _at;
	
	{
		Vector2 useLBOffset = leftBottomOffset;
		if (_context.use.is_set())
		{
			Vector2 lbo = useLBOffset;
			if (_context.use.get().mirroredX)
			{
				lbo.x = -size.x - lbo.x;
			}
			if (_context.use.get().mirroredY)
			{
				lbo.y = -size.y - lbo.y;
			}
			int rot90 = mod(_context.use.get().rotated90, 4);
			/*
				+-----+		+-----+		+-----+		+-----+
				|	  |		|  	  |		|	2 |		|	  |
				|	  |		|1    |		|	  |		|	  |
				|	  |		|	  |		|	  |		|	  |
				|	  |		|	  |		|	  |		|	 3|
				| 0	  |		|	  |		|	  |		|	  |
				+-----+		+-----+		+-----+		+-----+
			 */
			if (rot90 == 1)
			{
				useLBOffset.x = lbo.y;
				useLBOffset.y = -size.x - lbo.x;
			}
			else if (rot90 == 2)
			{
				useLBOffset.x = -size.x - lbo.x;
				useLBOffset.y = -size.y - lbo.y;
			}
			else if (rot90 == 3)
			{
				useLBOffset.x = -size.y - lbo.y;
				useLBOffset.y = lbo.x;
			}
		}
		at += useLBOffset * _context.scale;
	}

	at = _context.alignToPixels ? at.to_vector_int2_cells().to_vector2() : at;

	_v3d->set_fallback_colour(_context.colour);
	if (auto* t = texture.get())
	{
		_v3d->set_fallback_texture(t->get_texture()->get_texture_id());
		if (!_context.shaderProgramInstance)
		{
			set_fallback_shader_texture(t->get_texture());
		}
	}
	else if (auto* rt = renderTarget.get())
	{
		_v3d->set_fallback_texture(rt->get_data_texture_id(renderTargetIdx));
		if (!_context.shaderProgramInstance)
		{
			set_fallback_shader_texture(rt, renderTargetIdx);
		}
	}

	bool isBlending = _context.blend.get(true);
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		isBlending = _context.blend.get(false);
	}
	if (isBlending)
	{
		_v3d->mark_enable_blend();
	}
	Vector2 sizeScaled = size * _context.scale;
	ArrayStatic<TexturePartVertex, 4> vertices; SET_EXTRA_DEBUG_INFO(vertices, TXT("TexturePart::draw_at[1].vertices"));
	Vector2 lbl = at;
	Vector2 ltr = at + sizeScaled;
	Vector2 uvbl = uv0;
	Vector2 uvtr = uv1;
	if (_context.scale.x < 0.0f)
	{
		swap(lbl.x, ltr.x);
		swap(uvbl.x, uvtr.x);
	}
	if (_context.scale.y < 0.0f)
	{
		swap(lbl.y, ltr.y);
		swap(uvbl.y, uvtr.y);
	}
	an_assert(lbl.x <= ltr.x);
	an_assert(lbl.y <= ltr.y);
	struct Utils
	{
		static void part_of(REF_ float & _b, REF_ float & _t, Range const& _part)
		{
			float b = _b;
			float t = _t;
			float b2t = t - b;
			_b = b + b2t * _part.min;
			_t = b + b2t * _part.max;
		}
	};
	Utils::part_of(lbl.y, ltr.y, _context.verticalRange);
	Utils::part_of(uvbl.y, uvtr.y, _context.verticalRange);

	if (_context.limits.is_set() ||
		_context.showPt.is_set())
	{
		Vector2 orglbl = lbl;
		Vector2 orgltr = ltr;
		Vector2 orguvbl = uvbl;
		Vector2 orguvtr = uvtr;

		if (_context.limits.is_set())
		{
			lbl.x = max(lbl.x, _context.limits.get().x.min);
			ltr.x = min(ltr.x, _context.limits.get().x.max);
			lbl.y = max(lbl.y, _context.limits.get().y.min);
			ltr.y = min(ltr.y, _context.limits.get().y.max);
		}

		if (_context.showPt.is_set())
		{
			lbl.x = at.x + max(lbl.x - at.x, _context.showPt.get().x.min * sizeScaled.x);
			ltr.x = at.x + min(ltr.x - at.x, _context.showPt.get().x.max * sizeScaled.x);
			lbl.y = at.y + max(lbl.y - at.y, _context.showPt.get().y.min * sizeScaled.y);
			ltr.y = at.y + min(ltr.y - at.y, _context.showPt.get().y.max * sizeScaled.y);
		}

		uvbl.x = lerp(calc_pt(orglbl.x, orgltr.x, lbl.x), orguvbl.x, orguvtr.x);
		uvbl.y = lerp(calc_pt(orglbl.y, orgltr.y, lbl.y), orguvbl.y, orguvtr.y);
		uvtr.x = lerp(calc_pt(orglbl.x, orgltr.x, ltr.x), orguvbl.x, orguvtr.x);
		uvtr.y = lerp(calc_pt(orglbl.y, orgltr.y, ltr.y), orguvbl.y, orguvtr.y);
	}

	Vector2 uvSC = uvbl;
	Vector2 uvUp = Vector2(0.0f, uvtr.y - uvbl.y);
	Vector2 uvRt = Vector2(uvtr.x - uvbl.x, 0.0f);
	if (_context.use.is_set())
	{
		an_assert(! _context.limits.is_set() &&
				  ! _context.showPt.is_set() &&
				  _context.verticalRange == Range(0.0f, 1.0f), TXT("\"use\" works only without limits, showPt and verticalRange"));
		an_assert(_context.use.get().texturePart == this, TXT("we should be using TexturePartUse for this texture!"));
		_context.use.get().get_uv_info(OUT_ uvSC, OUT_ uvUp, OUT_ uvRt);
	}

	vertices.push_back(TexturePartVertex(uvSC,					Vector3(lbl.x, lbl.y, 0.0f)));
	vertices.push_back(TexturePartVertex(uvSC + uvUp,			Vector3(lbl.x, ltr.y, 0.0f)));
	vertices.push_back(TexturePartVertex(uvSC + uvRt + uvUp,	Vector3(ltr.x, ltr.y, 0.0f)));
	vertices.push_back(TexturePartVertex(uvSC + uvRt,			Vector3(ltr.x, lbl.y, 0.0f)));

	//System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
	//modelViewStack.push_set(Matrix44::identity);
	Meshes::Mesh3D::render_data(_v3d,
		_context.shaderProgramInstance ? _context.shaderProgramInstance : (isBlending ? fallbackShaderProgramInstance : fallbackNoBlendShaderProgramInstance),
		nullptr, _context.renderingContext ? *_context.renderingContext : Meshes::Mesh3DRenderingContext::none(),
		vertices.get_data(), ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat);
	//modelViewStack.pop();

	if (isBlending)
	{
		_v3d->mark_disable_blend();
	}

	_v3d->set_fallback_colour();
	_v3d->set_fallback_texture();
}

void TexturePart::draw_at(::System::Video3D* _v3d, Vector3 const& _at, TexturePartDrawingContext const& _context) const
{
	Vector3 at = _at;

	bool for3D = _v3d->is_for_3d();
	if (for3D)
	{
		at += (leftBottomOffset * _context.scale).to_vector3_xz();
	}
	else
	{
		at += (leftBottomOffset * _context.scale).to_vector3();
	}

	_v3d->set_fallback_colour(_context.colour);
	if (auto* t = texture.get())
	{
		_v3d->set_fallback_texture(t->get_texture()->get_texture_id());
		if (!_context.shaderProgramInstance)
		{
			set_fallback_shader_texture(t->get_texture());
		}
	}
	else if (auto* rt = renderTarget.get())
	{
		_v3d->set_fallback_texture(rt->get_data_texture_id(renderTargetIdx));
		if (!_context.shaderProgramInstance)
		{
			set_fallback_shader_texture(rt, renderTargetIdx);
		}
	}

	bool isBlending = _context.blend.get(true);
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		isBlending = _context.blend.get(false);
	}
	if (isBlending)
	{
		_v3d->mark_enable_blend();
	}

	Vector2 atXY(at.x, for3D ? at.z : at.y);

	ArrayStatic<TexturePartVertex, 4> vertices; SET_EXTRA_DEBUG_INFO(vertices, TXT("TexturePart::draw_at[2].vertices"));
	Vector2 sizeScaled = size * _context.scale;
	Range2 range(Range(atXY.x, atXY.x + sizeScaled.x), Range(atXY.y, atXY.y + sizeScaled.y));
	Range2 uvRange(Range(uv0.x, uv1.x), Range(uv0.y, uv1.y));

	range.y = range.y.get_part(_context.verticalRange);
	uvRange.y = uvRange.y.get_part(_context.verticalRange);

	if (_context.limits.is_set() ||
		_context.showPt.is_set())
	{
		Range2 orgRange = range;
		Range2 orgUVRange = uvRange;

		if (_context.limits.is_set())
		{
			range.x.min = max(range.x.min, _context.limits.get().x.min);
			range.x.max = min(range.x.max, _context.limits.get().x.max);
			range.y.min = max(range.y.min, _context.limits.get().y.min);
			range.y.max = min(range.y.max, _context.limits.get().y.max);
		}

		if (_context.showPt.is_set())
		{
			range.x.min = atXY.x + max(range.x.min - atXY.x, _context.showPt.get().x.min * sizeScaled.x);
			range.x.max = atXY.x + min(range.x.max - atXY.x, _context.showPt.get().x.max * sizeScaled.x);
			range.y.min = atXY.y + max(range.y.min - atXY.y, _context.showPt.get().y.min * sizeScaled.y);
			range.y.max = atXY.y + min(range.y.max - atXY.y, _context.showPt.get().y.max * sizeScaled.y);
		}

		uvRange.x.min = lerp(calc_pt(orgRange.x.min, orgRange.x.max, range.x.min), orgUVRange.x.min, orgUVRange.x.max);
		uvRange.y.min = lerp(calc_pt(orgRange.y.min, orgRange.y.max, range.y.min), orgUVRange.y.min, orgUVRange.y.max);
		uvRange.x.max = lerp(calc_pt(orgRange.x.min, orgRange.x.max, range.x.max), orgUVRange.x.min, orgUVRange.x.max);
		uvRange.y.max = lerp(calc_pt(orgRange.y.min, orgRange.y.max, range.y.max), orgUVRange.y.min, orgUVRange.y.max);
	}

	Vector2 uvSC = uvRange.bottom_left();
	Vector2 uvUp = Vector2(0.0f, uvRange.y.max - uvRange.y.min);
	Vector2 uvRt = Vector2(uvRange.x.max - uvRange.x.min, 0.0f);
	if (_context.use.is_set())
	{
		an_assert(! _context.limits.is_set() &&
				  ! _context.showPt.is_set() &&
				  _context.verticalRange == Range(0.0f, 1.0f), TXT("\"use\" works only without limits, showPt and verticalRange"));
		an_assert(_context.use.get().texturePart == this, TXT("we should be using TexturePartUse for this texture!"));
		_context.use.get().get_uv_info(OUT_ uvSC, OUT_ uvUp, OUT_ uvRt);
	}

	if (for3D)
	{
		vertices.push_back(TexturePartVertex(uvSC,					 Vector3(range.x.min, at.y, range.y.min)));
		vertices.push_back(TexturePartVertex(uvSC + uvUp,			 Vector3(range.x.min, at.y, range.y.max)));
		vertices.push_back(TexturePartVertex(uvSC + uvRt + uvUp,	 Vector3(range.x.max, at.y, range.y.max)));
		vertices.push_back(TexturePartVertex(uvSC + uvRt,			 Vector3(range.x.max, at.y, range.y.min)));
	}
	else
	{
		vertices.push_back(TexturePartVertex(uvSC,					Vector3(range.x.min, range.y.min, at.z)));
		vertices.push_back(TexturePartVertex(uvSC + uvUp,			Vector3(range.x.min, range.y.max, at.z)));
		vertices.push_back(TexturePartVertex(uvSC + uvRt + uvUp,	Vector3(range.x.max, range.y.max, at.z)));
		vertices.push_back(TexturePartVertex(uvSC + uvRt,			Vector3(range.x.max, range.y.min, at.z)));
	}

	//System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
	//modelViewStack.push_set(Matrix44::identity);
	Meshes::Mesh3D::render_data(_v3d,
		_context.shaderProgramInstance ? _context.shaderProgramInstance : (isBlending ? fallbackShaderProgramInstance : fallbackNoBlendShaderProgramInstance),
		nullptr, _context.renderingContext ? *_context.renderingContext : Meshes::Mesh3DRenderingContext::none(),
		vertices.get_data(), ::Meshes::Primitive::TriangleFan, 2, *s_vertexFormat);
	//modelViewStack.pop();

	if (isBlending)
	{
		_v3d->mark_disable_blend();
	}

	_v3d->set_fallback_colour();
	_v3d->set_fallback_texture();
}

void TexturePart::set_fallback_shader_texture(::System::Texture const* _texture) const
{
	fallbackShaderProgramInstance->set_uniform(NAME(inTexture), _texture ? _texture->get_texture_id() : ::System::texture_id_none());
	fallbackNoBlendShaderProgramInstance->set_uniform(NAME(inTexture), _texture ? _texture->get_texture_id() : ::System::texture_id_none());
}

void TexturePart::set_fallback_shader_texture(::System::RenderTarget const* _rt, int _rtIdx) const
{
	fallbackShaderProgramInstance->set_uniform(NAME(inTexture), _rt ? _rt->get_data_texture_id(_rtIdx) : ::System::texture_id_none());
	fallbackNoBlendShaderProgramInstance->set_uniform(NAME(inTexture), _rt ? _rt->get_data_texture_id(_rtIdx) : ::System::texture_id_none());
}
