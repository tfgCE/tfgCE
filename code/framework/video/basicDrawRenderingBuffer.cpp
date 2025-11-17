#include "basicDrawRenderingBuffer.h"

#include "texture.h"

#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\shaderProgramInstance.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define TEST_RENDER_EVERY_FRAME

//

using namespace Framework;

//

// render target outputs
DEFINE_STATIC_NAME(colour);

// shader params
DEFINE_STATIC_NAME(inTexture);

//

::System::VertexFormat* BasicDrawRenderingBuffer::s_nonSpriteVertexFormat = nullptr;

void BasicDrawRenderingBuffer::initialise_static()
{
	todo_note(TXT("update after reading main settings?"));
	s_nonSpriteVertexFormat = new ::System::VertexFormat();
	s_nonSpriteVertexFormat->with_colour_rgba().no_padding().calculate_stride_and_offsets();
}

void BasicDrawRenderingBuffer::close_static()
{
	delete_and_clear(s_nonSpriteVertexFormat);
}

#ifdef BASIC_DRAW_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING
int BasicDrawRenderingBuffer::s_forcedRerendering = 0;

void BasicDrawRenderingBuffer::force_rerendering_of_all()
{
	++s_forcedRerendering;
}
#endif

BasicDrawRenderingBuffer::BasicDrawRenderingBuffer()
{
}

BasicDrawRenderingBuffer::~BasicDrawRenderingBuffer()
{
	delete_and_clear(spriteShaderProgramInstance);
	delete_and_clear(nonSpriteShaderProgramInstance);
}

void BasicDrawRenderingBuffer::init(VectorInt2 const& _size)
{
	assert_rendering_thread();

	if (!spriteShaderProgramInstance)
	{
		spriteShaderProgramInstance = new ::System::ShaderProgramInstance();

		an_assert(::System::Video3D::get());
		spriteShaderProgramInstance->set_shader_program(::System::Video3D::get()->get_fallback_shader_program(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::WithTexture | ::System::Video3DFallbackShader::DiscardsBlending));
	}

	if (!nonSpriteShaderProgramInstance)
	{
		nonSpriteShaderProgramInstance = new ::System::ShaderProgramInstance();

		an_assert(::System::Video3D::get());
		nonSpriteShaderProgramInstance->set_shader_program(::System::Video3D::get()->get_fallback_shader_program(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::WithoutTexture | ::System::Video3DFallbackShader::DiscardsBlending));
	}

	::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(_size);

	// we use ordinary rgb 8bit textures
	rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba,
		false, // no mip maps
		System::TextureFiltering::nearest, // mag, big pixels
		System::TextureFiltering::linear // min, allow blending when smaller
	));

	renderTarget = new ::System::RenderTarget(rtSetup);
}

void BasicDrawRenderingBuffer::close()
{
	assert_rendering_thread();

	renderTarget.clear();
}

void BasicDrawRenderingBuffer::clear()
{
	for_every(e, entries)
	{
		e->requested.content = DrawCommand::None;
	}
	firstEmpty = 0;
}

void BasicDrawRenderingBuffer::prune()
{
	entries.clear();
	firstEmpty = NONE;
}

int BasicDrawRenderingBuffer::get_empty_draw_command(int _idx)
{
	int idx = _idx;
	firstEmpty = max(firstEmpty, 0);

	if (entries.is_index_valid(firstEmpty))
	{
		auto* e = &entries[firstEmpty];
		while (firstEmpty < entries.get_size())
		{
			if (e->requested.content == DrawCommand::None)
			{
				idx = firstEmpty;
				break;
			}
			++firstEmpty;
			++e;
		}
	}

	if (idx == NONE)
	{
		for_every(e, entries)
		{
			if (e->requested.content == DrawCommand::None)
			{
				e->requested = DrawCommand();
				idx = for_everys_index(e);
				break;
			}
		}
	}

	if (idx == NONE)
	{
		idx = entries.get_size();
		entries.push_back();
	}

	return idx;
}

int BasicDrawRenderingBuffer::add(Sprite const * _sprite, VectorInt2 const& _at, Optional<VectorInt2> const& _scale)
{
	int idx = get_empty_draw_command();

	set(idx, _sprite, _at, _scale);
	firstEmpty = idx + 1;

	return idx;
}

int BasicDrawRenderingBuffer::add_back(Sprite const* _sprite, VectorInt2 const& _at, Optional<VectorInt2> const& _scale)
{
	int idx = entries.get_size();
	entries.push_back();

	set(idx, _sprite, _at, _scale);
	firstEmpty = idx + 1;

	return idx;
}

