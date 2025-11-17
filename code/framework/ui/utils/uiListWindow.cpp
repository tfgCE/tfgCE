#include "uiListWindow.h"

#include "..\uiCanvas.h"
#include "..\uiCanvasButton.h"
#include "..\uiCanvasWindow.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace UI;

//

CanvasWindow* Utils::ListWindow::step_1_create_window(Canvas * _canvas, Optional<float> const& _textScale, Optional<Vector2> const& _spacing)
{
	canvas = _canvas;
	textScale = _textScale.get(1.0f);
	spacing = _spacing.get(canvas->get_default_spacing());

	window = new UI::CanvasWindow();
	window->set_closable(false)->set_movable(false)->set_modal(true);
	canvas->add(window);

	// bottom panel with buttons
	bottomPanel = new UI::CanvasWindow();
	bottomPanel->set_closable(false)->set_movable(false);
	window->add(bottomPanel);

	// main panel with assets
	listPanel = new UI::CanvasWindow();
	listPanel->set_closable(false)->set_movable(false);
	window->add(listPanel);

	return window;
}

CanvasWindow* Utils::ListWindow::step_2_setup_list(Name const& _listId)
{
	listPanel->set_id(_listId, -1);

	return listPanel;
}

CanvasButton* Utils::ListWindow::step_3_add_button(tchar const* _buttonText)
{
	auto* b = new UI::CanvasButton();
	b->set_caption(_buttonText);
	b->set_scale(textScale);
	b->set_auto_width(canvas)->set_default_height(canvas);
	bottomPanel->add(b);
	return b;
}

CanvasWindow* Utils::ListWindow::step_4_finalise_force_size(Vector2 const& _size, Optional<VectorInt2> const& _buttonGrid)
{
	window->set_size(_size); // full!
	window->set_at_pt(canvas, Vector2::half);

	place_everything_inside(_buttonGrid);

	return window;
}

void Utils::ListWindow::place_everything_inside(Optional<VectorInt2> const& _buttonGrid)
{
	Range2 windowRange = window->get_inner_placement(canvas);

	VectorInt2 buttonGrid = _buttonGrid.get(VectorInt2::zero);
	{
		if (buttonGrid.is_zero())
		{
			Concurrency::ScopedSpinLock lock(bottomPanel->access_elements().access_lock(), true);
			buttonGrid.x = min(4, bottomPanel->access_elements().get_elements().get_size());
		}

		bottomPanel->set_at(spacing); // left bottom
		bottomPanel->place_content_on_grid(canvas, buttonGrid, Vector2(windowRange.x.length() - spacing.x * 2.0f, 0.0f), spacing);
	}

	Range2 bottomPanelRange = bottomPanel->get_placement(canvas);
	bottomPanelRange.move_by(-bottomPanel->get_global_offset()); // will be local/inside

	{
		Vector2 lb = Vector2(spacing.x, bottomPanelRange.y.max + spacing.y);
		Vector2 rb = Vector2(windowRange.x.length() - spacing.x, windowRange.y.length() - spacing.y);
		listPanel->set_at(lb);
		listPanel->set_size(rb - lb);
		listPanel->set_use_scroll_vertical();
	}
}

int Utils::ListWindow::get_highlighted_list_element_index(Canvas* canvas, Name const& _listID)
{
	auto* listPanel = fast_cast<UI::CanvasWindow>(canvas->find_by_id(_listID, -1));
	if (!listPanel)
	{
		return -1;
	}

	auto& e = listPanel->access_elements();
	Concurrency::ScopedSpinLock lock(e.access_lock());

	for_every_ptr(e, e.access_elements())
	{
		if (auto* b = fast_cast<UI::CanvasButton>(e))
		{
			if (b->get_id() == _listID &&
				b->is_highlighted())
			{
				return b->get_additional_id();
			}
		}
	}

	return -1;
}

void const * Utils::ListWindow::get_highlighted_list_element_user_data(Canvas* canvas, Name const& _listID)
{
	auto* listPanel = fast_cast<UI::CanvasWindow>(canvas->find_by_id(_listID, -1));
	if (!listPanel)
	{
		return nullptr;
	}

	auto& e = listPanel->access_elements();
	Concurrency::ScopedSpinLock lock(e.access_lock());

	for_every_ptr(e, e.access_elements())
	{
		if (auto* b = fast_cast<UI::CanvasButton>(e))
		{
			if (b->get_id() == _listID &&
				b->is_highlighted())
			{
				return b->get_user_data();
			}
		}
	}

	return nullptr;
}

void Utils::ListWindow::highlight_list_element(Canvas* canvas, Name const& _listID, int _idx)
{
	auto* listPanel = fast_cast<UI::CanvasWindow>(canvas->find_by_id(_listID, -1));
	if (!listPanel)
	{
		return;
	}

	auto& e = listPanel->access_elements();
	Concurrency::ScopedSpinLock lock(e.access_lock());

	ICanvasElement* highlightedElement = nullptr;

	for_every_ptr(e, e.access_elements())
	{
		if (auto* b = fast_cast<UI::CanvasButton>(e))
		{
			if (b->get_id() == _listID)
			{
				bool highlight = b->get_additional_id() == _idx;
				b->set_highlighted(highlight);
				if (highlight)
				{
					highlightedElement = b;
				}
			}
		}
	}

	listPanel->scroll_to_show_element(canvas, highlightedElement);
}

