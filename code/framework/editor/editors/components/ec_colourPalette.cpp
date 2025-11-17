#include "ec_colourPalette.h"

#include "..\editorBase.h"
#include "..\..\editorContext.h"

#include "..\..\..\ui\uiCanvasInputContext.h"

#include "..\..\..\ui\uiCanvasButton.h"
#include "..\..\..\ui\uiCanvasWindow.h"

#include "..\..\..\ui\utils\uiUtils.h"
#include "..\..\..\ui\utils\uiGrid2Menu.h"
#include "..\..\..\ui\utils\uiListWindow.h"
#include "..\..\..\ui\utils\uiListWindowImpl.inl"
#include "..\..\..\ui\utils\uiQuestionWindow.h"

#include "..\..\..\video\colourPalette.h"
#include "..\..\..\video\sprite.h"

#include "..\..\..\video\colourPalette.h"
#include "..\..\..\video\sprite.h"

#include "..\..\..\voxel\voxelMold.h"

#include "..\..\..\..\core\system\input.h"
#include "..\..\..\..\core\other\simpleVariableStorage.h"

//

#include "..\editorBaseImpl.inl"

//

using namespace Framework;

//

// settings
DEFINE_STATIC_NAME(editorPalette_open);
DEFINE_STATIC_NAME(editorPalette_at);
DEFINE_STATIC_NAME(editorPalette_atValid);
DEFINE_STATIC_NAME(editorPalette_selectedColourIdx);
DEFINE_STATIC_NAME(editorPalette_colourShortCut0);
DEFINE_STATIC_NAME(editorPalette_colourShortCut1);
DEFINE_STATIC_NAME(editorPalette_colourShortCut2);
DEFINE_STATIC_NAME(editorPalette_colourShortCut3);
DEFINE_STATIC_NAME(editorPalette_colourShortCut4);
DEFINE_STATIC_NAME(editorPalette_colourShortCut5);
DEFINE_STATIC_NAME(editorPalette_colourShortCut6);
DEFINE_STATIC_NAME(editorPalette_colourShortCut7);

// ui
DEFINE_STATIC_NAME(editor_props_palette);

DEFINE_STATIC_NAME(editorColourPalette);
DEFINE_STATIC_NAME(editorColourPalette_colour);

DEFINE_STATIC_NAME(chooseLibraryStored_colourPalette);

//

Editor::Component::ColourPalette::ColourPalette(Editor::Base* _editor)
	: editor(_editor)
{
	colourShortCuts.clear();
	colourShortCuts.set_size(4);
	colourShortCuts[0].keyVisible = 'a';
#ifdef AN_STANDARD_INPUT
	colourShortCuts[0].keyCode = ::System::Key::A;
#endif
	colourShortCuts[1].keyVisible = 's';
#ifdef AN_STANDARD_INPUT
	colourShortCuts[1].keyCode = ::System::Key::S;
#endif
	colourShortCuts[2].keyVisible = 'd';
#ifdef AN_STANDARD_INPUT
	colourShortCuts[2].keyCode = ::System::Key::D;
#endif
	colourShortCuts[3].keyVisible = 'f';
#ifdef AN_STANDARD_INPUT
	colourShortCuts[3].keyCode = ::System::Key::F;
#endif
}

#define STORE_RESTORE(_op) \
	_op(_setup, windows.palette.open, NAME(editorPalette_open)); \
	_op(_setup, windows.palette.at, NAME(editorPalette_at)); \
	_op(_setup, windows.palette.atValid, NAME(editorPalette_atValid)); \
	_op(_setup, selectedColourIdx, NAME(editorPalette_selectedColourIdx)); \
	_op(_setup, colourShortCuts[0].selectedColourIdx, NAME(editorPalette_colourShortCut0)); \
	_op(_setup, colourShortCuts[1].selectedColourIdx, NAME(editorPalette_colourShortCut1)); \
	_op(_setup, colourShortCuts[2].selectedColourIdx, NAME(editorPalette_colourShortCut2)); \
	_op(_setup, colourShortCuts[3].selectedColourIdx, NAME(editorPalette_colourShortCut3)); \
	;