void BasicDrawRenderingBuffer::set(int _idx, Sprite const* _sprite)
{
	if (_idx < 0)
	{
		return;
	}

	if (entries.is_index_valid(_idx))
	{
		auto& e = entries[_idx];
		if (e.requested.content != DrawCommand::DrawSprite)
		{
			error(TXT("non sprite used as sprite"));
		}
		else
		{
			e.requested.sprite = _sprite;
			
			firstEmpty = min(firstEmpty, _idx + 1);
		}
	}
}

void BasicDrawRenderingBuffer::set(int _idx, Sprite const* _sprite, VectorInt2 const& _at, Optional<VectorInt2> const& _scale)
{
	if (_idx < 0)
	{
		return;
	}

	if (!entries.is_index_valid(_idx))
	{
		entries.set_size(_idx + 1);
	}

	auto& e = entries[_idx];
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (e.requested.content != DrawCommand::DrawSprite &&
		e.requested.content != DrawCommand::None)
	{
		warn(TXT("using filled entry"));
	}
#endif
	e.requested.content = DrawCommand::DrawSprite;
	e.requested.sprite = _sprite;
	e.requested.at = _at;
	e.requested.scale = _scale.get(e.requested.scale);
	e.requested.stretchTo.clear();

	firstEmpty = min(firstEmpty, _idx + 1);
}

void BasicDrawRenderingBuffer::set(int _idx, VectorInt2 const& _at, Optional<VectorInt2> const& _scale)
{
	if (_idx < 0)
	{
		return;
	}

	if (!entries.is_index_valid(_idx))
	{
		entries.set_size(_idx + 1);
	}

	auto& e = entries[_idx];
	if (e.requested.content != DrawCommand::DrawSprite)
	{
		error(TXT("non sprite used as sprite"));
	}
	else
	{
		e.requested.at = _at;
		e.requested.scale = _scale.get(e.requested.scale);
		e.requested.stretchTo.clear();

		firstEmpty = min(firstEmpty, _idx);
	}
}

void BasicDrawRenderingBuffer::set_stretch_to(int _idx, VectorInt2 const & _stretchTo)
{
	if (_idx < 0)
	{
		return;
	}

	if (entries.is_index_valid(_idx))
	{
		auto& e = entries[_idx];
		if (e.requested.content != DrawCommand::DrawSprite)
		{
			error(TXT("non sprite used as sprite"));
		}
		else
		{
			e.requested.stretchTo = _stretchTo;
		}
	}
}

void BasicDrawRenderingBuffer::clear(int _idx)
{
	if (entries.is_index_valid(_idx))
	{
		auto& e = entries[_idx];
		e.requested.content = DrawCommand::None;

		firstEmpty = min(firstEmpty, _idx);
	}
}

int BasicDrawRenderingBuffer::draw_point(Colour const& _colour, VectorInt2 const& _at, Optional<int> const& _idx)
{
	int idx = _idx.get(NONE);
	if (idx == NONE)
	{
		idx = get_empty_draw_command();
	}

	auto& e = entries[idx];
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (e.requested.content != DrawCommand::DrawPoint &&
		e.requested.content != DrawCommand::None)
	{
		warn(TXT("using filled entry"));
	}
#endif
	
	e.requested.content = DrawCommand::DrawPoint;
	e.requested.colour = _colour;
	e.requested.at = _at;

	firstEmpty = min(firstEmpty, idx + 1);

	return idx;
}

int BasicDrawRenderingBuffer::draw_line(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, Optional<int> const& _idx)
{
	int idx = _idx.get(NONE);
	if (idx == NONE)
	{
		idx = get_empty_draw_command();
	}

	auto& e = entries[idx];
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (e.requested.content != DrawCommand::DrawLine &&
		e.requested.content != DrawCommand::None)
	{
		warn(TXT("using filled entry"));
	}
#endif
	
	e.requested.content = DrawCommand::DrawLine;
	e.requested.colour = _colour;
	e.requested.at = _at;
	e.requested.to = _to;

	firstEmpty = min(firstEmpty, idx + 1);

	return idx;
}

