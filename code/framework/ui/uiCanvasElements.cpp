#include "uiCanvasElements.h"

#include "uiCanvasElement.h"
#include "uiCanvasWindow.h"
#include "uiCanvasUpdateWorktable.h"

#include "..\..\core\concurrency\scopedSpinLock.h"

//

using namespace Framework;
using namespace UI;

//

CanvasElements::CanvasElements(ICanvasElement* _isPartOfElement)
: isPartOfElement(_isPartOfElement)
{

}

CanvasElements::CanvasElements(Canvas* _isMainCanvas)
: isMainCanvas(_isMainCanvas)
{

}

CanvasElements::~CanvasElements()
{
	clear();
}

void CanvasElements::reset()
{
	globalOffset = Vector2::zero;
	clear();
}

void CanvasElements::clear()
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_every_ptr(element, elements)
	{
		an_assert(element->inElements == this);
		element->inElements = nullptr;
		element->release_element();
	}
	elements.clear();
}

void CanvasElements::update_for_canvas(Canvas const* _canvas) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_every_ptr(element, elements)
	{
		element->update_for_canvas(_canvas);
	}
}

void CanvasElements::render(CanvasRenderContext& _context) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_count(int, pass, 2)
	{
		for_every_ptr(element, elements)
		{
			bool isWindow = fast_cast<CanvasWindow>(element);
			if ((pass == 0 && !isWindow) ||
				(pass == 1 && isWindow))
			{
				element->render(_context);
			}
		}
	}
}

void CanvasElements::add(ICanvasElement* _element)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	an_assert(_element->inElements == nullptr);
	_element->inElements = this;
	elements.push_back(_element);
}

void CanvasElements::to_top(ICanvasElement* _element)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	an_assert(_element->inElements == this);
	elements.remove(_element);
	elements.push_back(_element);
}

void CanvasElements::remove(ICanvasElement* _element)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	if (elements.does_contain(_element))
	{
		elements.remove(_element);
		_element->inElements = nullptr;
		_element->release_element();
	}
}

void CanvasElements::remove_last()
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	if (! elements.is_empty())
	{
		auto* e = elements.get_last();
		elements.pop_back();
		e->inElements = nullptr;
		e->release_element();
	}
}

bool CanvasElements::process_shortcuts(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_count_reverse(int, pass, 2)
	{
		for_every_reverse_ptr(element, elements)
		{
			auto* isWindow = fast_cast<CanvasWindow>(element);
			if ((pass == 0 && !isWindow) ||
				(pass == 1 && isWindow))
			{
				if (element->process_shortcuts(_cic, _cuw))
				{
					return true;
				}
				if (isWindow && isWindow->is_modal())
				{
					_cuw.set_in_modal_window();
					return false;
				}
			}
		}
	}

	return false;
}

bool CanvasElements::process_controls(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_count_reverse(int, pass, 2)
	{
		for_every_reverse_ptr(element, elements)
		{
			auto* isWindow = fast_cast<CanvasWindow>(element);
			if ((pass == 0 && !isWindow) ||
				(pass == 1 && isWindow))
			{
				if (element->process_controls(_cic, _cuw))
				{
					return true;
				}
				if (isWindow && isWindow->is_modal())
				{
					_cuw.set_in_modal_window();
					return false;
				}
			}
		}
	}

	return false;
}

void CanvasElements::update_hover_on(CanvasInputContext const& _cic, CanvasUpdateWorktable& _cuw) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_count_reverse(int, pass, 2)
	{
		for_every_reverse_ptr(element, elements)
		{
			auto* isWindow = fast_cast<CanvasWindow>(element);
			if ((pass == 0 && !isWindow) ||
				(pass == 1 && isWindow))
			{
				element->update_hover_on(_cic, _cuw);
				if (isWindow && isWindow->is_modal())
				{
					_cuw.set_in_modal_window();
					return;
				}
			}
		}
	}
}

ICanvasElement* CanvasElements::find_by_id(Name const& _id, Optional<int> const& _additionalId) const
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_every_ptr(element, elements)
	{
		if (auto* e = element->find_by_id(_id, _additionalId))
		{
			return e;
		}
	}

	return nullptr;
}

ICanvasElement* CanvasElements::get_in_element() const
{
	return isPartOfElement;
}

Canvas* CanvasElements::get_in_canvas() const
{
	if (isMainCanvas)
	{
		return isMainCanvas;
	}
	else if (isPartOfElement)
	{
		return isPartOfElement->get_in_elements().get_in_canvas();
	}
	return nullptr;
}

void CanvasElements::offset_placement_by(Vector2 const& _offset)
{
	Concurrency::ScopedSpinLock lock(accessLock, true);

	for_every_ptr(element, elements)
	{
		element->offset_placement_by(_offset);
	}
}
