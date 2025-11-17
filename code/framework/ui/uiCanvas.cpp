#include "uiCanvas.h"

#include "uiCanvasElement.h"
#include "uiCanvasInputContext.h"
#include "uiCanvasRenderContext.h"
#include "uiCanvasUpdateWorktable.h"
#include "uiCanvasWindow.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\system\core.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\video3dPrimitives.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"
#include "..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define CLICK_TIME 0.2f
#define DOUBLE_CLICK_UP_TIME 0.2f

//

using namespace Framework;
using namespace UI;

//

// library tags
DEFINE_STATIC_NAME_STR(fontTags, TXT("uiCanvas"));

//

Canvas::Canvas()
: base(this)
, elements(this)
{
	logicalSize = Vector2(320.0f, 200.0f);
	realSize = Vector2(320.0f, 200.0f);
	realAnchor = Vector2::zero;

	fonts.clear();
	{
		if (auto* lib = Framework::Library::get_current())
		{
			TagCondition tagged;
			tagged.load_from_string(NAME(fontTags).to_string());
			for_every_ptr(f, lib->get_fonts().get_tagged(tagged))
			{
				fonts.push_back(); fonts.get_last().font = f;
			}
		}
		if (fonts.is_empty())
		{
			error(TXT("didn't find any font tagged \"%S\""), NAME(fontTags).to_char());
		}
		for_every(font, fonts)
		{
			font->size = font->font->calculate_char_size();
		}
	}
}

Canvas::~Canvas()
{
	make_safe_object_unavailable();
}

void Canvas::clear()
{
	Concurrency::ScopedSpinLock l(lock, true);

	elements.clear();
}

void Canvas::render(CanvasRenderContext & _context) const
{
	Concurrency::ScopedSpinLock l(lock, true);

	elements.update_for_canvas(this);
	elements.render(_context);
}

void Canvas::add(ICanvasElement* _element)
{
	Concurrency::ScopedSpinLock l(lock, true);

	elements.add(_element);
}

void Canvas::update_convertions()
{
	logicalToRealSize.x = logicalSize.x != 0.0f ? realSize.x / logicalSize.x : 0.0f;
	logicalToRealSize.y = logicalSize.y != 0.0f ? realSize.y / logicalSize.y : 0.0f;
}

Vector2 Canvas::logical_to_real_location(Vector2 const & _location) const
{
	return _location * logicalToRealSize + realAnchor;
}

Vector2 Canvas::logical_to_real_vector(Vector2 const & _vector) const
{
	return _vector * logicalToRealSize;
}

Vector2 Canvas::logical_to_real_size(Vector2 const & _size) const
{
	return _size * logicalToRealSize;
}

float Canvas::logical_to_real_height(float _height) const
{
	return _height * logicalToRealSize.y;
}

Vector2 Canvas::real_to_logical_location(Vector2 const & _location) const
{
	return (_location - realAnchor) / logicalToRealSize;
}

Vector2 Canvas::real_to_logical_vector(Vector2 const & _vector) const
{
	return (_vector) / logicalToRealSize;
}

Vector2 Canvas::real_to_logical_size(Vector2 const & _size) const
{
	return _size / logicalToRealSize;
}

float Canvas::real_to_logical_height(float _height) const
{
	return _height / logicalToRealSize.y;
}