int BasicDrawRenderingBuffer::draw_rect(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, Optional<int> const& _idx)
{
	int idx = _idx.get(NONE);
	if (idx == NONE)
	{
		idx = get_empty_draw_command();
	}

	auto& e = entries[idx];
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (e.requested.content != DrawCommand::DrawRect &&
		e.requested.content != DrawCommand::None)
	{
		warn(TXT("using filled entry"));
	}
#endif
	
	e.requested.content = DrawCommand::DrawRect;
	e.requested.colour = _colour;
	e.requested.at = _at;
	e.requested.to = _to;
	e.requested.stretchTo.clear();

	firstEmpty = min(firstEmpty, idx + 1);

	return idx;
}

int BasicDrawRenderingBuffer::draw_rect_fill(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, Optional<int> const& _idx)
{
	int idx = _idx.get(NONE);
	if (idx == NONE)
	{
		idx = get_empty_draw_command();
	}

	auto& e = entries[idx];
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (e.requested.content != DrawCommand::DrawRectFill &&
		e.requested.content != DrawCommand::None)
	{
		warn(TXT("using filled entry"));
	}
#endif
	
	e.requested.content = DrawCommand::DrawRectFill;
	e.requested.colour = _colour;
	e.requested.at = _at;
	e.requested.to = _to;

	firstEmpty = min(firstEmpty, idx + 1);

	return idx;
}

int BasicDrawRenderingBuffer::draw_tri(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, VectorInt2 const& _to2, Optional<int> const& _idx)
{
	int idx = _idx.get(NONE);
	if (idx == NONE)
	{
		idx = get_empty_draw_command();
	}

	auto& e = entries[idx];
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (e.requested.content != DrawCommand::DrawTri &&
		e.requested.content != DrawCommand::None)
	{
		warn(TXT("using filled entry"));
	}
#endif
	
	e.requested.content = DrawCommand::DrawTri;
	e.requested.colour = _colour;
	e.requested.at = _at;
	e.requested.to = _to;
	e.requested.to2 = _to2;

	firstEmpty = min(firstEmpty, idx + 1);

	return idx;
}

int BasicDrawRenderingBuffer::draw_tri_fill(Colour const& _colour, VectorInt2 const& _at, VectorInt2 const& _to, VectorInt2 const& _to2, Optional<int> const& _idx)
{
	int idx = _idx.get(NONE);
	if (idx == NONE)
	{
		idx = get_empty_draw_command();
	}

	auto& e = entries[idx];
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (e.requested.content != DrawCommand::DrawTriFill &&
		e.requested.content != DrawCommand::None)
	{
		warn(TXT("using filled entry"));
	}
#endif
	
	e.requested.content = DrawCommand::DrawTriFill;
	e.requested.colour = _colour;
	e.requested.at = _at;
	e.requested.to = _to;
	e.requested.to2 = _to2;

	firstEmpty = min(firstEmpty, idx + 1);

	return idx;
}