void Editor::Component::ColourPalette::restore_for_use(SimpleVariableStorage const& _setup)
{
	STORE_RESTORE(editor->read);
	update_ui_highlight();
	restore_windows(true);
}

void Editor::Component::ColourPalette::store_for_later(SimpleVariableStorage& _setup) const
{
	STORE_RESTORE(editor->write);
}

void Editor::Component::ColourPalette::process_controls()
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

		if (!ctrlPressed && !shiftPressed && !altPressed)
		{
			for_every(csc, colourShortCuts)
			{
				if (input->has_key_been_pressed((System::Key::Type)csc->keyCode) ||
					input->has_key_been_released((System::Key::Type)csc->keyCode))
				{
					if (csc->selectedColourIdx >= 0)
					{
						selectedColourIdx = csc->selectedColourIdx;
						update_ui_highlight();
					}
				}
			}
		}
	}
}

void Editor::Component::ColourPalette::set_selected_colour_index(int _idx)
{
	selectedColourIdx = _idx;
	if (auto* input = ::System::Input::get())
	{
		for_every(csc, colourShortCuts)
		{
			if (input->is_key_pressed((System::Key::Type)csc->keyCode))
			{
				csc->selectedColourIdx = selectedColourIdx;
			}
		}
	}
}

void Editor::Component::ColourPalette::restore_windows(bool _forcePlaceAt)
{
	if (auto* c = editor->get_canvas())
	{
		show_palette(windows.palette.open, _forcePlaceAt);
	}
}

void Editor::Component::ColourPalette::store_windows_at()
{
	if (auto* c = editor->get_canvas())
	{
		if (auto* w = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorColourPalette))))
		{
			windows.palette.at = w->get_placement(c).get_at(Vector2(0.0f, 1.0f));
			windows.palette.atValid = true;
		}
	}
}

void Editor::Component::ColourPalette::show_or_hide_palette()
{
	UI::Canvas* canvas = editor->get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorColourPalette)));
	show_palette(existing == nullptr);
}

void Editor::Component::ColourPalette::reshow_palette()
{
	UI::Canvas* canvas = editor->get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorColourPalette))))
	{
		show_palette(false);
		show_palette(true, true);
	}
}

void Editor::Component::ColourPalette::setup_palette_colour_button(Framework::UI::CanvasButton* b, int colourIdx)
{
	b->set_id(NAME(editorColourPalette_colour), colourIdx);
	b->set_on_press([this, colourIdx](Framework::UI::ActContext const& _context)
		{
			selectedColourIdx = colourIdx;
			// if any colour hot-key is pressed, make it set here and clear everywhere else
			if (auto* input = ::System::Input::get())
			{
				for_every(csc, colourShortCuts)
				{
					if (input->is_key_pressed((System::Key::Type)csc->keyCode))
					{
						csc->selectedColourIdx = colourIdx;
					}
				}
			}
			update_ui_highlight();
		});
}

