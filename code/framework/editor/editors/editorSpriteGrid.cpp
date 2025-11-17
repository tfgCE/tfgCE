#include "editorSpriteGrid.h"

#include "editorBaseImpl.inl"
#include "..\editorContext.h"

#include "..\..\ui\uiCanvasInputContext.h"

#include "..\..\ui\uiCanvasButton.h"
#include "..\..\ui\uiCanvasWindow.h"

#include "..\..\ui\utils\uiUtils.h"
#include "..\..\ui\utils\uiGrid2Menu.h"
#include "..\..\ui\utils\uiListWindow.h"
#include "..\..\ui\utils\uiListWindowImpl.inl"
#include "..\..\ui\utils\uiQuestionWindow.h"
#include "..\..\ui\utils\uiTextEditWindow.h"

#include "..\..\video\colourPalette.h"
#include "..\..\video\sprite.h"
#include "..\..\video\spriteTextureProcessor.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\system\input.h"
#include "..\..\..\core\system\core.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define SELECTION_COLOUR Colour::yellow
#define SELECTION_FULLY_INLCUDED false
#define HIGHLIGHT_EDITED_LAYER_TIME 0.2f
#define HIGHLIGHT_EDITED_LAYER_COLOUR Colour::orange

#define CONTENT_ACCESS SpriteContentAccess

//

using namespace Framework;

//

// settings
DEFINE_STATIC_NAME(editorSpriteGrid_camera_pivot);
DEFINE_STATIC_NAME(editorSpriteGrid_camera_scale);
DEFINE_STATIC_NAME(editorSpriteGrid_mode);
DEFINE_STATIC_NAME(editorPalette_open);
DEFINE_STATIC_NAME(editorPalette_at);
DEFINE_STATIC_NAME(editorPalette_atValid);
DEFINE_STATIC_NAME(editorPalette_selectedSpriteIdx);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut0);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut1);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut2);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut3);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut4);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut5);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut6);
DEFINE_STATIC_NAME(editorPalette_spriteShortCut7);
DEFINE_STATIC_NAME(editorLayers_open);
DEFINE_STATIC_NAME(editorLayers_at);
DEFINE_STATIC_NAME(editorLayers_atValid);
DEFINE_STATIC_NAME(editorLayerProps_open);
DEFINE_STATIC_NAME(editorLayerProps_at);
DEFINE_STATIC_NAME(editorLayerProps_atValid);

// ui
DEFINE_STATIC_NAME(editorSpriteGrid_props);
DEFINE_STATIC_NAME(editorSpriteGrid_props_spriteSizeX);
DEFINE_STATIC_NAME(editorSpriteGrid_props_spriteSizeY);
DEFINE_STATIC_NAME(editorSpriteGrid_addRemove);
DEFINE_STATIC_NAME(editorSpriteGrid_add);
DEFINE_STATIC_NAME(editorSpriteGrid_remove);
DEFINE_STATIC_NAME(editorSpriteGrid_spritePicker);

DEFINE_STATIC_NAME(editorSpriteGridPalette);
DEFINE_STATIC_NAME(editorSpriteGridPalette_sprite);
DEFINE_STATIC_NAME(editorSpriteGridPalette_swap);

DEFINE_STATIC_NAME(editorLayers);
DEFINE_STATIC_NAME(editorLayers_layerList);

DEFINE_STATIC_NAME(chooseLibraryStored_addSprite);
DEFINE_STATIC_NAME(chooseLibraryStored_spriteGrid);

//--

REGISTER_FOR_FAST_CAST(Editor::SpriteGrid);

