#include "uiCanvasElement.h"

#include "uiCanvas.h"
#include "uiCanvasElements.h"
#include "uiCanvasUpdateWorktable.h"
#include "uiCanvasWindow.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace UI;

//

REGISTER_FOR_FAST_CAST(ICanvasElement);

ICanvasElement::ICanvasElement()
: SafeObject<ICanvasElement>(nullptr)
{
	on_get_element();
}

ICanvasElement::~ICanvasElement()
{
	on_release_element();
}

void ICanvasElement::to_top()
{
	if (inElements)
	{
		inElements->to_top(this);
	}
}

void ICanvasElement::window_to_top()
{
	if (inElements)
	{
		if (auto* e = inElements->get_in_element())
		{
			e->window_to_top();
		}
	}
}

void ICanvasElement::remove()
{
	if (inElements)
	{
		inElements->remove(this);
	}
}

void ICanvasElement::on_get_element()
{
	if (!is_available_as_safe_object())
	{
		make_safe_object_available(this);
	}

	id = Name::invalid();
	additionalId = 0;

	inElements = nullptr;

	shortcuts.clear();
}

void ICanvasElement::on_release_element()
{
	if (is_available_as_safe_object())
	{
		make_safe_object_unavailable();
	}
}

ICanvasElement* ICanvasElement::find_by_id(Name const& _id, Optional<int> const& _additionalId)
{
	if (id == _id &&
		_additionalId.get(additionalId) == additionalId)
	{
		return this;
	}
	return nullptr;
}

Canvas* ICanvasElement::get_in_canvas() const
{
	if (inElements)
	{
		return inElements->get_in_canvas();
	}
	return nullptr;
}

bool ICanvasElement::is_part_of(ICanvasElement const* _e) const
{
	ICanvasElement const* e = inElements->get_in_element();
	while (e)
	{
		if (e == _e)
		{
			return true;
		}
		e = e->inElements->get_in_element();
	}
	return false;
}

CanvasWindow* ICanvasElement::get_in_window() const
{
	ICanvasElement const * e = inElements->get_in_element();
	while (e)
	{
		if (auto* w = fast_cast<CanvasWindow>(e))
		{
			return cast_to_nonconst(w);
		}
		e = e->inElements->get_in_element();
	}
	return nullptr;
}

Vector2 const & ICanvasElement::get_global_offset() const
{
	if (inElements)
	{
		return inElements->get_global_offset();
	}
	else
	{
		return Vector2::zero;
	}
}
ICanvasElement* ICanvasElement::clear_shortcuts()
{
	shortcuts.clear();
	return this;
}

ICanvasElement* ICanvasElement::add_shortcut(System::Key::Type _key, Optional<ShortcutParams> const& _params, std::function<void(ActContext const&)> _perform, String const& _info)
{
	shortcuts.push_back();
	auto& shortcut = shortcuts.get_last();
	{
		shortcut.key = _key;
		if (_params.is_set())
		{
			shortcut.params = _params.get();
		}
		shortcut.perform = _perform;

		shortcut.info = String::empty();
		if (shortcut.params.ctrl)
		{
			if (!shortcut.info.is_empty()) { shortcut.info += TXT("+"); }
			shortcut.info += TXT("ctrl");
		}
		if (shortcut.params.alt)
		{
			if (!shortcut.info.is_empty()) { shortcut.info += TXT("+"); }
			shortcut.info += TXT("alt");
		}
		if (shortcut.params.shift)
		{
			if (!shortcut.info.is_empty()) { shortcut.info += TXT("+"); }
			shortcut.info += TXT("shift");
		}
#ifdef AN_STANDARD_INPUT
		if (shortcut.params.shift && (_key == System::Key::LeftShift || _key == System::Key::RightShift))
		{
			// ignore and do not add the key afterwards
		}
		else if (shortcut.params.alt && (_key == System::Key::LeftAlt || _key == System::Key::RightAlt))
		{
			// ignore and do not add the key afterwards
		}
		else if (shortcut.params.ctrl && (_key == System::Key::LeftCtrl || _key == System::Key::RightCtrl))
		{
			// ignore and do not add the key afterwards
		}
		else
#endif
		{
			if (!shortcut.info.is_empty()) { shortcut.info += TXT("+"); }
			shortcut.info += ::System::Key::to_ui_char(_key);
		}
	}
	
	return this;
}

void ICanvasElement::execute_shortcut(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	if (shortcuts.is_index_valid(_cuw.get_shortcut_index_to_activate()))
	{
		auto& shortcut = shortcuts[_cuw.get_shortcut_index_to_activate()];
		if (shortcut.perform)
		{
			ActContext context;
			prepare_context(OUT_ context, _cuw.get_canvas(), NP);
			shortcut.perform(context);
		}
	}
}

bool ICanvasElement::process_shortcuts(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	if (! shortcuts.is_empty())
	{
		if (auto* input = ::System::Input::get())
		{
			bool ctrlPressed = false;
			bool altPressed = false;
			bool shiftPressed = false;
#ifdef AN_STANDARD_INPUT
			ctrlPressed = input->is_key_pressed(System::Key::LeftCtrl) || input->is_key_pressed(System::Key::RightCtrl);
			altPressed = input->is_key_pressed(System::Key::LeftAlt) || input->is_key_pressed(System::Key::RightAlt);
			shiftPressed = input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift);
#endif

			for_every(shortcut, shortcuts)
			{
				if (!shortcut->params.notToBeUsed &&
					ctrlPressed == shortcut->params.ctrl &&
					altPressed == shortcut->params.alt &&
					shiftPressed == shortcut->params.shift)
				{
					bool ok = false;
					if (shortcut->params.handleHoldAsPresses)
					{
						if (input->has_key_been_hold_pressed(shortcut->key))
						{
							ok = true;
						}
					}
					else
					{
						if ((!shortcut->params.actOnRelease && input->has_key_been_pressed(shortcut->key)) ||
							(shortcut->params.actOnRelease && input->has_key_been_released(shortcut->key)))
						{
							ok = true;
						}
					}
					if (ok)
					{
						if (shortcut->params.cursorHasToBeIn)
						{
							ok = _cuw.get_canvas()->does_hover_on_or_inside(shortcut->params.cursorHasToBeIn);
						}
						if (ok)
						{
							_cuw.set_shortcut_to_activate(this, for_everys_index(shortcut));
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool ICanvasElement::process_controls(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw)
{
	return false;
}

void ICanvasElement::prepare_context(ActContext& _context, Canvas* _canvas, Optional<Vector2> const& _actAt) const
{
	_context.canvas = _canvas;
	_context.element = cast_to_nonconst(this);
	_context.at = _actAt;
	if (_actAt.is_set())
	{
		Range2 placement = get_placement(_canvas);
		_context.relToCentre = _actAt.get() - placement.centre();
	}
}

void ICanvasElement::fit_into_canvas(Canvas const* _canvas)
{
	Range2 placement = get_placement(_canvas);

	Vector2 offset = Vector2::zero;

	// right bottom
	if (placement.x.max > _canvas->get_logical_size().x)
	{
		offset.x = _canvas->get_logical_size().x - placement.x.max;
	}
	if (placement.y.min < 0.0f)
	{
		offset.y = -placement.y.min;
	}

	// top left
	if (placement.x.min < 0.0f)
	{
		offset.x = -placement.x.min;
	}
	if (placement.y.max > _canvas->get_logical_size().y)
	{
		offset.y = _canvas->get_logical_size().y - placement.y.max;
	}

	if (!offset.is_zero())
	{
		offset_placement_by(offset);
	}
}