void Utils::ListWindow::populate_list(Canvas* canvas, Name const& _listID, int _count, Optional<int> const & _selectedIdx, Optional<float> const& _textScale,
	std::function<void(int _idx, CanvasButton* _element)> _setupElement,
	std::function<void(int _idx, void const * _userData)> _onSelect,
	std::function<void(int _idx, void const * _userData)> _onPress,
	std::function<void(int _idx, void const * _userData)> _onDoubleClick)
{
	auto* listPanel = fast_cast<UI::CanvasWindow>(canvas->find_by_id(_listID, -1));
	if (!listPanel)
	{
		return;
	}

	if (!_onPress)
	{
		_onPress = _onSelect;
	}

	float textScale = _textScale.get(1.0f);

	Range2 listRange = listPanel->get_inner_placement(canvas);

	auto& e = listPanel->access_elements();
	Concurrency::ScopedSpinLock lock(e.access_lock());

	Optional<Vector2> firstElementAt;

#ifdef AN_STANDARD_INPUT
	auto* inWindow = listPanel->get_in_window();
#endif

	listPanel->clear_shortcuts();
#ifdef AN_STANDARD_INPUT
	listPanel->add_shortcut(System::Key::UpArrow, UI::ShortcutParams().cursor_has_to_be_in(inWindow).handle_hold_as_presses(),
		[canvas, _listID, _onSelect](UI::ActContext const& _context)
		{
			int idx = get_highlighted_list_element_index(canvas, _listID);
			idx = max(0, idx - 1);
			highlight_list_element(canvas, _listID, idx);
			if (_onSelect)
			{
				_onSelect(idx, get_highlighted_list_element_user_data(canvas, _listID));
			}
		});
	listPanel->add_shortcut(System::Key::DownArrow, UI::ShortcutParams().cursor_has_to_be_in(inWindow).handle_hold_as_presses(),
		[canvas, _listID, _onSelect, _count](UI::ActContext const& _context)
		{
			int idx = get_highlighted_list_element_index(canvas, _listID);
			idx = min(_count - 1, idx + 1);
			highlight_list_element(canvas, _listID, idx);
			if (_onSelect)
			{
				_onSelect(idx, get_highlighted_list_element_user_data(canvas, _listID));
			}
		});
	listPanel->add_shortcut(System::Key::Home, UI::ShortcutParams().cursor_has_to_be_in(inWindow).handle_hold_as_presses(),
		[canvas, _listID, _onSelect](UI::ActContext const& _context)
		{
			int idx = 0;
			highlight_list_element(canvas, _listID, idx);
			if (_onSelect)
			{
				_onSelect(idx, get_highlighted_list_element_user_data(canvas, _listID));
			}
		});
	listPanel->add_shortcut(System::Key::End, UI::ShortcutParams().cursor_has_to_be_in(inWindow).handle_hold_as_presses(),
		[canvas, _listID, _onSelect, _count](UI::ActContext const& _context)
		{
			int idx = max(0, _count - 1);
			highlight_list_element(canvas, _listID, idx);
			if (_onSelect)
			{
				_onSelect(idx, get_highlighted_list_element_user_data(canvas, _listID));
			}
		});
#endif

	if (!e.get_elements().is_empty())
	{
		auto* f = e.get_elements().get_first();
		Range2 fr = f->get_placement(canvas);
		firstElementAt = fr.bottom_left();
	}

	while (e.get_elements().get_size() > _count)
	{
		e.remove_last();
	}
	while (e.get_elements().get_size() < _count)
	{
		int eIdx = e.get_elements().get_size();
		{
			auto* b = new UI::CanvasButton();
			b->set_id(_listID, eIdx);
			b->set_caption(String::printf(TXT("element %i"), eIdx));
			b->set_scale(textScale);
			b->set_width(listRange.x.length())->set_default_height(canvas);
			b->set_alignment(Vector2(0.0f, 0.5f));
			b->set_no_frame()->set_use_window_paper();
			b->set_on_press([canvas, _listID, eIdx, _onPress](UI::ActContext const&)
				{
					highlight_list_element(canvas, _listID, eIdx);
					if (_onPress)
					{
						_onPress(eIdx, get_highlighted_list_element_user_data(canvas, _listID));
					}
				});
			if (_onDoubleClick)
			{
				b->set_on_double_click([canvas, _listID, eIdx, _onDoubleClick](UI::ActContext const&)
					{
						highlight_list_element(canvas, _listID, eIdx);
						if (_onDoubleClick)
						{
							_onDoubleClick(eIdx, get_highlighted_list_element_user_data(canvas, _listID));
						}
					});
			}
			listPanel->add(b);
		}
	}

	UI::CanvasButton* selected = nullptr;
	for_count(int, eIdx, _count)
	{
		if (auto* b = fast_cast<UI::CanvasButton>(canvas->find_by_id(_listID, eIdx)))
		{
			if (_selectedIdx.is_set())
			{
				bool thisSelected = _selectedIdx.get() == eIdx;
				b->set_highlighted(thisSelected);
				if (thisSelected)
				{
					selected = b;
				}
			}
			if (_setupElement)
			{
				_setupElement(eIdx, b);
			}
		}
	}

	listPanel->place_content_on_grid(canvas, VectorInt2(1, 0), Vector2(listRange.x.length(), 0.0f), Vector2::zero, NP, true);

	// move them to their previous locations
	if (firstElementAt.is_set() &&
		!e.get_elements().is_empty())
	{
		auto* f = e.get_elements().get_first();
		Range2 fr = f->get_placement(canvas);
		Vector2 firstElementNowAt = fr.bottom_left();

		e.offset_placement_by(firstElementAt.get() - firstElementNowAt);
	}

	if (selected)
	{
		listPanel->update_for_canvas(canvas);

		listPanel->scroll_to_show_element(canvas, selected);
	}
}
