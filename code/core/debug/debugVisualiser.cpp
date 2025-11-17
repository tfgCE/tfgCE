#include "debugVisualiser.h"

#include "..\mesh\mesh3d.h"
#include "..\system\core.h"
#include "..\system\input.h"
#include "..\system\video\font.h"
#include "..\system\video\renderTarget.h"
#include "..\system\video\vertexFormat.h"
#include "..\system\video\video3d.h"
#include "..\system\video\viewFrustum.h"
#include "..\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

bool DebugVisualiser::s_block = false;
Concurrency::SpinLock DebugVisualiser::listLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
DebugVisualiser* DebugVisualiser::s_first = nullptr;
DebugVisualiser* DebugVisualiser::s_active = nullptr;
::System::Font* DebugVisualiser::s_defaultFont = nullptr;

Vector2 DebugVisualiser::camera = Vector2::zero;
float DebugVisualiser::zoom = 1.0f;
float DebugVisualiser::wholeScale = 0.2f;

DebugVisualiser::DebugVisualiser(String const & _title)
: title(_title)
{
	Concurrency::ScopedSpinLock lock(listLock);
	next = s_first;
	prev = nullptr;
	if (s_first) s_first->prev = this;
	s_first = this;
	font = s_defaultFont;
}

DebugVisualiser::~DebugVisualiser()
{
	deactivate();
	{
		Concurrency::ScopedSpinLock lock(listLock);
		if (next) next->prev = prev;
		if (prev) prev->next = next;
		if (nextActiveStack) nextActiveStack->prevActiveStack = prevActiveStack;
		if (prevActiveStack) prevActiveStack->nextActiveStack = nextActiveStack;
		if (s_first == this) s_first = next;
	}
	while (s_active == this)
	{
		deactivate();
	}
}

void DebugVisualiser::activate()
{
	if (is_blocked())
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(listLock);
	if (s_active == this)
	{
		return;
	}
	an_assert(nextActiveStack == nullptr, TXT("shouldn't be called active from middle of stack"));
	if (s_active)
	{
		an_assert(prevActiveStack == nullptr, TXT("assume we're on top if activating over active one"));
		s_active->on_deactivate();
		an_assert(s_active->nextActiveStack == nullptr);
		prevActiveStack = s_active;
		s_active->nextActiveStack = this;
	}
	s_active = this;
	on_activate();
}

void DebugVisualiser::deactivate()
{
	bool activatePrev = false;
	{
		Concurrency::ScopedSpinLock lock(listLock);
		if (s_active == this)
		{
			on_deactivate();
			s_active = nullptr;
			if (nextActiveStack) nextActiveStack->prevActiveStack = prevActiveStack;
			if (prevActiveStack) prevActiveStack->nextActiveStack = nextActiveStack;
			if (prevActiveStack)
			{
				// activate previous on stack
				activatePrev = true;
			}
		}
	}
	if (activatePrev)
	{
		prevActiveStack->activate();
	}
}

void DebugVisualiser::use_font(::System::Font* _font, float _fontHeight)
{
	font = _font;
	fontHeight = _fontHeight == 0.0f? 1.0f : _fontHeight;
}

void DebugVisualiser::start_gathering_data()
{
	dataLock.acquire();
}

void DebugVisualiser::clear()
{
	lines.clear();
	lines3D.clear();
	circles.clear();
	texts.clear();
	texts3D.clear();
	logs.clear();
}

void DebugVisualiser::add_line(Vector2 const & _a, Vector2 const & _b, Colour const & _colour)
{
	lines.push_back(DebugVisualiserLine(_a, _b, _colour));
}

void DebugVisualiser::add_line_3d(Vector3 const & _a, Vector3 const & _b, Colour const & _colour)
{
	lines3D.push_back(DebugVisualiserLine3D(_a, _b, _colour));
}

void DebugVisualiser::add_text_3d(Vector3 const& _a, String const & _text, Colour const& _colour)
{
	texts3D.push_back(DebugVisualiserText3D(_a, _text, _colour));
}

void DebugVisualiser::add_arrow(Vector2 const & _a, Vector2 const & _b, Colour const & _colour, float _arrowPt)
{
	lines.push_back(DebugVisualiserLine(_a, _b, _colour));
	Vector2 arrowBase = (_b - _a) * _arrowPt;
	Vector2 c = _b - arrowBase;
	Vector2 right = arrowBase.rotated_right();
	lines.push_back(DebugVisualiserLine(_b, c + right, _colour));
	lines.push_back(DebugVisualiserLine(_b, c - right, _colour));
}

void DebugVisualiser::add_circle(Vector2 const & _a, float _radius, Colour const & _colour)
{
	circles.push_back(DebugVisualiserCircle(_a, _radius, _colour));
}