void Canvas::render_on(::System::Video3D* _v3d, ::System::RenderTarget* _rt, Optional<Colour> const& _backgroundColour)
{
	Concurrency::ScopedSpinLock l(lock, true);

	Vector2 realSizeCopy = realSize;
	Vector2 realAnchorCopy = realAnchor;

	{
		Vector2 rtSize = _rt? _rt->get_size().to_vector2() : _v3d->get_screen_size().to_vector2();
		Vector2 at;
		at.x = rtSize.x * 0.5f;
		at.y = rtSize.y * 0.5f;
		if (_rt)
		{
			_rt->bind();
		}
		else
		{
			System::RenderTarget::bind_none();
		}

		Vector2 newRtSize = maintain_aspect_ratio_keep_y(rtSize, get_logical_size());
		_v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2()); // ?? we then assume that we will do everything with at and newRtSize
		_v3d->set_near_far_plane(0.01f, 1000.0f);
		_v3d->set_2d_projection_matrix_ortho(rtSize);
		_v3d->access_model_view_matrix_stack().clear();
		_v3d->access_clip_plane_stack().clear();
		_v3d->setup_for_2d_display();
		_v3d->ready_for_rendering();

		CanvasRenderContext canvasRenderContext(_v3d, this);
		set_real(at - newRtSize * 0.5f, newRtSize);

		RangeInt2 prevScissors = _v3d->get_scissors();
		bool renderDueToScissors = true;
		{
			Range2 r = Range2::empty;
			r.include(at - newRtSize * 0.5f);
			r.include(at + newRtSize * 0.5f);
			if (_backgroundColour.is_set())
			{
				::System::Video3DPrimitives::rect_2d(_backgroundColour.get(), r);
			}
			{
				RangeInt2 ri = RangeInt2::empty;
				ri.include(TypeConversions::Normal::f_i_cells(r.bottom_left()));
				ri.include(TypeConversions::Normal::f_i_cells(r.top_right()));
				if (!prevScissors.is_empty())
				{
					ri.intersect_with(prevScissors);
					renderDueToScissors = !ri.is_empty();
				}
				_v3d->set_scissors(ri);
			}
		}

		if (renderDueToScissors)
		{
			render(canvasRenderContext);
		}

		if (_rt)
		{
			_rt->unbind();
		}

		_v3d->set_scissors(prevScissors);
	}

	// restore
	set_real(realAnchorCopy, realSizeCopy);
}

void Canvas::render_on_output(bool _doesUseVR, ::System::Video3D* _v3d, ::System::RenderTarget* _rt, Optional<Colour> const& _backgroundColour)
{
	Concurrency::ScopedSpinLock l(lock, true);

	Vector2 realSizeCopy = realSize;
	Vector2 realAnchorCopy = realAnchor;
	for (int eyeIdx = 0; eyeIdx < (_doesUseVR ? 2 : 1); ++eyeIdx)
	{
		Vector2 rtSize = _rt ? _rt->get_size().to_vector2() : _v3d->get_screen_size().to_vector2();
		Vector2 at;
		at.x = rtSize.x * 0.5f;
		at.y = rtSize.y * 0.5f;
		VR::Eye::Type eye = (VR::Eye::Type)eyeIdx;
		bool rtBound = false;
		if (_doesUseVR && VR::IVR::get()->get_output_render_target(eye))
		{
			VR::IVR::get()->get_output_render_target(eye)->bind();
			rtSize = VR::IVR::get()->get_output_render_target(eye)->get_size().to_vector2();
			at.x = rtSize.x * 0.5f;
			at.y = rtSize.y * 0.5f;
			at += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
		}
		else
		{
			if (_rt)
			{
				_rt->bind();
				rtBound = true;
			}
			else
			{
				System::RenderTarget::bind_none();
			}
		}

		Vector2 newRtSize = maintain_aspect_ratio_keep_y(rtSize, get_logical_size());
		_v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2()); // ?? we then assume that we will do everything with at and newRtSize
		_v3d->set_2d_projection_matrix_ortho(rtSize);
		_v3d->access_model_view_matrix_stack().clear();
		_v3d->access_clip_plane_stack().clear();
		_v3d->setup_for_2d_display();
		_v3d->ready_for_rendering();

		CanvasRenderContext canvasRenderContext(_v3d, this);
		set_real(at - newRtSize * 0.5f, newRtSize);

		RangeInt2 prevScissors = _v3d->get_scissors();
		bool renderDueToScissors = true;
		{
			Range2 r = Range2::empty;
			r.include(at - newRtSize * 0.5f);
			r.include(at + newRtSize * 0.5f);
			if (_backgroundColour.is_set())
			{
				::System::Video3DPrimitives::rect_2d(_backgroundColour.get(), r);
			}
			{
				RangeInt2 ri = RangeInt2::empty;
				ri.include(TypeConversions::Normal::f_i_cells(r.bottom_left()));
				ri.include(TypeConversions::Normal::f_i_cells(r.top_right()));
				if (!prevScissors.is_empty())
				{
					ri.intersect_with(prevScissors);
					renderDueToScissors = !ri.is_empty();
				}
				_v3d->set_scissors(ri);
			}
		}

		if (renderDueToScissors)
		{
			render(canvasRenderContext);
		}

		if (_rt)
		{
			_rt->unbind();
		}

		_v3d->set_scissors(prevScissors);
	}

	// restore
	set_real(realAnchorCopy, realSizeCopy);
}