Editor::SpriteGrid::SpriteGrid()
{
	storeRestoreNames_cameraPivot = NAME(editorSpriteGrid_camera_pivot);
	storeRestoreNames_cameraScale = NAME(editorSpriteGrid_camera_scale);

	spriteShortCuts.clear();
	spriteShortCuts.set_size(4);
	spriteShortCuts[0].keyVisible = 'a';
#ifdef AN_STANDARD_INPUT
	spriteShortCuts[0].keyCode = ::System::Key::A;
#endif
	spriteShortCuts[1].keyVisible = 's';
#ifdef AN_STANDARD_INPUT
	spriteShortCuts[1].keyCode = ::System::Key::S;
#endif
	spriteShortCuts[2].keyVisible = 'd';
#ifdef AN_STANDARD_INPUT
	spriteShortCuts[2].keyCode = ::System::Key::D;
#endif
	spriteShortCuts[3].keyVisible = 'f';
#ifdef AN_STANDARD_INPUT
	spriteShortCuts[3].keyCode = ::System::Key::F;
#endif

	// we will need sprites!
	SpriteTextureProcessor::add_job_update_and_load();
}

Editor::SpriteGrid::~SpriteGrid()
{
}

#define STORE_RESTORE(_op) \
	_op(_setup, mode, NAME(editorSpriteGrid_mode)); \
	_op(_setup, windows.palette.open, NAME(editorPalette_open)); \
	_op(_setup, windows.palette.at, NAME(editorPalette_at)); \
	_op(_setup, windows.palette.atValid, NAME(editorPalette_atValid)); \
	_op(_setup, selectedSpriteIdx, NAME(editorPalette_selectedSpriteIdx)); \
	_op(_setup, spriteShortCuts[0].selectedSpriteIdx, NAME(editorPalette_spriteShortCut0)); \
	_op(_setup, spriteShortCuts[1].selectedSpriteIdx, NAME(editorPalette_spriteShortCut1)); \
	_op(_setup, spriteShortCuts[2].selectedSpriteIdx, NAME(editorPalette_spriteShortCut2)); \
	_op(_setup, spriteShortCuts[3].selectedSpriteIdx, NAME(editorPalette_spriteShortCut3)); \
	;

void Editor::SpriteGrid::edit(LibraryStored* _asset, String const& _filePathToSave)
{
	base::edit(_asset, _filePathToSave);
}

void Editor::SpriteGrid::restore_for_use(SimpleVariableStorage const& _setup)
{
	base::restore_for_use(_setup);
	STORE_RESTORE(read);
	update_ui_highlight();
	restore_windows(true);
}

void Editor::SpriteGrid::store_for_later(SimpleVariableStorage& _setup) const
{
	base::store_for_later(_setup);
	STORE_RESTORE(write);
}

void Editor::SpriteGrid::restore_windows(bool _forcePlaceAt)
{
	if (auto* c = get_canvas())
	{
		show_palette(windows.palette.open, _forcePlaceAt);
	}
}

void Editor::SpriteGrid::store_windows_at()
{
	if (auto* c = get_canvas())
	{
		if (auto* w = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorSpriteGridPalette))))
		{
			windows.palette.at = w->get_placement(c).get_at(Vector2(0.0f, 1.0f));
			windows.palette.atValid = true;
		}
	}
}

void Editor::SpriteGrid::on_hover(int _cursorIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);
}

void Editor::SpriteGrid::on_press(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left)
	{
		if (mode == Mode::AddRemove)
		{
			do_operation(_cursorIdx, Operation::AddNode);
			return;
		}
	}

	base::on_press(_cursorIdx, _buttonIdx, _rayStart);
}

void Editor::SpriteGrid::on_click(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Right)
	{
		if (mode == Mode::AddRemove)
		{
			do_operation(_cursorIdx, Operation::RemoveNode);
			return;
		}
	}

	base::on_click(_cursorIdx, _buttonIdx, _rayStart);
}

bool Editor::SpriteGrid::on_hold_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left &&
		(mode == Mode::AddRemove || mode == Mode::Add || mode == Mode::Remove || mode == Mode::GetSprite))
	{
		if (cursorRays.is_index_valid(_cursorIdx))
		{
			if (mode == Mode::AddRemove)
			{
				do_operation(_cursorIdx, Operation::AddNode, true);
			}
			if (mode == Mode::Add)
			{
				do_operation(_cursorIdx, Operation::AddNode, true);
			}
			if (mode == Mode::Remove)
			{
				do_operation(_cursorIdx, Operation::RemoveNode, true);
			}
			if (mode == Mode::GetSprite)
			{
				do_operation(_cursorIdx, Operation::GetSpriteFromNode, true);
			}

			inputGrabbed = false;
		}

		return true;
	}

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Right &&
		(mode == Mode::AddRemove))
	{
		if (cursorRays.is_index_valid(_cursorIdx))
		{
			if (mode == Mode::AddRemove)
			{
				do_operation(_cursorIdx, Operation::RemoveNode, true);
			}

			inputGrabbed = false;
		}

		return true;
	}

	return false;
}