void DebugVisualiser::add_range2(Range2 const & _r, Colour const& _colour)
{
	add_line(Vector2(_r.x.min, _r.y.min), Vector2(_r.x.min, _r.y.max), _colour);
	add_line(Vector2(_r.x.min, _r.y.max), Vector2(_r.x.max, _r.y.max), _colour);
	add_line(Vector2(_r.x.max, _r.y.max), Vector2(_r.x.max, _r.y.min), _colour);
	add_line(Vector2(_r.x.max, _r.y.min), Vector2(_r.x.min, _r.y.min), _colour);
}

void DebugVisualiser::add_text(Vector2 const & _a, String const & _text, Colour const & _colour, Optional<float> const& _scale, Optional<Vector2> const& _pt, Optional<VectorInt2> const& _charOffset)
{
	texts.push_back(DebugVisualiserText(_a, _text, _colour, _scale.get(1.0f), _pt.get(Vector2::half), _charOffset.get(VectorInt2::zero)));
}

void DebugVisualiser::add_log(Optional<Colour> const& _colour, String const& _text)
{
	logs.push_back(DebugVisualiserLog(_colour, _text));
}

void DebugVisualiser::end_gathering_data()
{
	dataLock.release();
}

void DebugVisualiser::on_activate()
{
	inputWasGrabbed = ::System::Input::get()->is_grabbed();
	::System::Input::get()->grab(false);
}

void DebugVisualiser::on_deactivate()
{
	::System::Input::get()->grab(inputWasGrabbed);
}

void DebugVisualiser::advance()
{
	if (is_active())
	{
		timeActive += ::System::Core::get_raw_delta_time();
		Vector2 mouse = ::System::Input::get()->get_mouse_window_location();
		Range2 screenRange(Range(0.0f, (float)::System::Video3D::get()->get_screen_size().x), Range(0.0f, (float)::System::Video3D::get()->get_screen_size().y));
		Vector2 mouseAt(screenRange.x.get_pt_from_value(mouse.x) - 0.5f,
						screenRange.y.get_pt_from_value(mouse.y) - 0.5f);
		Vector2 mouseRel(screenRange.x.get_pt_from_value(::System::Input::get()->get_mouse_relative_location().x + screenRange.x.length() * 0.5f) - 0.5f,
						 screenRange.y.get_pt_from_value(::System::Input::get()->get_mouse_relative_location().y + screenRange.y.length() * 0.5f) - 0.5f);
		// into our view size
		mouseAt *= useViewSize;
		mouseAt = mouseAt / zoom;
		mouseAt += camera;
		Vector2 mouseRelRaw = mouseRel;
		mouseRel *= useViewSize;
		mouseRel = mouseRel / zoom;
		float factor = 1.0f;
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelDown))
		{
			factor = ::System::Input::get()->is_key_pressed(::System::Key::LeftShift)? 0.5f : 0.9f;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelUp))
		{
			factor = ::System::Input::get()->is_key_pressed(::System::Key::LeftShift) ? 2.0f : 1.2f;
		}
#endif
		if (factor != 1.0f)
		{
			float prevZoom = zoom;
			zoom = zoom * factor;
			camera = camera - mouseAt;
			camera *= prevZoom / zoom;
			camera = camera + mouseAt;
		}
		if (::System::Input::get()->is_mouse_button_pressed(0))
		{
			camera -= mouseRel;
			transform3D = transform3D.to_world(Transform(-mouseRel.to_vector3(), Quat::identity));
		}

		if (::System::Input::get()->is_mouse_button_pressed(2))
		{
			transform3D = transform3D.to_world(Transform(Vector3::zero, Rotator3(mouseRelRaw.y * 200.0f, 0.0f, mouseRelRaw.x * 200.0f).to_quat()));
		}

		if (timeActive > 0.05f)
		{
			if (requestedKeyInput != ::System::Key::None)
			{
				requestedKeyBlink += ::System::Core::get_delta_time() * 1.5f;
				requestedKeyBlink = mod(requestedKeyBlink, 1.0f);
				if (::System::Input::get()->has_key_been_pressed(requestedKeyInput))
				{
					requestedKeyPressed = true;
				}
			}
			if (breakKeyInput != ::System::Key::None)
			{
				if (::System::Input::get()->has_key_been_pressed(breakKeyInput))
				{
					breakKeyPressed = true;
				}
			}
		}
	}
}