void BasicDrawRenderingBuffer::update(::System::Video3D* _v3d)
{
	assert_rendering_thread();

	// check if we need to render anything
#ifdef BASIC_DRAW_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING
	if (forcedRerendering == s_forcedRerendering)
#endif
#ifdef TEST_RENDER_EVERY_FRAME
	if (false)
#endif
	{
		bool renderingRequired = false;

		int drawCommandsToRender = 0;

		for_every(s, entries)
		{
			if (s->requested != s->rendered)
			{
				renderingRequired = true;
				break;
			}
			if (s->requested.sprite)
			{
				++drawCommandsToRender;
			}
		}

		renderingRequired |= drawCommandsToRender != drawCommandsRendered;

		if (!renderingRequired)
		{
			return;
		}
	}
#ifdef BASIC_DRAW_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING
	forcedRerendering = s_forcedRerendering;
#endif

	// we will be rendering
	auto* v3d = ::System::Video3D::get();
	::System::ForRenderTargetOrNone forRT(renderTarget.get());
	forRT.bind();

	// prepare and clear render target
	{
		VectorInt2 rtSize = forRT.get_size();
		v3d->set_viewport(VectorInt2::zero, rtSize);
		v3d->set_2d_projection_matrix_ortho(rtSize.to_vector2());
		v3d->access_model_view_matrix_stack().clear();
		v3d->access_clip_plane_stack().clear();
		v3d->set_near_far_plane(0.02f, 100.0f);
		v3d->setup_for_2d_display();
		v3d->ready_for_rendering();

		v3d->clear_colour_depth_stencil(Colour::none);
	}

	// go through all sprites, render them grouped by texture
	an_assert(spriteVertices.is_empty());
	Texture* lastTexture = nullptr;
	DrawCommand::Content lastContent = DrawCommand::None;

	int renderedNow = 0;

	SpriteDrawingData data;
	SpriteDrawingContext context;
	for_every(s, entries)
	{
		if (s->requested.content == DrawCommand::DrawSprite)
		{
			if (s->requested.sprite)
			{
				++renderedNow;
				context.scale = s->requested.scale;
				context.stretchTo = s->requested.stretchTo;
				s->requested.sprite->prepare_to_draw_at(OUT_ data, s->requested.at, context);
				if (data.texture)
				{
					// render last if we change
					{
						if (lastContent != DrawCommand::DrawSprite)
						{
							render_non_sprite_vertices_and_clear(_v3d, lastContent);
						}
						if (lastTexture != data.texture)
						{
							render_sprite_vertices_and_clear(_v3d, lastTexture);
						}
					}
					// update content to draw
					{
						lastContent = DrawCommand::DrawSprite;
						lastTexture = data.texture;
					}
					// add actual content (vertices)
					{
						an_assert(data.vertices.get_size() == 4);
						// 012
						spriteVertices.push_back(data.vertices[0]);
						spriteVertices.push_back(data.vertices[1]);
						spriteVertices.push_back(data.vertices[2]);
						// 023
						spriteVertices.push_back(data.vertices[0]);
						spriteVertices.push_back(data.vertices[2]);
						spriteVertices.push_back(data.vertices[3]);
					}
				}
			}
		}
		else
		{
			DrawCommand::Content drawAsContent = s->requested.content;
			// get actual content type to draw
			{
				if (s->requested.content == DrawCommand::DrawRect ||
					s->requested.content == DrawCommand::DrawTri)
				{
					drawAsContent = DrawCommand::DrawLine;
				}
				if (s->requested.content == DrawCommand::DrawRectFill)
				{
					drawAsContent = DrawCommand::DrawTriFill;
				}
			}
			// render last if we change
			{
				if (lastContent != drawAsContent)
				{
					render_non_sprite_vertices_and_clear(_v3d, lastContent);
				}
				if (lastTexture != nullptr)
				{
					render_sprite_vertices_and_clear(_v3d, lastTexture);
				}
			}
			// update content to draw
			{
				lastContent = drawAsContent;
				lastTexture = nullptr;
			}
			// add actual content (vertices)
			{
				if (s->requested.content == DrawCommand::DrawPoint)
				{
					Vector3 a = (s->requested.at.to_vector2() + Vector2::half).to_vector3();
					an_assert(drawAsContent == DrawCommand::DrawPoint);
					nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, a));
				}
				if (s->requested.content == DrawCommand::DrawLine)
				{
					Vector3 a = (s->requested.at.to_vector2() + Vector2::half).to_vector3();
					Vector3 b = (s->requested.to.to_vector2() + Vector2::half).to_vector3();
					an_assert(drawAsContent == DrawCommand::DrawLine); 
					nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, a));
					nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, b));
					//
					// draw it backwards to make sure the last pixel is visible
					nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, b));
					nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, a));
				}
				if (s->requested.content == DrawCommand::DrawRect ||
					s->requested.content == DrawCommand::DrawRectFill)
				{
					Vector3 a = (s->requested.at.to_vector2() + Vector2::half).to_vector3();
					Vector3 b = (s->requested.to.to_vector2() + Vector2::half).to_vector3();
					if (s->requested.content == DrawCommand::DrawRect)
					{
						an_assert(drawAsContent == DrawCommand::DrawLine);

						Vector3 p[4];
						p[0] = Vector3(a.x, a.y, 0.0f);
						p[1] = Vector3(a.x, b.y, 0.0f);
						p[2] = Vector3(b.x, b.y, 0.0f);
						p[3] = Vector3(b.x, a.y, 0.0f);

						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[0]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[1]));
						//
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[1]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[2]));
						//
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[2]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[3]));
						//
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[3]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[0]));
					}
					else
					{
						an_assert(s->requested.content == DrawCommand::DrawRectFill);
						an_assert(drawAsContent == DrawCommand::DrawTriFill);

						// draw extra last pixel
						if (a.x <= b.x) b.x += 1.0f; else a.x += 1.0f;
						if (a.y <= b.y) b.y += 1.0f; else a.y += 1.0f;

						Vector3 p[4];
						p[0] = Vector3(a.x, a.y, 0.0f);
						p[1] = Vector3(a.x, b.y, 0.0f);
						p[2] = Vector3(b.x, b.y, 0.0f);
						p[3] = Vector3(b.x, a.y, 0.0f);

						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[0]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[1]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[2]));
						//
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[2]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[3]));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, p[0]));
					}
				}
				if (s->requested.content == DrawCommand::DrawTri ||
					s->requested.content == DrawCommand::DrawTriFill)
				{
					Vector3 a = (s->requested.at.to_vector2() + Vector2::half).to_vector3();
					Vector3 b = (s->requested.to.to_vector2() + Vector2::half).to_vector3();
					Vector3 c = (s->requested.to2.to_vector2() + Vector2::half).to_vector3();
					if (s->requested.content == DrawCommand::DrawTri)
					{
						an_assert(drawAsContent == DrawCommand::DrawLine);
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, a));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, b));
						//
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, b));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, c));
						//
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, c));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, a));
					}
					else
					{
						an_assert(s->requested.content == DrawCommand::DrawTri);
						an_assert(drawAsContent == DrawCommand::DrawTriFill);
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, a));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, b));
						nonSpriteVertices.push_back(NonSpriteVertex(s->requested.colour, c));
					}
				}
			}
		}
		s->rendered = s->requested;
	}

	if (lastContent == DrawCommand::DrawSprite)
	{
		render_sprite_vertices_and_clear(_v3d, lastTexture);
	}
	else if (lastContent != DrawCommand::None)
	{
		render_non_sprite_vertices_and_clear(_v3d, lastContent);
	}

	drawCommandsRendered = renderedNow;

	// we're done rendering
	forRT.unbind();
}