void Editor::Component::ColourPalette::show_palette(bool _show, bool _forcePlaceAt)
{
	UI::Canvas* canvas = editor->get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorColourPalette)));

	if (_show)
	{
		if (!existing)
		{
			uiColourCount = 0;

			Framework::UI::ICanvasElement* above = nullptr;
			if (auto* cp = get_colour_palette())
			{
				auto* menu = Framework::UI::CanvasWindow::get();
				existing = menu;
				menu->set_closable(true);
				menu->set_id(NAME(editorColourPalette));
				menu->set_title(TXT("palette (asdf)"));
				menu->set_on_moved([this](UI::CanvasWindow* _window) { store_windows_at(); });
				c->add(menu);
				{
					Framework::UI::ICanvasElement* belowPanel = nullptr;
					// invisible / force empty
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						menu->add(panel);
						if (auto* s = fast_cast<Framework::Sprite>(editor->get_edited_asset()))
						{
							for_range(int, colourIdx, Framework::SpritePixel::MinActualContent, Framework::SpritePixel::MaxActualContent)
							{
								auto* b = Framework::UI::CanvasButton::get();
								b->set_caption(Framework::SpritePixel::content_to_char(Framework::SpritePixel::Content(colourIdx)));
								b->set_scale(0.7f)->set_auto_width(c)->set_default_height(c);
								b->set_frame_width_pt(0.1f);
								b->set_use_frame_width_only_for_action();
								setup_palette_colour_button(b, colourIdx);
								panel->add(b);
								++uiColourCount;
							}
							panel->place_content_on_grid(c, VectorInt2(0, 1), NP, Vector2::zero);
						}
						else if (auto* a = fast_cast<Framework::VoxelMold>(editor->get_edited_asset()))
						{
							for_range(int, colourIdx, Framework::Voxel::MinActualContent, Framework::Voxel::MaxActualContent)
							{
								auto* b = Framework::UI::CanvasButton::get();
								b->set_caption(Framework::Voxel::content_to_char(Framework::Voxel::Content(colourIdx)));
								b->set_scale(0.7f)->set_auto_width(c)->set_default_height(c);
								b->set_frame_width_pt(0.1f); 
								b->set_use_frame_width_only_for_action();
								setup_palette_colour_button(b, colourIdx);
								panel->add(b);
								++uiColourCount;
							}
							panel->place_content_on_grid(c, VectorInt2(0, 1), NP, Vector2::zero);
						}
						else
						{
							todo_implement(TXT("implement support for this asset"));
						}
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, 0.0f);
					}
					// actual colours
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						menu->add(panel);
						{
							for_every(col, cp->get_colours())
							{
								auto* b = Framework::UI::CanvasButton::get()->set_colour(*col)->set_no_frame()->set_size(Vector2::one * (c->get_logical_size().y * 0.025f));
								int colourIdx = for_everys_index(col);
								b->set_frame_width_pt(0.2f);
								b->set_use_frame_width_only_for_action();
								b->set_inner_frame_with_background();
								setup_palette_colour_button(b, colourIdx);
								panel->add(b);
								++uiColourCount;
							}

							VectorInt2 grid = VectorInt2::zero;
							if (cp->get_preferred_grid().is_set())
							{
								grid = cp->get_preferred_grid().get();
							}
							if (grid.is_zero())
							{
								int colourCount = cp->get_colours().get_size();
								grid.y = max(1, (int)(sqrt((float)colourCount)) / 2);
							}

							panel->place_content_on_grid(c, grid, NP, Vector2::zero);
						}
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, 0.0f);
					}
					menu->set_size_to_fit_all(c, Vector2::zero);
				}
				if (windows.palette.atValid)
				{
					menu->set_at(windows.palette.at);
					menu->set_at(windows.palette.at - Vector2(0.0f, menu->get_placement(c).y.length()));
					menu->fit_into_canvas(c);
				}
				else
				{
					UI::Utils::place_above(c, menu, REF_ above, 1.0f);
				}
			}
		}

		if (existing && _forcePlaceAt)
		{
			if (windows.palette.atValid)
			{
				existing->set_at(windows.palette.at);
				existing->set_at(windows.palette.at - Vector2(0.0f, existing->get_placement(c).y.length()));
				existing->fit_into_canvas(c);
			}
		}
	}
	else
	{
		if (existing)
		{
			store_windows_at();
			existing->remove();
		}
	}

	windows.palette.open = _show;

	update_ui_highlight();
}