void DebugVisualiser::render()
{
	if (! title.is_empty() ||
		requestedKeyInput != ::System::Key::None ||
		breakKeyInput != ::System::Key::None)
	{
		if (auto* font = get_font())
		{
			::System::Video3D* v3d = ::System::Video3D::get();

			float messageScale = (0.25f / get_use_view_size().y);
			Vector2 at = get_use_view_size() * Vector2(0.5f, 0.1f);
			float yd = font->get_height() * messageScale;
			if (breakKeyInput != System::Key::None)
			{
				font->draw_text_at_with_border(v3d, String::printf(TXT("%S to break"), ::System::Key::to_char(breakKeyInput)), requestedKeyBlink < 0.5f ? Colour::white : Colour::red, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2(0.5f, 0.0f), false);
				at.y += yd;
			}
			if (requestedKeyInput != System::Key::None)
			{
				font->draw_text_at_with_border(v3d, String::printf(TXT("press %S"), ::System::Key::to_char(requestedKeyInput)), requestedKeyBlink < 0.5f ? Colour::red : Colour::white, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2(0.5f, 0.0f), false);
				at.y += yd;
			}
			if (! title.is_empty())
			{
				font->draw_text_at_with_border(v3d, title, Colour::cyan, Colour::black, messageScale, at, Vector2(messageScale, messageScale), Vector2(0.5f, 0.0f), false);
				at.y += yd;
			}
		}
	}
}

struct DebugVisualiserVertex
{
	Vector2 loc;
	Colour colour;

	DebugVisualiserVertex() {}
	DebugVisualiserVertex(Vector2 const & _loc, Colour const & _colour) : loc(_loc), colour(_colour){}
};

void DebugVisualiser::render_and_display()
{
	assert_rendering_thread();

	::System::Video3D * v3d = ::System::Video3D::get();

	::System::RenderTarget::bind_none();

	v3d->set_default_viewport();
	v3d->set_near_far_plane(0.02f, 100.0f);

	{
		useViewSize = viewSize;
		float v3dAR = v3d->get_aspect_ratio();
		if (v3dAR > 1.0f)
		{
			useViewSize.x = v3dAR * useViewSize.y;
		}
		else
		{
			useViewSize.x = v3dAR * useViewSize.y;
		}
	}

	v3d->set_2d_projection_matrix_ortho(useViewSize);
	v3d->access_model_view_matrix_stack().clear();

	v3d->setup_for_2d_display();

	v3d->clear_colour(backgroundColour);

	v3d->mark_disable_blend();
	v3d->depth_mask(0);
	v3d->face_display(::System::FaceDisplay::Both);

	v3d->access_model_view_matrix_stack().clear();
	v3d->access_clip_plane_stack().clear();

	v3d->set_fallback_colour();

	v3d->ready_for_rendering();

	{
		::System::VertexFormat vf;
		vf.with_location_2d();
		vf.with_colour_rgba();
		vf.no_padding();
		vf.calculate_stride_and_offsets();
		vf.used_for_dynamic_data();

		int const MAXLINES = 5000;
		ArrayStatic<DebugVisualiserVertex, MAXLINES * 2> renderLineVertices;

		{	// lock just for building data to render (while we still have data to get)
			Concurrency::ScopedSpinLock lock(dataLock);

			Range2 rect = Range2::empty;

			if (font)
			{
				for_every(text, texts)
				{
					Vector2 a = (text->a - camera) * zoom + useViewSize * 0.5f;
					float scale = text->scale * (fontHeight / font->get_height()) * zoom * wholeScale * textScale;
					Vector2 vScale(scale, scale);
					a += text->charOffset.to_vector2() * font->calculate_char_size() * vScale;
					font->draw_text_at(v3d, text->text.to_char(), text->colour, a, vScale, text->pt, false);
					rect.include(a);
				}

				for_every(text, texts3D)
				{
					Vector3 a3D = transform3D.location_to_local(text->a);
					Vector2 a(a3D.x, a3D.y);
					a = a * zoom + useViewSize * 0.5f;
					float scale = 1.0f * (fontHeight / font->get_height()) * zoom * wholeScale * textScale;
					Vector2 vScale(scale, scale);
					font->draw_text_at(v3d, text->text.to_char(), text->colour, a, vScale, Vector2::half, false);
					rect.include(a);
				}
			}

			int keepLines = lines.get_size();

			// transform circles into lines here (to consider zoom);
			for_every(circle, circles)
			{
				float deg = 0.0f;
				float sides = clamp(30.0f * sqrt(circle->radius * zoom), 10.0f, 360.0f);
				float step = 360.0f / sides;
				Vector2 p0 = circle->at + Vector2(0.0f, circle->radius).rotated_by_angle(deg);
				Vector2 p = p0;

				deg += step;
				while (deg < 360.0f)
				{
					Vector2 np = circle->at + Vector2(0.0f, circle->radius).rotated_by_angle(deg);
					add_line(p, np, circle->colour);
					deg += step;
					p = np;
				}
				add_line(p, p0, circle->colour);
			}

			for_every(line, lines3D)
			{
				Vector3 a3D = transform3D.location_to_local(line->a);
				Vector3 b3D = transform3D.location_to_local(line->b);
				Vector2 a(a3D.x, a3D.y);
				Vector2 b(b3D.x, b3D.y);
				a = a * zoom + useViewSize * 0.5f;
				b = b * zoom + useViewSize * 0.5f;
				renderLineVertices.push_back(DebugVisualiserVertex(a, line->colour));
				renderLineVertices.push_back(DebugVisualiserVertex(b, line->colour));
				if (renderLineVertices.get_size() >= MAXLINES * 2)
				{
					Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
						Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderLineVertices.get_data(), ::Meshes::Primitive::Line, renderLineVertices.get_size() / 2, vf);
					renderLineVertices.clear();
				}
			}

			for_every(line, lines)
			{
				Vector2 a = (line->a - camera) * zoom + useViewSize * 0.5f;
				Vector2 b = (line->b - camera) * zoom + useViewSize * 0.5f;
				renderLineVertices.push_back(DebugVisualiserVertex(a, line->colour));
				renderLineVertices.push_back(DebugVisualiserVertex(b, line->colour));
				if (renderLineVertices.get_size() >= MAXLINES * 2)
				{
					Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
						Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderLineVertices.get_data(), ::Meshes::Primitive::Line, renderLineVertices.get_size() / 2, vf);
					renderLineVertices.clear();
				}
				rect.include(a);
				rect.include(b);
			}

			lines.set_size(keepLines);

			// if we got something left, we already keep it in renderLineVertices array

			if (font)
			{
				Vector2 at = Vector2::zero;
				float scale = 1.0f;
				if (!rect.is_empty())
				{
					at.x = rect.x.min;
					at.y = rect.y.max;

					scale = min(rect.x.length() / 100.0f, rect.y.length() / 50.0f);
				}
				for_every(log, logs)
				{
					Vector2 a = at;
					float useScale = scale * (fontHeight / font->get_height());
					Vector2 vScale(useScale, useScale);
					font->draw_text_at(v3d, log->text.to_char(), log->colour.get(Colour::white), a, vScale, Vector2(0.0f, 1.0f), false);
					at.y -= useScale * font->get_height();
				}
			}
		}

		if (!renderLineVertices.is_empty())
		{
			Meshes::Mesh3D::render_data(v3d, nullptr, nullptr,
				Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderLineVertices.get_data(), ::Meshes::Primitive::Line, renderLineVertices.get_size() / 2, vf);
		}

		render();
	}

	v3d->set_fallback_colour();
	v3d->mark_disable_blend();

	v3d->display_buffer();
}