bool Editor::SpriteGrid::on_hold(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	if (on_hold_held(_cursorIdx, _buttonIdx, _rayStart))
	{
		return true;
	}

	return base::on_hold(_cursorIdx, _buttonIdx, _rayStart);
}

bool Editor::SpriteGrid::on_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart, Vector2 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised)
{
	if (on_hold_held(_cursorIdx, _buttonIdx, _rayStart))
	{
		return true;
	}

	return base::on_held(_cursorIdx, _buttonIdx, _rayStart, _movedBy, _cursorMovedBy2DNormalised);
}

void Editor::SpriteGrid::on_release(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	base::on_release(_cursorIdx, _buttonIdx, _rayStart);
}

void Editor::SpriteGrid::process_controls()
{
	base::process_controls();

	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	if (!canvas->is_modal_window_active())
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
				for_every(csc, spriteShortCuts)
				{
					if (input->has_key_been_pressed((System::Key::Type)csc->keyCode) ||
						input->has_key_been_released((System::Key::Type)csc->keyCode))
					{
						if (csc->selectedSpriteIdx != Framework::SpritePixel::None)
						{
							selectedSpriteIdx = csc->selectedSpriteIdx;
							update_ui_highlight();
						}
					}
				}
			}
		}
	}

	// manage gizmos
	//
}

void Editor::SpriteGrid::update_cursor_ray(int _cursorIdx, Vector2 const& _rayStart)
{
	if (!cursorRays.is_index_valid(_cursorIdx))
	{
		cursorRays.set_size(max(cursorRays.get_size(), _cursorIdx + 1));
	}

	auto& cr = cursorRays[_cursorIdx];

	if (hoveringOverAnyGizmo)
	{
		// clear and ignore
		cr = CursorRay();
		return;
	}

	if (cr.rayStart.is_set() && cr.rayStart == _rayStart)
	{
		// already cached
		return;
	}

	cr.rayStart = _rayStart;

	if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
	{
		VectorInt2 rayStartPixel = TypeConversions::Normal::f_i_cells(_rayStart);
		cr.gridCoord = vm->pixel_to_grid(rayStartPixel);
		cr.hitNode = vm->get_content().get_at(cr.gridCoord.get()) != NONE;
	}
}

void Editor::SpriteGrid::do_operation(int _cursorIdx, Operation _operation, bool _checkLastOperationCell)
{
	bool didSomething = false;

	if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
	{
		if (cursorRays.is_index_valid(_cursorIdx))
		{
			auto& cr = cursorRays[_cursorIdx];

			if (cr.gridCoord.is_set())
			{
				auto& sgc = vm->access_content();
				{
					RangeInt2 r = sgc.get_range();
					if (r.is_empty())
					{
						r = RangeInt2::zero;
					}
					// allow to add only close to already existing blocks
					int extraRange = 200;
					r.expand_by(VectorInt2(extraRange, extraRange));
					if (r.does_contain(cr.gridCoord.get()))
					{
						if (_operation == Operation::AddNode)
						{
							VectorInt2 at = cr.gridCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								sgc.access_at(at) = selectedSpriteIdx;
								didSomething = true;
								lastOperationCell = at;
								sgc.prune();
							}
						}
						if (_operation == Operation::RemoveNode)
						{
							VectorInt2 at = cr.gridCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								sgc.access_at(at) = NONE;
								didSomething = true;
								lastOperationCell = at;
								sgc.prune();
							}
						}
					}
				}
				if (_operation == Operation::GetSpriteFromNode)
				{
					// colour can be read from any layer, top most first and without range/selection checks
					VectorInt2 at = cr.gridCoord.get();
					bool read = false;
					int readSpriteIdx = sgc.get_at(at);
					if (readSpriteIdx != NONE)
					{
						selectedSpriteIdx = readSpriteIdx;
						if (auto* input = ::System::Input::get())
						{
							for_every(csc, spriteShortCuts)
							{
								if (input->is_key_pressed((System::Key::Type)csc->keyCode))
								{
									csc->selectedSpriteIdx = selectedSpriteIdx;
								}
							}
						}
						read = true;
					}
					if (read)
					{
						didSomething = true;
						lastOperationCell = at;
						update_ui_highlight();
					}
				}
			}
		}
	}

	if (_operation == Operation::ChangedSpritePalette ||
		_operation == Operation::ChangedSpriteGridProps)
	{
		didSomething = true;
	}

	if (didSomething)
	{
		if (editorContext)
		{
			if (_operation != Operation::GetSpriteFromNode)
			{
				if (editorContext->mark_save_required() ||
					_operation == Operation::ChangedSpriteGridProps)
				{
					update_ui_highlight();
				}
			}
		}

		cursorRays.clear();
	}
}