CanvasFont const* Canvas::get_font_for_logical_size(float _size) const
{
	return get_font_for_real_size(_size * logicalToRealSize.y);
}

CanvasFont const* Canvas::get_font_for_real_size(float _size) const
{
	CanvasFont const * best = nullptr;
	float bestDiff = 0.0f;
	for_every(font, fonts)
	{
		float diff = font->size.y - _size; // positive value if we're to use larger font
		if (diff < 0.0f)
		{
			diff = -diff * 10.0f; // if we want to use smaller font, give penalty, prefer larger, unless quite close
		}
		if (!best || bestDiff > diff)
		{
			bestDiff = diff;
			best = font;
		}
	}

	return best;
}

void Canvas::update(CanvasInputContext const& _cic)
{
	Concurrency::ScopedSpinLock l(lock, true);

	elements.update_for_canvas(this);

	CanvasUpdateWorktable cuw(this);

	elements.process_shortcuts(_cic, cuw);
	if (auto* shortcutElement = cuw.get_shortcut_to_activate())
	{
		shortcutElement->execute_shortcut(_cic, cuw);
		return;
	}

	elements.process_controls(_cic, cuw);

	cursors.set_size(max(cursors.get_size(), _cic.get_cursors().get_size()));

	bool anyButtonStartedHoldOutsideCanvas = false;
	// force to hover if holding, so we won't change
	for_every(c, cursors)
	{
		cuw.hovers_on(for_everys_index(c), c->holding.get());
		int idx = for_everys_index(c);
		c->buttons.set_size(3);
		if (_cic.get_cursors().is_index_valid(idx))
		{
			c->prevAt = c->at;
			auto& cicc = _cic.get_cursors()[idx];
			c->at = cicc.at;
			for_every(b, c->buttons)
			{
				b->wasDown = b->isDown;
				b->isDown = is_flag_set(cicc.buttonFlags, bit(for_everys_index(b)));
				if (b->isDown && !b->wasDown)
				{
					b->startedHoldOutsideCanvas = ! c->hoversOn.get();
				}
				else if (! b->isDown)
				{
					b->startedHoldOutsideCanvas = false;
				}
			}
		}
		else
		{
			c->prevAt.clear();
			c->at.clear();
			for_every(b, c->buttons)
			{
				b->wasDown = b->isDown;
				b->isDown = false;
				b->startedHoldOutsideCanvas = false;
			}
		}

		for_every(b, c->buttons)
		{
			anyButtonStartedHoldOutsideCanvas |= b->startedHoldOutsideCanvas;
		}
	}

	if (!anyButtonStartedHoldOutsideCanvas)
	{
		elements.update_hover_on(_cic, cuw);
	}
	modalWindowActive = cuw.get_in_modal_window();

	cursors.set_size(max(cursors.get_size(), cuw.get_hovers_on().get_size()));
	for_every(c, cursors)
	{
		int idx = for_everys_index(c);
		if (cuw.get_hovers_on().is_index_valid(idx))
		{
			c->hoversOn = cuw.get_hovers_on()[idx];
		}
		else
		{
			c->hoversOn.clear();
		}
	}

	float deltaTime = ::System::Core::get_delta_time();

	for_every(c, cursors)
	{
		for_every(b, c->buttons)
		{
			b->downTime += deltaTime;
			b->upTime += deltaTime;
			if (b->isDown && !b->wasDown)
			{
				if (auto* e = c->hoversOn.get())
				{
					{
						auto* w = fast_cast<CanvasWindow>(e);
						if (!w)
						{
							w = e->get_in_window();
						}
						if (w)
						{
							w->window_to_top();
						}
					}
					{
						ICanvasElement::HoldContext context(this);
						context.at = c->at;
						e->on_hold(context);
						if (context.isHolding)
						{
							c->holding = e;
							c->holdingCustomId = context.holdingCustomId;
						}
					}
					if (!c->holding.get())
					{
						bool doNormalPress = true;
						if (b->downTime < CLICK_TIME &&
							b->upTime < DOUBLE_CLICK_UP_TIME)
						{
							ICanvasElement::ClickContext context(this);
							context.at = c->at;

							if (e->on_double_click(context))
							{
								doNormalPress = false;
							}
						}
						if (doNormalPress)
						{
							ICanvasElement::PressContext context(this);
							context.at = c->at;
							e->on_press(context);
						}
					}
				}
				b->downTime = 0.0f;
			}
			else if (!b->isDown && b->wasDown)
			{
				c->holding.clear();
				if (b->downTime < CLICK_TIME)
				{
					if (auto* e = c->hoversOn.get())
					{
						ICanvasElement::ClickContext context(this);
						context.at = c->at;
						e->on_click(context);
					}
				}
				b->upTime = 0.0f;
			}
			else if (b->isDown)
			{
				if (auto* e = c->holding.get())
				{
					ICanvasElement::HoldContext context(this);
					context.at = c->at;
					context.isHolding = true;
					context.holdingCustomId = c->holdingCustomId;
					if (c->at.is_set() &&
						c->prevAt.is_set())
					{
						context.movedBy = c->at.get() - c->prevAt.get();
					}
					e->on_hold(context);
					if (! context.isHolding)
					{
						c->holding.clear();
					}
				}
			}
		}
	}
}