void Editor::Component::ColourPalette::update_ui_highlight()
{
	auto* c = editor->get_canvas();
	if (!c)
	{
		return;
	}

	int minActualContent = 0;
	if (auto* a = editor->get_edited_asset())
	{
		if (fast_cast<Framework::Sprite>(a))
		{
			minActualContent = Framework::SpritePixel::MinActualContent;
		}
		else if (fast_cast<Framework::VoxelMold>(a))
		{
			minActualContent = Framework::Voxel::MinActualContent;
		}
		else
		{
			todo_implement(TXT("implement support for this asset"));
		}
	}
	if (minActualContent < 0 || uiColourCount > 0)
	{
		// only if there is something
		if (selectedColourIdx >= 0)
		{
			selectedColourIdx = clamp(selectedColourIdx, 0, uiColourCount);
		}

		for_range(int, cIdx, minActualContent, uiColourCount - 1)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorColourPalette_colour), cIdx)))
			{
				b->set_highlighted(selectedColourIdx == cIdx);
				tchar caption = 0;
				for_every(csc, colourShortCuts)
				{
					if (csc->selectedColourIdx == cIdx)
					{
						caption = csc->keyVisible;
						break;
					}
				}
				if (cIdx >= 0)
				{
					if (caption)
					{
						b->set_caption(String() + caption);
						b->set_scale(0.5f);
					}
					else
					{
						b->set_caption(String::empty());
					}
				}
				else
				{
#ifdef AN_STANDARD_INPUT
					if (caption)
					{
						b->set_shortcut(System::Key::Space, UI::ShortcutParams().not_to_be_used(), nullptr, String() + caption);
					}
					else
#endif
					{
						b->clear_shortcuts();
					}
				}
			}
		}
	}

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor_props_palette))))
	{
		auto* cp = get_colour_palette();
		b->set_caption(cp ? cp->get_name().to_string().to_char() : TXT("--"));
	}
}

void Editor::Component::ColourPalette::add_to_asset_props(UI::Utils::Grid2Menu& menu)
{
	menu.step_2_add_text_and_button(TXT("palette"), TXT("--"))
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::P)
#endif
		->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				{
					UI::Utils::choose_library_stored<Framework::ColourPalette>(_context.canvas, NAME(chooseLibraryStored_colourPalette), TXT("colour palette"), get_colour_palette(), 1.0f,
						[this](Framework::ColourPalette* _chosen)
						{
							UI::Utils::QuestionWindow qw;
							qw.step_1_create_window(editor->get_canvas());
							qw.step_2_add_text(TXT("changing palette"));
							qw.step_2_add_answer(TXT("convert"), [this, _chosen]()
								{
									set_colour_palette(_chosen, true);
									editor->get_editor_context()->mark_save_required();
									reshow_palette();
								});
							qw.step_2_add_answer(TXT("just change"), [this, _chosen]()
								{
									set_colour_palette(_chosen, false);
									editor->get_editor_context()->mark_save_required();
									reshow_palette();
								});
							qw.step_3_finalise();
						});
				}
			})
		->set_id(NAME(editor_props_palette));
}

void Editor::Component::ColourPalette::set_colour_palette(Framework::ColourPalette* _palette, bool _convertToNewOne) const
{
	if (auto* a = fast_cast<Framework::Sprite>(editor->get_edited_asset()))
	{
		SpriteContentAccess::set_colour_palette(a, _palette, _convertToNewOne);
		return;
	}
	if (auto* a = fast_cast<Framework::VoxelMold>(editor->get_edited_asset()))
	{
		VoxelMoldContentAccess::set_colour_palette(a, _palette, _convertToNewOne);
		return;
	}

	todo_implement(TXT("implement support for this asset"));

}

Framework::ColourPalette const * Editor::Component::ColourPalette::get_colour_palette() const
{
	if (auto* ea = editor->get_edited_asset())
	{
		if (auto* a = fast_cast<Framework::Sprite>(ea))
		{
			return a->get_colour_palette();
		}
		if (auto* a = fast_cast<Framework::VoxelMold>(ea))
		{
			return a->get_colour_palette();
		}

		todo_implement(TXT("implement support for this asset"));
		return nullptr;
	}
	return nullptr;
}