void BasicDrawRenderingBuffer::render_sprite_vertices_and_clear(::System::Video3D* _v3d, Texture* _texture)
{
	if (spriteVertices.is_empty())
	{
		return;
	}
	if (!_texture)
	{
		spriteVertices.clear();
		return;
	}

	//

	_v3d->set_fallback_colour(Colour::white);

	{
		auto tId = _texture->get_texture()->get_texture_id();
		_v3d->set_fallback_texture(tId);
		spriteShaderProgramInstance->set_uniform(NAME(inTexture), tId);
	}

	Meshes::Mesh3D::render_data(_v3d, spriteShaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext::none(), spriteVertices.get_data(), ::Meshes::Primitive::Triangle, spriteVertices.get_size() / 3, *Sprite::get_vertex_format());

	_v3d->set_fallback_colour();
	_v3d->set_fallback_texture();

	//

	spriteVertices.clear();
}

void BasicDrawRenderingBuffer::render_non_sprite_vertices_and_clear(::System::Video3D* _v3d, DrawCommand::Content _content)
{
	if (nonSpriteVertices.is_empty())
	{
		return;
	}
	if (_content == DrawCommand::None)
	{
		nonSpriteVertices.clear();
		return;
	}

	//

	_v3d->set_fallback_colour(Colour::white);

	if (_content == DrawCommand::DrawPoint)
	{
		Meshes::Mesh3D::render_data(_v3d, nonSpriteShaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext::none(), nonSpriteVertices.get_data(), ::Meshes::Primitive::Point, nonSpriteVertices.get_size(), *get_non_sprite_vertex_format());
	}
	else if (_content == DrawCommand::DrawLine)
	{
		Meshes::Mesh3D::render_data(_v3d, nonSpriteShaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext::none(), nonSpriteVertices.get_data(), ::Meshes::Primitive::Line, nonSpriteVertices.get_size() / 2, *get_non_sprite_vertex_format());
	}
	else if (_content == DrawCommand::DrawTriFill)
	{
		Meshes::Mesh3D::render_data(_v3d, nonSpriteShaderProgramInstance, nullptr, Meshes::Mesh3DRenderingContext::none(), nonSpriteVertices.get_data(), ::Meshes::Primitive::Triangle, nonSpriteVertices.get_size() / 3, *get_non_sprite_vertex_format());
	}

	_v3d->set_fallback_colour();
	_v3d->set_fallback_texture();

	//

	nonSpriteVertices.clear();
}

//--

bool BasicDrawRenderingBuffer::DrawCommand::operator== (DrawCommand const& _other) const
{
	if (content == _other.content)
	{
		if (content == DrawSprite)
		{
			return sprite == _other.sprite && (!sprite || (at == _other.at && scale == _other.scale && stretchTo == _other.stretchTo));
		}
		if (content == DrawPoint)
		{
			return colour == _other.colour && at == _other.at;
		}
		else
		{
			return colour == _other.colour && at == _other.at && to == _other.to;
		}
	}
	else
	{
		return false;
	}
}