Range2 Editor::SpriteGrid::get_grid_range() const
{
	if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
	{
		RangeInt2 r = vm->get_content().get_range();
		if (r.is_empty())
		{
			r = RangeInt2::zero;
		}
		r.expand_by(VectorInt2::one * 4);

		return vm->grid_to_pixel_range(r).to_range2();
	}

	return base::get_grid_range();
}

Vector2 Editor::SpriteGrid::get_grid_step() const
{
	if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
	{
		return vm->get_sprite_size().to_vector2();
	}

	return base::get_grid_step();
}

Vector2 Editor::SpriteGrid::get_grid_zero_at() const
{
	if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
	{
		return Vector2::zero;
	}

	return base::get_grid_zero_at();
}

float Editor::SpriteGrid::get_grid_alpha(float _v, float _step) const
{
	float a = 0.2f;
	int i = TypeConversions::Normal::f_i_closest(round_to(_v, _step) / _step);
	if ((i % 8) == 0)
	{
		a = max(a, 1.0f);
	}
	if ((i % 4) == 0)
	{
		a = max(a, 0.75f);
	}
	if ((i % 2) == 0)
	{
		a = max(a, 0.5f);
	}
	return a;
}

void Editor::SpriteGrid::render_debug(CustomRenderContext* _customRC)
{
	debug_gather_in_renderer();

	if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
	{
		for_every(cr, cursorRays)
		{
			if (!cr->gridCoord.is_set())
			{
				continue;
			}
			bool mayAdd = false;
			bool mayRemove = false;
			bool mayReadColour = false;
			bool maySelect = false;
			if (cr->hitNode)
			{
				mayRemove = mode == Mode::Remove || mode == Mode::AddRemove;
				mayReadColour = mode == Mode::GetSprite;
			}
			{
				mayAdd = mode == Mode::Add || mode == Mode::AddRemove;
			}

			Range3 r = Range3::empty;
			{
				Vector2 lb = (cr->gridCoord.get() * vm->get_sprite_size()).to_vector2();
				Vector2 rt = lb + vm->get_sprite_size().to_vector2();
				r.include(lb.to_vector3());
				r.include(rt.to_vector3());
			}
			r.z.min = 0.000f;
			r.z.max = 0.001f;
			if (mayReadColour)
			{
				debug_draw_box(true, false, Colour::red, 0.1f, Box(r));
			}
			else if (mayRemove || maySelect)
			{
				debug_draw_box(true, false, mayAdd ? Colour::white : Colour::black, 0.1f, Box(r));
			}
			else if (mayAdd)
			{
				debug_draw_box(true, false, Colour::white, 0.1f, Box(r));
			}
		}
	}

	debug_gather_restore();

	base::render_debug(_customRC);
}

void Editor::SpriteGrid::render_edited_asset()
{
	if (auto* vm = fast_cast<Framework::SpriteGrid>(editedAsset.get()))
	{
		vm->editor_render();
	}
	else
	{
		base::render_edited_asset();
	}
}

void Editor::SpriteGrid::setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	base::setup_ui_add_to_bottom_left_panel(c, panel, scale);
}