bool DebugVisualiser::advance_and_render_if_on_render_thread()
{
	if (Concurrency::ThreadManager::get() && Concurrency::ThreadManager::get_current_thread_id() == 0)
	{
		advance();
		render_and_display();
		return true;
	}
	else
	{
		return false;
	}
}

Vector2 DebugVisualiser::get_mouse_at() const
{
	Vector2 mouse = ::System::Input::get()->get_mouse_window_location();
	Range2 screenRange(Range(0.0f, (float)::System::Video3D::get()->get_screen_size().x), Range(0.0f, (float)::System::Video3D::get()->get_screen_size().y));
	Vector2 mouseAt(screenRange.x.get_pt_from_value(mouse.x) - 0.5f,
					screenRange.y.get_pt_from_value(mouse.y) - 0.5f);
	// into our view size
	mouseAt *= useViewSize;
	mouseAt = mouseAt / zoom;
	mouseAt += camera;
	return mouseAt;
}

void DebugVisualiser::show_and_wait_for_key(::System::Key::Type _key, ::System::Key::Type _breakKey)
{
	if (!System::Video3D::get())
	{
		return;
	}
	if (is_active() && ! is_blocked())
	{
		requestedKeyInput = _key;
		requestedKeyPressed = false;
		breakKeyInput = _breakKey;
		breakKeyPressed = false;
		while (true)
		{
			if (advance_and_render_if_on_render_thread())
			{
				// advance system core
				::System::Core::set_time_multiplier(); 
				::System::Core::advance();
			}
			else
			{
				// sleep as we don't want to stall - advancing and rendering is done somewhere else, on actual render thread?
				::System::Core::sleep(0.01f);
			}
			if (requestedKeyPressed)
			{
				break;
			}
			if (breakKeyPressed)
			{
				break;
			}
		}
		requestedKeyInput = ::System::Key::None;
		if (breakKeyPressed)
		{
			AN_BREAK;
		}
	}
}