bool Canvas::does_hover_on(ICanvasElement const* _element) const
{
	// lock might be not needed - we won't crash, we may just give wrong result
	//Concurrency::ScopedSpinLock l(lock, true);

	for_every(c, cursors)
	{
		if (c->hoversOn.get() == _element)
		{
			return true;
		}
	}

	return false;
}

bool Canvas::does_hover_on_or_inside(ICanvasElement const* _element) const
{
	// lock might be not needed - we won't crash, we may just give wrong result
	//Concurrency::ScopedSpinLock l(lock, true);

	for_every(c, cursors)
	{
		if (auto* e = c->hoversOn.get())
		{
			if (e->is_part_of(_element))
			{
				return true;
			}
		}
	}

	return false;
}

bool Canvas::is_cursor_available_outside_canvas(int _cursorIdx) const
{
	if (modalWindowActive)
	{
		return false;
	}

	if (cursors.is_index_valid(_cursorIdx))
	{
		auto& cursor = cursors[_cursorIdx];
		for_every(b, cursor.buttons)
		{
			if (b->startedHoldOutsideCanvas)
			{
				return true;
			}
		}
		if (!cursor.hoversOn.get() &&
			!cursor.holding.get())
		{
			return true;
		}

		return false;
	}
	return true;
}

ICanvasElement* Canvas::find_by_id(Name const& _id, Optional<int> const& _additionalId) const
{
	Concurrency::ScopedSpinLock l(lock, true);
	
	return elements.find_by_id(_id, _additionalId);
}