void Editor::SpriteGrid::setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	base::setup_ui_add_to_top_name_panel(c, panel, scale);
}

void Editor::SpriteGrid::setup_ui(REF_ SetupUIContext& _setupUIContext)
{
	base::setup_ui(REF_ _setupUIContext);

	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	Framework::UI::ICanvasElement* & below = _setupUIContext.topRight_down;

	// top right corner
	{
		{
			auto* menu = Framework::UI::CanvasWindow::get();
			menu->set_title(TXT("tools"))->set_closable(false)->set_movable(false);
			c->add(menu);
			{
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("a+r")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N1)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSpriteGrid_addRemove));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::AddRemove;
							show_palette(true); // has to be open to work
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("add")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N2)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSpriteGrid_add));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Add;
							show_palette(true); // has to be open to work
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("rem")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N3)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSpriteGrid_remove));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Remove;
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("picker")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N5)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSpriteGrid_spritePicker));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::GetSprite;
							show_palette(true); // has to be open to work
							update_ui_highlight();
						});
					menu->add(b);
				}

				//menu->place_content_horizontally(c);
				menu->place_content_on_grid(c, VectorInt2(4, 0));
			}
			UI::Utils::place_below(c, menu, REF_ below, 1.0f);

			if (!_setupUIContext.topRight)
			{
				_setupUIContext.topRight = below;
				_setupUIContext.topRight_left = below;
			}
		}
		{
			auto* menu = Framework::UI::CanvasWindow::get();
			menu->set_closable(false)->set_movable(false);
			c->add(menu);
			{
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("palette")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::P, UI::ShortcutParams().with_shift())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							auto* existing = _context.canvas->find_by_id(NAME(editorSpriteGridPalette));
							show_palette(existing == nullptr);
						});
					menu->add(b);
				}

				menu->place_content_vertically(c);
			}
			UI::Utils::place_below(c, menu, REF_ below, 1.0f);
		}

		restore_windows(true);
	}
}

void Editor::SpriteGrid::reshow_palette()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorSpriteGridPalette))))
	{
		show_palette(false);
		show_palette(true, true);
	}
}

void Editor::SpriteGrid::setup_palette_button(Framework::UI::CanvasButton* b, int spriteIdx)
{
	b->set_id(NAME(editorSpriteGridPalette_sprite), spriteIdx);
	b->set_on_press([this, spriteIdx](Framework::UI::ActContext const& _context)
		{
			if (swappingPalette)
			{
				if (selectedSpriteIdx != NONE)
				{
					if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
					{
						vm->access_palette().swap(selectedSpriteIdx, spriteIdx);
						vm->access_palette().cache();
						selectedSpriteIdx = NONE;
						reshow_palette();
					}
				}
				else
				{
					selectedSpriteIdx = spriteIdx;
					update_ui_highlight();
				}
			}
			else
			{
				selectedSpriteIdx = spriteIdx;
				// if any colour hot-key is pressed, make it set here and clear everywhere else
				if (auto* input = ::System::Input::get())
				{
					for_every(csc, spriteShortCuts)
					{
						if (input->is_key_pressed((System::Key::Type)csc->keyCode))
						{
							csc->selectedSpriteIdx = spriteIdx;
						}
					}
				}
				update_ui_highlight();
			}
		});
}

void Editor::SpriteGrid::show_palette(bool _show, bool _forcePlaceAt)
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorSpriteGridPalette)));

	if (_show)
	{
		if (!existing)
		{
			Framework::UI::ICanvasElement* above = nullptr;
			if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
			{
				auto& sgp = vm->get_palette();
				auto* menu = Framework::UI::CanvasWindow::get();
				existing = menu;
				menu->set_closable(true);
				menu->set_id(NAME(editorSpriteGridPalette));
				menu->set_title(TXT("palette"));
				menu->set_on_moved([this](UI::CanvasWindow* _window) { store_windows_at(); });
				menu->set_size(Vector2(0.3f, 0.4f) * c->get_logical_size());
				c->add(menu);
				{
					Framework::UI::ICanvasElement* belowPanel = nullptr;
					// management buttons
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						menu->add(panel);
						{
							{
								auto* b = Framework::UI::CanvasButton::get();
								b->set_caption(TXT("add"));
								b->set_scale(0.7f)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* sg = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
										{
											UI::Utils::choose_library_stored<Framework::Sprite>(get_canvas(), NAME(chooseLibraryStored_addSprite), TXT("add sprite to palette"), lastSpriteAdded.get(), 1.0f,
												[this, sg](Framework::Sprite* _chosen)
												{
													selectedSpriteIdx = sg->access_palette().add(_chosen);
													sg->access_palette().cache();
													reshow_palette();
													do_operation(0, Operation::ChangedSpritePalette);
												});
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get();
								b->set_caption(TXT("swap"));
								b->set_id(NAME(editorSpriteGridPalette_swap));
								b->set_scale(0.7f)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										swappingPalette = !swappingPalette;
										update_ui_highlight();
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get();
								b->set_caption(TXT("sort"));
								b->set_scale(0.7f)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* sg = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
										{
											auto* s = sg->get_palette().get(selectedSpriteIdx);
											sg->access_palette().sort();
											sg->access_palette().cache();
											selectedSpriteIdx = sg->get_palette().find_index(s);
											reshow_palette();
											do_operation(0, Operation::ChangedSpritePalette);
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get();
								b->set_caption(TXT("prune"));
								b->set_scale(0.7f)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* sg = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
										{
											auto* s = sg->get_palette().get(selectedSpriteIdx);
											sg->access_palette().remove_unused();
											sg->access_palette().cache();
											selectedSpriteIdx = sg->get_palette().find_index(s);
											reshow_palette();
											do_operation(0, Operation::ChangedSpritePalette);
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get();
								b->set_caption(TXT("remove"));
								b->set_scale(0.7f)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* sg = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
										{
											sg->access_palette().clear_at_and_in_grid(selectedSpriteIdx);
											sg->access_palette().cache();
											reshow_palette();
											do_operation(0, Operation::ChangedSpritePalette);
										}
									});
								panel->add(b);
							}
							panel->place_content_on_grid(c, VectorInt2(0,1), NP, Vector2::zero);
						}
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, 0.0f);
					}
					// actual sprites
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						menu->add(panel);
						VectorInt2 spriteSize = vm->get_sprite_size();
						Range2 menuPlacement = menu->get_inner_placement(c);
						Range2 belowPanelPlacement = belowPanel->get_placement(c);
						Vector2 panelSize = Vector2::zero;
						panelSize.x = menuPlacement.x.length();
						panelSize.y = belowPanelPlacement.y.min - menuPlacement.y.min;
						panel->set_use_scroll_horizontal(true);
						panel->set_size(panelSize);
						Range2 panelInnerSize = panel->get_inner_placement(c);
						int amountY = 4;
						float wantedSpriteSizeY = (panelInnerSize.y.length() / (float)amountY);
						Vector2 useSpriteSize = spriteSize.to_vector2() * (wantedSpriteSizeY / spriteSize.to_vector2().y);
						{
							VectorInt2 at = VectorInt2::zero;
							for_count(int, idx, sgp.get_count())
							{
								auto* b = Framework::UI::CanvasButton::get()->set_size(useSpriteSize)->set_sprite(sgp.get(idx));
								setup_palette_button(b, idx);
								panel->add(b);
								b->set_frame_width_pt(0.1f);
								b->set_use_frame_width_only_for_action();
								b->set_inner_frame_with_background();
								b->set_anchor_rel_placement(Vector2::zero); // lb
								b->set_at(VectorInt2(at.x, amountY - 1 - at.y).to_vector2() * useSpriteSize);
								{
									at.y += 1;
									if (at.y >= amountY)
									{
										at.x += 1;
										at.y = 0;
									}
								}
							}
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

	scroll_to_show_selected_sprite_on_palette();
}

void Editor::SpriteGrid::scroll_to_show_selected_sprite_on_palette()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* w = fast_cast<Framework::UI::CanvasWindow>(c->find_by_id(NAME(editorSpriteGridPalette))))
	{
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteGridPalette_sprite), selectedSpriteIdx)))
		{
			w->scroll_to_show_element(c, b);
		}
	}
}

void Editor::SpriteGrid::update_ui_highlight()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteGrid_addRemove))))
	{
		b->set_highlighted(mode == Mode::AddRemove);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteGrid_add))))
	{
		b->set_highlighted(mode == Mode::Add);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteGrid_remove))))
	{
		b->set_highlighted(mode == Mode::Remove);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteGrid_spritePicker))))
	{
		b->set_highlighted(mode == Mode::GetSprite);
	}
	auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset());
	if (vm)
	{
		if (selectedSpriteIdx >= 0)
		{
			selectedSpriteIdx = clamp(selectedSpriteIdx, 0, vm->get_palette().get_count() - 1);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteGridPalette_swap))))
		{
			b->set_highlighted(swappingPalette);
		}
		for_count(int, idx, vm->get_palette().get_count())
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteGridPalette_sprite), idx)))
			{
				b->set_highlighted(selectedSpriteIdx == idx);
				tchar caption = 0;
				for_every(csc, spriteShortCuts)
				{
					if (csc->selectedSpriteIdx == idx)
					{
						caption = csc->keyVisible;
						break;
					}
				}
				if (idx >= 0)
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

	{
		for_count(int, i, 2)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(i == 0? NAME(editorSpriteGrid_props_spriteSizeX) : NAME(editorSpriteGrid_props_spriteSizeY))))
			{
				b->set_caption(vm ? String::printf(TXT("%i"), vm->get_sprite_size().get_element(i)).to_char() : TXT("--"));
			}
		}
	}

	base::update_ui_highlight();
}

void Editor::SpriteGrid::show_asset_props()
{
	auto* c = get_canvas();

	if (auto* e = c->find_by_id(NAME(editorSpriteGrid_props)))
	{
		e->remove();
		return;
	}

	if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
	{
		UI::Utils::Grid2Menu menu;

		auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("sprite props"));
		window->set_id(NAME(editorSpriteGrid_props));
		window->set_modal(false)->set_closable(true);
		add_common_props_to_asset_props(menu);
		menu.step_2_add_text_and_button(TXT("add sprite palette from"), TXT("[sprite grid]"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::P)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					{
						UI::Utils::choose_library_stored<Framework::SpriteGrid>(_context.canvas, NAME(chooseLibraryStored_spriteGrid), TXT("sprite grid"), fast_cast<Framework::SpriteGrid>(get_edited_asset()), 1.0f,
							[this](Framework::SpriteGrid* _chosen)
							{
								if (auto* sg = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
								{
									sg->access_palette().fill_with(_chosen->get_palette());
									sg->access_palette().cache();
									editorContext->mark_save_required();
								}
								reshow_palette();
							});
					}
				});
		for_count(int, i, 2)
		{
			menu.step_2_add_text_and_button(String::printf(TXT("sprite size %c"), i == 0? 'x' : 'y').to_char(), TXT("--"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(i == 0 ? System::Key::S : System::Key::D)
#endif
				->set_on_press([this, i](Framework::UI::ActContext const& _context)
					{
						if (auto* vm = fast_cast<Framework::SpriteGrid>(get_edited_asset()))
						{
							UI::Utils::edit_text(_context.canvas, TXT("sprite size"), String::printf(TXT("%i"), vm->get_sprite_size().get_element(i)), [this, i, vm](String const& _text)
								{
									int v = ParserUtils::parse_int(_text);
									if (v > 0)
									{
										VectorInt2 ps = vm->get_sprite_size();
										ps.access_element(i) = v;
										vm->set_sprite_size(ps);
										do_operation(NONE, Operation::ChangedSpriteGridProps);

									}
								});
						}
					})
				->set_id(i == 0? NAME(editorSpriteGrid_props_spriteSizeX) : NAME(editorSpriteGrid_props_spriteSizeY));
		}
		menu.step_3_add_no_button();
		menu.step_3_add_button(TXT("close"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([window](Framework::UI::ActContext const& _context)
				{
					window->remove();
				});

		menu.step_4_finalise(c->get_logical_size().x * 0.5f);

		update_ui_highlight();
	}
}
