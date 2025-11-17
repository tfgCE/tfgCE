#include "editorSprite.h"

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

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\system\input.h"
#include "..\..\..\core\system\core.h"
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
DEFINE_STATIC_NAME(editorSprite_mode);
DEFINE_STATIC_NAME(editorSprite_drawEdges);
DEFINE_STATIC_NAME(editorSprite_onlySolidPixels);
DEFINE_STATIC_NAME(editorLayers_open);
DEFINE_STATIC_NAME(editorLayers_at);
DEFINE_STATIC_NAME(editorLayers_atValid);
DEFINE_STATIC_NAME(editorLayerProps_open);
DEFINE_STATIC_NAME(editorLayerProps_at);
DEFINE_STATIC_NAME(editorLayerProps_atValid);

// ui
DEFINE_STATIC_NAME(editorSprite_props);
DEFINE_STATIC_NAME(editorSprite_props_spriteGridDrawPriority);
DEFINE_STATIC_NAME(editorSprite_props_pixelSizeX);
DEFINE_STATIC_NAME(editorSprite_props_pixelSizeY);
DEFINE_STATIC_NAME(editorSprite_props_zeroOffsetX);
DEFINE_STATIC_NAME(editorSprite_props_zeroOffsetY);
DEFINE_STATIC_NAME(editorSprite_addRemove);
DEFINE_STATIC_NAME(editorSprite_add);
DEFINE_STATIC_NAME(editorSprite_remove);
DEFINE_STATIC_NAME(editorSprite_fill);
DEFINE_STATIC_NAME(editorSprite_paint);
DEFINE_STATIC_NAME(editorSprite_paintFill);
DEFINE_STATIC_NAME(editorSprite_replaceColour);
DEFINE_STATIC_NAME(editorSprite_colourPicker);
DEFINE_STATIC_NAME(editorSprite_select);
DEFINE_STATIC_NAME(editorSprite_move);

DEFINE_STATIC_NAME(editorSprite_editedLayerName);
DEFINE_STATIC_NAME(editorSprite_editedLayerNameWindow);

DEFINE_STATIC_NAME(editorLayers);
DEFINE_STATIC_NAME(editorLayers_layerList);

DEFINE_STATIC_NAME(editorLayerProps);
DEFINE_STATIC_NAME(editorLayerProps_offsetX);
DEFINE_STATIC_NAME(editorLayerProps_offsetY);
DEFINE_STATIC_NAME(editorLayerProps_rotateYaw);
DEFINE_STATIC_NAME(editorLayerProps_mirrorNone);
DEFINE_STATIC_NAME(editorLayerProps_mirrorX);
DEFINE_STATIC_NAME(editorLayerProps_mirrorY);
DEFINE_STATIC_NAME(editorLayerProps_mirrorAt);
DEFINE_STATIC_NAME(editorLayerProps_mirrorAxisYes);
DEFINE_STATIC_NAME(editorLayerProps_mirrorAxisNo);
DEFINE_STATIC_NAME(editorLayerProps_mirrorDirN);
DEFINE_STATIC_NAME(editorLayerProps_mirrorDirZ);
DEFINE_STATIC_NAME(editorLayerProps_mirrorDirP);
DEFINE_STATIC_NAME(editorLayerProps_includedSprite);

DEFINE_STATIC_NAME(chooseLibraryStored_includeSprite);

//--

REGISTER_FOR_FAST_CAST(Editor::Sprite);

Editor::Sprite::Sprite()
: colourPaletteComponent(this)
{
}

Editor::Sprite::~Sprite()
{
}

#define STORE_RESTORE(_op) \
	_op(_setup, mode, NAME(editorSprite_mode)); \
	_op(_setup, drawEdges, NAME(editorSprite_drawEdges)); \
	_op(_setup, onlySolidPixels, NAME(editorSprite_onlySolidPixels)); \
	_op(_setup, windows.layers.open, NAME(editorLayers_open)); \
	_op(_setup, windows.layers.at, NAME(editorLayers_at)); \
	_op(_setup, windows.layers.atValid, NAME(editorLayers_atValid)); \
	_op(_setup, windows.layerProps.open, NAME(editorLayerProps_open)); \
	_op(_setup, windows.layerProps.at, NAME(editorLayerProps_at)); \
	_op(_setup, windows.layerProps.atValid, NAME(editorLayerProps_atValid)); \
	;

void Editor::Sprite::edit(LibraryStored* _asset, String const& _filePathToSave)
{
	base::edit(_asset, _filePathToSave);

	update_edited_layer_name();

	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		if (CONTENT_ACCESS::get_layers(vm).is_empty())
		{
			auto* l = new Framework::SpriteLayer();
			CONTENT_ACCESS::add_layer(vm, l, 0);
			populate_layer_list();
		}
	}
}


void Editor::Sprite::restore_for_use(SimpleVariableStorage const& _setup)
{
	base::restore_for_use(_setup);
	STORE_RESTORE(read);
	colourPaletteComponent.restore_for_use(_setup);
	update_ui_highlight();
	restore_windows(true);
}

void Editor::Sprite::store_for_later(SimpleVariableStorage& _setup) const
{
	base::store_for_later(_setup);
	STORE_RESTORE(write);
	colourPaletteComponent.store_for_later(_setup);
}

void Editor::Sprite::restore_windows(bool _forcePlaceAt)
{
	colourPaletteComponent.restore_windows(_forcePlaceAt);
	if (auto* c = get_canvas())
	{
		show_layers(windows.layers.open, _forcePlaceAt);
		show_layer_props(windows.layerProps.open, _forcePlaceAt);
	}
}

void Editor::Sprite::store_windows_at()
{
	colourPaletteComponent.store_windows_at();
	if (auto* c = get_canvas())
	{
		if (auto* w = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorLayers))))
		{
			windows.layers.at = w->get_placement(c).get_at(Vector2(0.0f, 1.0f));
			windows.layers.atValid = true;
		}
		if (auto* w = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorLayerProps))))
		{
			windows.layerProps.at = w->get_placement(c).get_at(Vector2(0.0f, 1.0f));
			windows.layerProps.atValid = true;
		}
	}
}

void Editor::Sprite::on_hover(int _cursorIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);
}

void Editor::Sprite::on_press(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left)
	{
		if (mode == Mode::AddRemove)
		{
			do_operation(_cursorIdx, Operation::AddPixel);
			return;
		}
		if (mode == Mode::Fill)
		{
			do_operation(_cursorIdx, Operation::FillAtPixel);
			return;
		}
	}

	base::on_press(_cursorIdx, _buttonIdx, _rayStart);
}

void Editor::Sprite::on_click(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Right)
	{
		if (mode == Mode::AddRemove)
		{
			do_operation(_cursorIdx, Operation::RemovePixel);
			return;
		}
		if (mode == Mode::Fill)
		{
			do_operation(_cursorIdx, Operation::FillRemoveAtPixel);
			return;
		}
	}

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left)
	{
		if (mode == Mode::Select)
		{
			if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
			{
				if (CONTENT_ACCESS::merge_temporary_layers(vm))
				{
					populate_layer_list();
				}
				do_operation(NONE, Operation::ChangedLayerProps);
			}
			selection = Selection();
			return;
		}
	}

	base::on_click(_cursorIdx, _buttonIdx, _rayStart);
}

bool Editor::Sprite::on_hold_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left &&
		(mode == Mode::AddRemove || mode == Mode::Add || mode == Mode::Remove || mode == Mode::Paint || mode == Mode::PaintFill || mode == Mode::ReplaceColour || mode == Mode::GetColour || mode == Mode::Select))
	{
		if (cursorRays.is_index_valid(_cursorIdx))
		{
			auto& cr = cursorRays[_cursorIdx];

			if (mode == Mode::AddRemove)
			{
				do_operation(_cursorIdx, Operation::AddPixel, true);
			}
			if (mode == Mode::Add)
			{
				do_operation(_cursorIdx, Operation::AddPixel, true);
			}
			if (mode == Mode::Remove)
			{
				do_operation(_cursorIdx, Operation::RemovePixel, true);
			}
			if (mode == Mode::Paint)
			{
				do_operation(_cursorIdx, Operation::PaintPixel, true);
			}
			if (mode == Mode::PaintFill)
			{
				do_operation(_cursorIdx, Operation::PaintFillPixel, true);
			}
			if (mode == Mode::ReplaceColour)
			{
				do_operation(_cursorIdx, Operation::ReplaceColour, true);
			}
			if (mode == Mode::GetColour)
			{
				do_operation(_cursorIdx, Operation::GetColourFromPixel, true);
			}
			if (mode == Mode::Select)
			{
				if (cr.cellCoord.is_set())
				{
					selection.draggingTo = cr.cellCoord.get();
					if (!selection.startedAt.is_set())
					{
						selection.startedAt = cr.cellCoord.get();
					}
				}
				else
				{
					selection.draggingTo = selection.startedAt;
				}
				selection.range = Range2::empty;
				if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
				{
					if (CONTENT_ACCESS::merge_temporary_layers(vm))
					{
						populate_layer_list();
					}
					if (selection.startedAt.is_set())
					{
						selection.range.include((selection.startedAt.get().to_vector2() - vm->get_zero_offset()) * vm->get_pixel_size());
					}
					if (selection.draggingTo.is_set())
					{
						selection.range.include((selection.draggingTo.get().to_vector2() - vm->get_zero_offset()) * vm->get_pixel_size());
					}
					selection.range.x.max += vm->get_pixel_size().x;
					selection.range.y.max += vm->get_pixel_size().y;
				}
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
				do_operation(_cursorIdx, Operation::RemovePixel, true);
			}

			inputGrabbed = false;
		}

		return true;
	}

	return false;
}

bool Editor::Sprite::on_hold(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	if (on_hold_held(_cursorIdx, _buttonIdx, _rayStart))
	{
		return true;
	}

	return base::on_hold(_cursorIdx, _buttonIdx, _rayStart);
}

bool Editor::Sprite::on_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart, Vector2 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised)
{
	if (on_hold_held(_cursorIdx, _buttonIdx, _rayStart))
	{
		return true;
	}

	return base::on_held(_cursorIdx, _buttonIdx, _rayStart, _movedBy, _cursorMovedBy2DNormalised);
}

void Editor::Sprite::on_release(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left)
	{
		lastOperationCell.clear();
		// selection.range should be already set
		selection.startedAt.clear();
		selection.draggingTo.clear();
	}

	base::on_release(_cursorIdx, _buttonIdx, _rayStart);
}

void Editor::Sprite::process_controls()
{
	base::process_controls();

	highlightEditedLayerFor = max(0.0f, highlightEditedLayerFor - ::System::Core::get_raw_delta_time());

	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	if (!canvas->is_modal_window_active())
	{
		colourPaletteComponent.process_controls();
	}

	// manage gizmos
	//

	// active pixel layer
	{
		Framework::SpriteLayer * activePixelLayer = nullptr;
		auto* vm = fast_cast<Framework::Sprite>(get_edited_asset());
		if (vm && (mode == Mode::Move || mode == Mode::Select))
		{
			activePixelLayer = fast_cast<Framework::SpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm));
		}
		Range2 forSelectionRangeNow = selection.range;
		if (activePixelLayer)
		{
			if (pixelLayerGizmos.forLayer != activePixelLayer ||
				pixelLayerGizmos.forSelectionRange != forSelectionRangeNow)
			{
				remove_pixel_layer_gizmos();
			}

			// create if required
			if (!pixelLayerGizmos.offsetGizmo.is_in_use())
			{
				if (! forSelectionRangeNow.is_empty())
				{
					pixelLayerGizmos.zeroOffsetAt = forSelectionRangeNow.centre();
				}
				else
				{
					RangeInt2 vr = activePixelLayer->get_range();
					if (vr.is_empty())
					{
						vr = CONTENT_ACCESS::get_combined_range(vm);
					}
					if (vr.is_empty())
					{
						pixelLayerGizmos.zeroOffsetAt = Vector2::zero;
					}
					else
					{
						VectorInt2 cLocal = vr.is_empty() ? VectorInt2::zero : vr.centre();
						VectorInt2 cWorld = activePixelLayer->to_world(vm, cLocal);
						pixelLayerGizmos.zeroOffsetAt = vm->to_full_coord(cWorld);
					}
				}
				pixelLayerGizmos.offsetGizmo.atWorld = pixelLayerGizmos.zeroOffsetAt;
				pixelLayerGizmos.offsetGizmo.fromWorld = Vector2::zero;
				pixelLayerGizmos.forLayer = activePixelLayer;
				pixelLayerGizmos.forSelectionRange = forSelectionRangeNow;
			}

			// update
			bool didSomethingChange = false;
			{
				// update main offset gizmo now
				pixelLayerGizmos.offsetGizmo.update(this);

				{
					bool makeCopy = false;
					{
						// outside offset is zero as we want to update this even if not holding/grabbing
						if (!grabbedAnyGizmo)
						{
							pixelLayerGizmos.allowMakeCopy = true;
						}
						if (auto* input = ::System::Input::get())
						{
#ifdef AN_STANDARD_INPUT
							if (input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift))
							{
								if (pixelLayerGizmos.allowMakeCopy)
								{
									makeCopy = true;
								}
							}
							else
#endif
							{
								pixelLayerGizmos.allowMakeCopy = true;
							}
						}
					}

					VectorInt2 offset = TypeConversions::Normal::f_i_closest((pixelLayerGizmos.offsetGizmo.atWorld - pixelLayerGizmos.zeroOffsetAt) / vm->get_pixel_size());
					if (!offset.is_zero())
					{
						// if we're trying to move selection, move it to a separate layer
						// if we're holding shift, make a copy
						if (!forSelectionRangeNow.is_empty() || makeCopy)
						{
							bool makeTemporary = false;
							if (!forSelectionRangeNow.is_empty())
							{
								do_operation(NONE, makeCopy? Operation::DuplicateSelectionToNewLayer : Operation::MoveSelectionToNewLayer);
								makeTemporary = true;
							}
							else
							{
								an_assert(makeCopy);
								CONTENT_ACCESS::duplicate_edited_layer(vm, 0);
							}
							activePixelLayer = fast_cast<SpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm));
							if (activePixelLayer && makeTemporary)
							{
								activePixelLayer->be_temporary();
							}
							CONTENT_ACCESS::merge_temporary_layers(vm, activePixelLayer);
							selection = Selection();

							// but keep gizmos
							pixelLayerGizmos.forLayer = activePixelLayer;
							pixelLayerGizmos.forSelectionRange = selection.range;
							pixelLayerGizmos.allowMakeCopy = !makeCopy;

							combine_pixels();
							populate_layer_list();
						}
						else
						{
							activePixelLayer->offset_placement(offset);
							pixelLayerGizmos.zeroOffsetAt += offset.to_vector2() * vm->get_pixel_size();
							didSomethingChange = true;
						}
					}
				}
			}

			if (didSomethingChange)
			{
				do_operation(NONE, Operation::ChangedLayerProps);
			}
		}
		else
		{
			remove_pixel_layer_gizmos();
		}
	}

	// active transform layer
	{
		Framework::SpriteTransformLayer * activeTransformLayer = nullptr;
		auto* vm = fast_cast<Framework::Sprite>(get_edited_asset());
		if (vm)
		{
			activeTransformLayer = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm));
		}
		if (activeTransformLayer)
		{
			if (transformLayerGizmos.forLayer != activeTransformLayer)
			{
				remove_transform_layer_gizmos();
			}

			// create if required
			if (!transformLayerGizmos.offsetGizmo.is_in_use())
			{
				transformLayerGizmos.offsetGizmo.atWorld = activeTransformLayer->access_offset().to_vector2() * vm->get_pixel_size();
				transformLayerGizmos.offsetGizmo.fromWorld = Vector2::zero;
				transformLayerGizmos.forLayer = activeTransformLayer;
			}

			auto& mirror = activeTransformLayer->access_mirror();
			if ((mirror.mirrorAxis == NONE || mirror.mirrorAxis != transformLayerGizmos.mirrorAtGizmoForAxis) && transformLayerGizmos.mirrorAtGizmo.get())
			{
				transformLayerGizmos.mirrorAtGizmo.clear();
				transformLayerGizmos.mirrorAtGizmoForAxis = NONE;
			}
			else if (mirror.mirrorAxis != NONE)
			{
				transformLayerGizmos.mirrorAtGizmoForAxis = mirror.mirrorAxis;
				if (!transformLayerGizmos.mirrorAtGizmo.get())
				{
					transformLayerGizmos.mirrorAtGizmo = Gizmo2D::get_one();
					auto* g = transformLayerGizmos.mirrorAtGizmo.get();
					add_gizmo(g);
					g->colour = Colour::c64LightBlue;
					g->radius2d *= 0.8f;
					g->axisWorld = Vector2::zero;
					g->axisWorld.access().access_element(transformLayerGizmos.mirrorAtGizmoForAxis) = 1.0f;
					g->atWorld = Vector2::zero;
					g->atWorld.access_element(transformLayerGizmos.mirrorAtGizmoForAxis) = (float)(mirror.at + (mirror.axis? 0.5f: 0.0f)) * (mirror.mirrorAxis == 0? vm->get_pixel_size().x : vm->get_pixel_size().y);
					g->atWorld += transformLayerGizmos.offsetGizmo.atWorld;
				}
			}

			// update
			bool didSomethingChange = false;
			{

				Vector2 mirrorAtRelative = Vector2::zero; // use this to move against main offset gizmo

				// read before we update offset gizmo, so we have the latest movement
				if (auto* g = transformLayerGizmos.mirrorAtGizmo.get())
				{
					int prevAt = mirror.at;
					mirrorAtRelative = g->atWorld - transformLayerGizmos.offsetGizmo.atWorld;
					float use = mirrorAtRelative.get_element(transformLayerGizmos.mirrorAtGizmoForAxis);
					if (mirror.axis)
					{
						mirror.at = TypeConversions::Normal::f_i_cells(use / (mirror.mirrorAxis == 0 ? vm->get_pixel_size().x : vm->get_pixel_size().y));
					}
					else
					{
						mirror.at = TypeConversions::Normal::f_i_closest(use / (mirror.mirrorAxis == 0 ? vm->get_pixel_size().x : vm->get_pixel_size().y));
					}
					didSomethingChange |= mirror.at != prevAt;
				}

				// update main offset gizmo now
				transformLayerGizmos.offsetGizmo.update(this);

				{
					VectorInt2& offset = activeTransformLayer->access_offset();
					VectorInt2 prevOffset = offset;
					offset = TypeConversions::Normal::f_i_closest(transformLayerGizmos.offsetGizmo.atWorld / vm->get_pixel_size());
					didSomethingChange |= offset != prevOffset;
				}

				if (auto* g = transformLayerGizmos.mirrorAtGizmo.get())
				{
					g->atWorld = mirrorAtRelative + transformLayerGizmos.offsetGizmo.atWorld;
				}
			}

			if (didSomethingChange)
			{
				do_operation(NONE, Operation::ChangedLayerProps);
			}
		}
		else
		{
			remove_transform_layer_gizmos();
		}
	}

	// active full transform layer
	{
		Framework::SpriteFullTransformLayer * activeTransformLayer = nullptr;
		auto* vm = fast_cast<Framework::Sprite>(get_edited_asset());
		if (vm)
		{
			activeTransformLayer = fast_cast<Framework::SpriteFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm));
		}
		if (activeTransformLayer)
		{
			if (fullTransformLayerGizmos.forLayer != activeTransformLayer)
			{
				remove_full_transform_layer_gizmos();
			}

			// create if required
			if (!fullTransformLayerGizmos.offsetAndRotationGizmo.is_in_use())
			{
				fullTransformLayerGizmos.offsetAndRotationGizmo.atWorld = activeTransformLayer->access_offset() * vm->get_pixel_size();
				fullTransformLayerGizmos.offsetAndRotationGizmo.fromWorld = Vector2::zero;
				fullTransformLayerGizmos.offsetAndRotationGizmo.rotation = activeTransformLayer->access_rotate();
				fullTransformLayerGizmos.forLayer = activeTransformLayer;
			}

			// update
			bool didSomethingChange = false;
			{
				// update main offset gizmo now
				fullTransformLayerGizmos.offsetAndRotationGizmo.update(this);

				{
					Vector2& offset = activeTransformLayer->access_offset();
					Rotator3& rotate = activeTransformLayer->access_rotate();
					Vector2 prevOffset = offset;
					Rotator3 prevRotate = rotate;
					offset = fullTransformLayerGizmos.offsetAndRotationGizmo.atWorld / vm->get_pixel_size();
					rotate = fullTransformLayerGizmos.offsetAndRotationGizmo.rotation;
					didSomethingChange |= offset != prevOffset || rotate != prevRotate;
				}
			}

			if (didSomethingChange)
			{
				do_operation(NONE, Operation::ChangedLayerProps);
			}
		}
		else
		{
			remove_full_transform_layer_gizmos();
		}
	}

	// selection
	{
		bool shouldBePresent = !selection.range.is_empty() && !selection.startedAt.is_set();
		if (shouldBePresent)
		{
			// create if needed
			{
				if (!selection.gizmos.rangeXMinGizmo.get())
				{
					selection.gizmos.rangeXMinGizmo = Gizmo2D::get_one();
					auto* g = selection.gizmos.rangeXMinGizmo.get();
					g->atWorld.x = selection.range.x.min + selection.gizmosOffDistance.x.min;
					g->atWorld.y = selection.range.y.centre();
					g->axisWorld = Vector2::xAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeXMaxGizmo.get())
				{
					selection.gizmos.rangeXMaxGizmo = Gizmo2D::get_one();
					auto* g = selection.gizmos.rangeXMaxGizmo.get();
					g->atWorld.x = selection.range.x.max + selection.gizmosOffDistance.x.max;
					g->atWorld.y = selection.range.y.centre();
					g->axisWorld = Vector2::xAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeYMinGizmo.get())
				{
					selection.gizmos.rangeYMinGizmo = Gizmo2D::get_one();
					auto* g = selection.gizmos.rangeYMinGizmo.get();
					g->atWorld.x = selection.range.x.centre();
					g->atWorld.y = selection.range.y.min + selection.gizmosOffDistance.y.min;
					g->axisWorld = Vector2::yAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeYMaxGizmo.get())
				{
					selection.gizmos.rangeYMaxGizmo = Gizmo2D::get_one();
					auto* g = selection.gizmos.rangeYMaxGizmo.get();
					g->atWorld.x = selection.range.x.centre();
					g->atWorld.y = selection.range.y.max + selection.gizmosOffDistance.y.max;
					g->axisWorld = Vector2::yAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
			}
			selection.range = Range2::empty;
			selection.range.x.include(selection.gizmos.rangeXMinGizmo->atWorld.x - selection.gizmosOffDistance.x.min);
			selection.range.x.include(selection.gizmos.rangeXMaxGizmo->atWorld.x - selection.gizmosOffDistance.x.max);
			selection.range.y.include(selection.gizmos.rangeYMinGizmo->atWorld.y - selection.gizmosOffDistance.y.min);
			selection.range.y.include(selection.gizmos.rangeYMaxGizmo->atWorld.y - selection.gizmosOffDistance.y.max);

			selection.gizmos.rangeXMinGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeXMaxGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeYMinGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeYMaxGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeXMinGizmo->atWorld.x = selection.range.x.min;
			selection.gizmos.rangeXMaxGizmo->atWorld.x = selection.range.x.max;
			selection.gizmos.rangeYMinGizmo->atWorld.y = selection.range.y.min;
			selection.gizmos.rangeYMaxGizmo->atWorld.y = selection.range.y.max;

			selection.gizmosOffDistance.x.min = -calculate_radius_for(selection.gizmos.rangeXMinGizmo->atWorld, selection.gizmos.rangeXMinGizmo->radius2d);
			selection.gizmosOffDistance.x.max =  calculate_radius_for(selection.gizmos.rangeXMaxGizmo->atWorld, selection.gizmos.rangeXMaxGizmo->radius2d);
			selection.gizmosOffDistance.y.min = -calculate_radius_for(selection.gizmos.rangeYMinGizmo->atWorld, selection.gizmos.rangeYMinGizmo->radius2d);
			selection.gizmosOffDistance.y.max =  calculate_radius_for(selection.gizmos.rangeYMaxGizmo->atWorld, selection.gizmos.rangeYMaxGizmo->radius2d);

			selection.gizmos.rangeXMinGizmo->atWorld.x += selection.gizmosOffDistance.x.min;
			selection.gizmos.rangeXMaxGizmo->atWorld.x += selection.gizmosOffDistance.x.max;
			selection.gizmos.rangeYMinGizmo->atWorld.y += selection.gizmosOffDistance.y.min;
			selection.gizmos.rangeYMaxGizmo->atWorld.y += selection.gizmosOffDistance.y.max;
		}
		else
		{
			selection.gizmos = Selection::Gizmos();
		}
	}

	// update windows
	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		auto* el = CONTENT_ACCESS::access_edited_layer(vm);
		if (windows.layerPropsForLayer != el ||
			windows.layerPropsForMode != mode)
		{
			reshow_layer_props();
			// to avoid doing this when there is no layer props window
			windows.layerPropsForLayer = el;
			windows.layerPropsForMode = mode;
		}
	}
}

void Editor::Sprite::update_cursor_ray(int _cursorIdx, Vector2 const& _rayStart)
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

	bool onlyActualContent = false;
	{
		if (auto* input = ::System::Input::get())
		{
#ifdef AN_STANDARD_INPUT
			bool ctrlPressed = input->is_key_pressed(System::Key::LeftCtrl) || input->is_key_pressed(System::Key::RightCtrl);
			if (ctrlPressed)
			{
				onlyActualContent = true;
			}
#endif
		}
	}

	if (cr.rayStart.is_set() && cr.rayStart == _rayStart &&
		cr.onlyActualContent.is_set() && cr.onlyActualContent == onlyActualContent)
	{
		// already cached
		return;
	}

	cr.rayStart = _rayStart;
	cr.onlyActualContent = onlyActualContent;

	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		Framework::SpritePixelHitInfo hitInfo;
		if (CONTENT_ACCESS::ray_cast(vm, _rayStart, hitInfo, onlyActualContent))
		{
			cr.hitPixel = true;
			cr.cellCoord = hitInfo.hit;
		}
		else
		{
			if (CONTENT_ACCESS::ray_cast_xy(vm, _rayStart, hitInfo))
			{
				cr.hitPixel = false;
				cr.cellCoord = hitInfo.hit;
			}
		}
	}
}

void Editor::Sprite::do_operation(int _cursorIdx, Operation _operation, bool _checkLastOperationCell)
{
	bool didSomething = false;

	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		if (cursorRays.is_index_valid(_cursorIdx))
		{
			auto& cr = cursorRays[_cursorIdx];

			if (cr.cellCoord.is_set())
			{
				auto* vml = fast_cast<SpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm));
				if (vml)
				{
					RangeInt2 r = CONTENT_ACCESS::get_combined_range(vm);
					if (r.is_empty())
					{
						r = RangeInt2::zero;
					}
					// allow to add only close to already existing blocks
					int extraRange = 200;
					r.expand_by(VectorInt2(extraRange, extraRange));
					struct ValidRange
					{
						RangeInt2 validRange = RangeInt2::empty;
						void setup(Framework::Sprite* vm, Range2 const& _r)
						{
							if (!_r.is_empty())
							{
								validRange = vm->to_pixel_range(_r, SELECTION_FULLY_INLCUDED);
							}
						}
						bool is_ok(VectorInt2 const& _a) const { return validRange.is_empty() || validRange.does_contain(_a); }
					} validRange;
					validRange.setup(vm, selection.range);
					if (r.does_contain(cr.cellCoord.get()))
					{
						if (_operation == Operation::AddPixel)
						{
							VectorInt2 at = cr.cellCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								VectorInt2 useAt = at;
								if (validRange.is_ok(useAt))
								{
									VectorInt2 useAtLocal = vml->to_local(vm, useAt);
									auto& pixel = vml->access_at(useAtLocal);
									pixel.pixel = get_selected_colour_index();
									didSomething = true;
									lastOperationCell = at;
									CONTENT_ACCESS::prune(vm);
									combine_pixels();
								}
							}
						}
						if (_operation == Operation::FillAtPixel ||
							_operation == Operation::FillRemoveAtPixel)
						{
							VectorInt2 at = cr.cellCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								VectorInt2 useAt = at;
								if (validRange.is_ok(useAt))
								{
									int useColourIdx = _operation == Operation::FillRemoveAtPixel ? Framework::SpritePixel::None : get_selected_colour_index();
									Optional<int> startColour;
									CONTENT_ACCESS::fill_at(vm, vml, useAt, [&startColour, useColourIdx](SpritePixel& _pixel)
										{
											if (!startColour.is_set())
											{
												startColour = _pixel.pixel;
											}
											if (_pixel.pixel == startColour.get())
											{
												_pixel.pixel = useColourIdx;
												return true;
											}
											return false;
										});
									didSomething = true;
									lastOperationCell = at;
									CONTENT_ACCESS::prune(vm);
									combine_pixels();
								}
							}
						}
						if (_operation == Operation::RemovePixel)
						{
							VectorInt2 at = cr.cellCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								if (validRange.is_ok(at))
								{
									VectorInt2 atLocal = vml->to_local(vm, at);
									auto& pixel = vml->access_at(atLocal);
									pixel.pixel = Framework::SpritePixel::None;
									didSomething = true;
									lastOperationCell = at;
									CONTENT_ACCESS::prune(vm);
									combine_pixels();
								}
							}
						}
						if (_operation == Operation::PaintPixel)
						{
							VectorInt2 at = cr.cellCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								if (validRange.is_ok(at))
								{
									VectorInt2 atLocal = vml->to_local(vm, at);
									auto& pixel = vml->access_at(atLocal);
									if (Framework::SpritePixel::is_anything(pixel.pixel))
									{
										pixel.pixel = get_selected_colour_index();
										didSomething = true;
										lastOperationCell = at;
										CONTENT_ACCESS::prune(vm);
										combine_pixels();
									}
								}
							}
						}
						if (_operation == Operation::PaintFillPixel)
						{
							VectorInt2 at = cr.cellCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								VectorInt2 atLocal = vml->to_local(vm, at);
								if (validRange.is_ok(at))
								{
									auto& pixel = vml->access_at(atLocal);
									if (Framework::SpritePixel::is_anything(pixel.pixel) &&
										pixel.pixel != get_selected_colour_index())
									{
										{
											int replace = pixel.pixel;
											Array<VectorInt2> at;
											at.push_back(atLocal);
											while (!at.is_empty())
											{
												VectorInt2 atLocal = at.get_first();
												at.pop_front();
												auto& pixel = vml->access_at(atLocal);
												if (pixel.pixel == replace)
												{
													pixel.pixel = get_selected_colour_index();
													for_range(int, ox, -1, 1)
													{
														for_range(int, oy, -1, 1)
														{
															if (ox != 0 || oy != 0)
															{
																at.push_back(atLocal + VectorInt2(ox, oy));
															}
														}
													}
												}
											}
										}
										didSomething = true;
										lastOperationCell = at;
										CONTENT_ACCESS::prune(vm);
										combine_pixels();
									}
								}
							}
						}
						if (_operation == Operation::ReplaceColour)
						{
							VectorInt2 at = cr.cellCoord.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get()))
							{
								if (validRange.is_ok(at))
								{
									VectorInt2 atLocal = vml->to_local(vm, at);
									auto& pixel = vml->access_at(atLocal);
									if (Framework::SpritePixel::is_anything(pixel.pixel) &&
										pixel.pixel != get_selected_colour_index())
									{
										{
											int replace = pixel.pixel;
											for_every_ref(l, CONTENT_ACCESS::get_layers(vm))
											{
												if (auto* vml = fast_cast<SpriteLayer>(l))
												{
													auto range = vml->get_range();
													for_range(int, y, range.y.min, range.y.max)
													{
														for_range(int, x, range.x.min, range.x.max)
														{
															VectorInt2 atLocal(x, y);
															auto& pixel = vml->access_at(atLocal);
															if (pixel.pixel == replace)
															{
																pixel.pixel = get_selected_colour_index();
															}
														}
													}
												}
											}
										}
										didSomething = true;
										lastOperationCell = at;
										CONTENT_ACCESS::prune(vm);
										combine_pixels();
									}
								}
							}
						}
					}
				}
				if (_operation == Operation::GetColourFromPixel)
				{
					// colour can be read from any layer, top most first and without range/selection checks
					VectorInt2 at = cr.cellCoord.get();
					bool read = false;
					/*
					if (vml)
					{
						VectorInt2 atLocal = vml->to_local(vm, at);
						auto& pixel = vml->access_at(atLocal);
						if (Framework::SpritePixel::is_anything(pixel.pixel))
						{
							set_selected_colour_index(pixel.pixel);
							read = true;
						}
					}
					*/
					if (!read)
					{
						// read from any layer
						for_every_ref(l, CONTENT_ACCESS::get_layers(vm))
						{
							if (auto* vml = fast_cast<SpriteLayer>(l))
							{
								VectorInt2 atLocal = vml->to_local(vm, at);
								auto& pixel = vml->access_at(atLocal);
								if (Framework::SpritePixel::is_anything(pixel.pixel))
								{
									set_selected_colour_index(pixel.pixel);
									read = true;
									break;
								}
							}
						}
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

		if (auto* vml = fast_cast<SpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
		{
			if (_operation == Operation::FillSelection ||
				_operation == Operation::ClearSelection ||
				_operation == Operation::PaintSelection ||
				_operation == Operation::DuplicateSelectionToNewLayer ||
				_operation == Operation::MoveSelectionToNewLayer)
			{
				if (!selection.range.is_empty())
				{
					SpriteLayer* nvml = nullptr;
					if (_operation == Operation::DuplicateSelectionToNewLayer ||
						_operation == Operation::MoveSelectionToNewLayer)
					{
						nvml = new Framework::SpriteLayer();
						CONTENT_ACCESS::add_layer(vm, nvml, 0);
						populate_layer_list();
					}
					RangeInt2 range = vm->to_pixel_range(selection.range, SELECTION_FULLY_INLCUDED);
					// first copy to new layer if requested (mirrors may make use remove stuff before we copy it)
					if (nvml)
					{
						for_range(int, y, range.y.min, range.y.max)
						{
							for_range(int, x, range.x.min, range.x.max)
							{
								VectorInt2 atGlobal(x, y);
								VectorInt2 atLocal = vml->to_local(vm, atGlobal, true);
								auto& v = vml->access_at(atLocal);
								if (Framework::SpritePixel::is_anything(v.pixel))
								{
									VectorInt2 atLocalNVML = nvml->to_local(vm, atGlobal, true);
									auto& dstv = nvml->access_at(atLocalNVML);
									dstv = v;
								}
							}
						}
					}
					for_range(int, y, range.y.min, range.y.max)
					{
						for_range(int, x, range.x.min, range.x.max)
						{
							VectorInt2 atGlobal(x, y);
							VectorInt2 atLocal = vml->to_local(vm, atGlobal);
							auto& v = vml->access_at(atLocal);
							if (_operation == Operation::FillSelection)
							{
								v.pixel = get_selected_colour_index();
							}
							if (_operation == Operation::ClearSelection ||
								_operation == Operation::MoveSelectionToNewLayer)
							{
								v.pixel = Framework::SpritePixel::None;
							}
							if (_operation == Operation::PaintSelection && Framework::SpritePixel::is_anything(v.pixel))
							{
								v.pixel = get_selected_colour_index();
							}
						}
					}
				}
			}
			CONTENT_ACCESS::prune(vm);
			combine_pixels();
			didSomething = true;
		}

		if (_operation == Operation::AddPixelLayer)
		{
			bool createNew = true;
			if (auto* vl = fast_cast<ISpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
			{
				if (vl->is_temporary())
				{
					vl->be_temporary(false);
					createNew = false;
				}
			}
			if (createNew)
			{
				CONTENT_ACCESS::add_layer(vm, new Framework::SpriteLayer(), 1);
			}
			didSomething = true;
			combine_pixels();
			populate_layer_list();
		}
		if (_operation == Operation::AddModLayer)
		{
			CONTENT_ACCESS::change_depth_of_edited_layer_and_its_children(vm, 1);
			auto* l = new Framework::SpriteTransformLayer();
			CONTENT_ACCESS::add_layer(vm, l, 0);
			l->set_depth(l->get_depth() - 1); // as we changed depth of edited and placed where edited was, we need to reverse that small change now
			didSomething = true;
			combine_pixels();
			populate_layer_list();
		}
		if (_operation == Operation::AddFullModLayer)
		{
			CONTENT_ACCESS::change_depth_of_edited_layer_and_its_children(vm, 1);
			auto* l = new Framework::SpriteFullTransformLayer();
			CONTENT_ACCESS::add_layer(vm, l, 0);
			l->set_depth(l->get_depth() - 1); // as we changed depth of edited and placed where edited was, we need to reverse that small change now
			didSomething = true;
			combine_pixels();
			populate_layer_list();
		}
		if (_operation == Operation::AddIncludeLayer)
		{
			UI::Utils::choose_library_stored<Framework::Sprite>(get_canvas(), NAME(chooseLibraryStored_includeSprite), TXT("include sprite"), lastSpriteIncluded.get(), 1.0f,
				[this, vm](Framework::Sprite* _chosen)
				{
					if (CONTENT_ACCESS::can_include(vm, _chosen))
					{
						auto* l = new Framework::IncludeSpriteLayer();
						l->set_included(_chosen);
						lastSpriteIncluded = _chosen;
						l->set_id(_chosen->get_name().get_name());
						CONTENT_ACCESS::add_layer(vm, l, 1);
						combine_pixels();
						populate_layer_list();
						update_ui_highlight();
					}
					else
					{
						UI::Utils::QuestionWindow::show_message(get_canvas(), String::printf(TXT("can't include sprite \"%S\""), _chosen->get_name().to_string().to_char()).to_char());
					}
				});
			didSomething = true;
		}
		if (_operation == Operation::DuplicateLayer)
		{
			CONTENT_ACCESS::duplicate_edited_layer(vm);
			didSomething = true;
			populate_layer_list();
		}
		if (_operation == Operation::SwitchTemporariness)
		{
			if (auto* vl = fast_cast<ISpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
			{
				vl->be_temporary(!vl->is_temporary());
			}
			didSomething = true;
			populate_layer_list();
		}
		if (_operation == Operation::RemoveLayer)
		{
			CONTENT_ACCESS::remove_edited_layer(vm);
			combine_pixels();
			didSomething = true;
			populate_layer_list();
		}
		if (_operation == Operation::ChangedLayerProps ||
			_operation == Operation::ChangedSpriteProps)
		{
			combine_pixels();
			didSomething = true;
			update_ui_highlight();
		}
		if (_operation == Operation::RenamedLayer ||
			_operation == Operation::ChangedLayerVisibility ||
			_operation == Operation::MovedLayer ||
			_operation == Operation::ChangedDepthOfLayer)
		{
			didSomething = true;
			populate_layer_list();
		}
		if (_operation == Operation::MergeWithNext)
		{
			String resultInfo;
			didSomething = CONTENT_ACCESS::merge_with_next(vm, OUT_ resultInfo); // same level if no between
			if (!resultInfo.is_empty())
			{
				UI::Utils::QuestionWindow::show_message(get_canvas(), resultInfo.to_char());
			}
			CONTENT_ACCESS::prune(vm);
			combine_pixels();
			populate_layer_list();
		}
		if (_operation == Operation::MergeWithChildren)
		{
			String resultInfo;
			didSomething = CONTENT_ACCESS::merge_with_children(vm, OUT_ resultInfo); // same level if no between
			if (!resultInfo.is_empty())
			{
				UI::Utils::QuestionWindow::show_message(get_canvas(), resultInfo.to_char());
			}
			CONTENT_ACCESS::prune(vm);
			combine_pixels();
			populate_layer_list();
		}
	}

	if (didSomething)
	{
		if (editorContext)
		{
			if (_operation != Operation::GetColourFromPixel)
			{
				if (editorContext->mark_save_required() ||
					_operation == Operation::ChangedSpriteProps)
				{
					update_ui_highlight();
				}
			}
		}

		cursorRays.clear();
	}
}

Range2 Editor::Sprite::get_grid_range() const
{
	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		RangeInt2 r = CONTENT_ACCESS::get_combined_range(vm);
		if (r.is_empty())
		{
			r = RangeInt2::zero;
		}
		r.expand_by(VectorInt2::one * 8);

		return vm->to_full_range(r);
	}

	return base::get_grid_range();
}

Vector2 Editor::Sprite::get_grid_step() const
{
	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		return vm->get_pixel_size();
	}

	return base::get_grid_step();
}

Vector2 Editor::Sprite::get_grid_zero_at() const
{
	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		return -vm->get_zero_offset() * vm->get_pixel_size();
	}

	return base::get_grid_zero_at();
}

float Editor::Sprite::get_grid_alpha(float _v, float _step) const
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

void Editor::Sprite::render_debug(CustomRenderContext* _customRC)
{
	debug_gather_in_renderer();

	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		bool shiftPressed = false;
#ifdef AN_STANDARD_INPUT
		if (auto* input = ::System::Input::get())
		{
			shiftPressed = input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift);
		}
#endif

		for_every(cr, cursorRays)
		{
			bool mayAdd = false;
			bool mayRemove = false;
			bool mayColour = false;
			bool maySelect = false;
			if (cr->cellCoord.is_set() &&
				(cr->hitPixel))
			{
				mayRemove = mode == Mode::Remove || mode == Mode::AddRemove || (mode == Mode::Fill);
				mayColour = mode == Mode::Paint || mode == Mode::PaintFill || mode == Mode::ReplaceColour || mode == Mode::GetColour;
			}
			if (cr->cellCoord.is_set())
			{
				maySelect = mode == Mode::Select;
			}
			if (cr->cellCoord.is_set())
			{
				mayAdd = mode == Mode::Add || mode == Mode::AddRemove || (mode == Mode::Fill && ! shiftPressed);
			}

			if (mayRemove || mayColour || maySelect)
			{
				CONTENT_ACCESS::debug_draw_pixel(vm, cr->cellCoord.get(), Colour::lerp(::System::Core::get_timed_pulse(1.0f) * 0.2f, mayAdd? Colour::black : Colour::white, Colour::red), true);
			}
			if (mayAdd)
			{
				CONTENT_ACCESS::debug_draw_pixel(vm, cr->cellCoord.get(), Colour::lerp(::System::Core::get_timed_pulse(1.0f) * 0.2f, Colour::white, Colour::red), true);
			}
		}
		if (! selection.range.is_empty())
		{
			Vector2 cmin = selection.range.bottom_left();
			Vector2 cmax = selection.range.top_right();
			Vector2 wider = vm->get_pixel_size() * 0.01f;
			cmin -= wider;
			cmax += wider;
			CONTENT_ACCESS::debug_draw_box_region(vm, cmin, cmax, SELECTION_COLOUR, true, 0.2f);
			{
				RangeInt2 pixelRange = vm->to_pixel_range(selection.range, SELECTION_FULLY_INLCUDED);
				Range2 pixeledRange = vm->to_full_range(pixelRange);
				if (!pixeledRange.is_empty())
				{
					CONTENT_ACCESS::debug_draw_box_region(vm, pixeledRange.bottom_left(), pixeledRange.top_right(), Colour::lerp(0.5f, SELECTION_COLOUR, Colour::red), true);
				}
			}
		}
	}

	debug_gather_restore();

	base::render_debug(_customRC);
}

void Editor::Sprite::render_edited_asset()
{
	if (auto* vm = fast_cast<Framework::Sprite>(editedAsset.get()))
	{
		Optional<Plane> highlightPlane;
		Framework::CONTENT_ACCESS::RenderContext context;
		context.highlightPlane = highlightPlane;
		if (highlightEditedLayerFor > 0.0f)
		{
			context.editedLayerColour = HIGHLIGHT_EDITED_LAYER_COLOUR;
		}
		if (drawEdges)
		{
			context.pixelEdgeColour = Colour::black.with_alpha(0.5f);
		}
		context.onlySolidPixels = onlySolidPixels;
		CONTENT_ACCESS::render(vm, context);
	}
	else
	{
		base::render_edited_asset();
	}
}

void Editor::Sprite::setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	base::setup_ui_add_to_bottom_left_panel(c, panel, scale);

	{
		auto* b = Framework::UI::CanvasButton::get();
		b->set_id(NAME(editorSprite_drawEdges));
		b->set_caption(TXT("edges"));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				drawEdges = !drawEdges;
				update_ui_highlight();
			});
		panel->add(b);
	}

	{
		auto* b = Framework::UI::CanvasButton::get();
		b->set_id(NAME(editorSprite_onlySolidPixels));
		b->set_caption(TXT("only solid"));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				onlySolidPixels = !onlySolidPixels;
				update_ui_highlight();
			});
		panel->add(b);
	}
}

void Editor::Sprite::setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	base::setup_ui_add_to_top_name_panel(c, panel, scale);

	{
		auto* b = Framework::UI::CanvasButton::get();
		b->set_caption(TXT("/"));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_font_scale(get_name_font_scale());
		b->set_text_only();
		panel->add(b);
	}
	{
		auto* b = Framework::UI::CanvasButton::get();
		b->set_id(NAME(editorSprite_editedLayerName));
		b->set_caption(TXT("layer"));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_font_scale(get_name_font_scale());
		b->set_text_only();
		panel->add(b);
	}
}

void Editor::Sprite::setup_ui(REF_ SetupUIContext& _setupUIContext)
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
					b->set_id(NAME(editorSprite_addRemove));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::AddRemove;
							colourPaletteComponent.show_palette(true); // has to be open to work
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
					b->set_id(NAME(editorSprite_add));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Add;
							colourPaletteComponent.show_palette(true); // has to be open to work
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
					b->set_id(NAME(editorSprite_remove));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Remove;
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("fill")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N4)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSprite_fill));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Fill;
							colourPaletteComponent.show_palette(true); // has to be open to work
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
					b->set_id(NAME(editorSprite_colourPicker));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::GetColour;
							colourPaletteComponent.show_palette(true); // has to be open to work
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("paint")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::Q)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSprite_paint));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Paint;
							colourPaletteComponent.show_palette(true); // has to be open to work
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("p-fill")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::W)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSprite_paintFill));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::PaintFill;
							colourPaletteComponent.show_palette(true); // has to be open to work
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("replace")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::E)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSprite_replaceColour));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::ReplaceColour;
							colourPaletteComponent.show_palette(true); // has to be open to work
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("select")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::R)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSprite_select));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Select;
							show_layer_props(true); // has to be open to work (shortcuts)
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("move")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::T)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorSprite_move));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							mode = Mode::Move;
							update_ui_highlight();
						});
					menu->add(b);
				}

				//menu->place_content_horizontally(c);
				menu->place_content_on_grid(c, VectorInt2(5, 0));
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
							colourPaletteComponent.show_or_hide_palette();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("layers")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::L, UI::ShortcutParams().with_shift())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							auto* existing = _context.canvas->find_by_id(NAME(editorLayers));
							show_layers(existing == nullptr);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("details")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::D, UI::ShortcutParams().with_shift())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							auto* existing = _context.canvas->find_by_id(NAME(editorLayerProps));
							show_layer_props(existing == nullptr);
						});
					menu->add(b);
				}

				menu->place_content_vertically(c);
			}
			UI::Utils::place_below(c, menu, REF_ below, 1.0f);
		}

		restore_windows(true);
	}

	update_edited_layer_name();
}

void Editor::Sprite::show_layers(bool _show, bool _forcePlaceAt)
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorLayers)));

	if (_show)
	{
		if (!existing)
		{
			Framework::UI::ICanvasElement* nextTo = nullptr;
			if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
			{
				float textScale = 0.7f;
				UI::Utils::ListWindow list;
				auto* window = list.step_1_create_window(c, textScale, NP);
				existing = window;
				window->set_title(TXT("layers"))->set_id(NAME(editorLayers));
				window->set_modal(false)->set_closable(true);
				window->set_on_moved([this](UI::CanvasWindow* _window) { store_windows_at(); });
				list.step_2_setup_list(NAME(editorLayers_layerList));
				list.step_3_add_button(TXT("+pix"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddPixelLayer);
							}
						});
				list.step_3_add_button(TXT("+mod"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddModLayer);
							}
						});
				list.step_3_add_button(TXT("+mod full"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddFullModLayer);
							}
						});
				list.step_3_add_button(TXT("+include"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddIncludeLayer);
							}
						});
				list.step_3_add_button(TXT("copy"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::DuplicateLayer);
							}
						});
				list.step_3_add_button(TXT("del"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								if (CONTENT_ACCESS::access_edited_layer(vm) &&
									CONTENT_ACCESS::access_edited_layer(vm)->is_temporary())
								{
									do_operation(NONE, Operation::RemoveLayer);
								}
								else
								{
									UI::Utils::QuestionWindow::ask_question_yes_no(_context.canvas, TXT("are you sure you want to remove?"),
										[this]()
										{
											do_operation(NONE, Operation::RemoveLayer);
										});
								}
							}
						});
				list.step_3_add_button(TXT("temp"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::SwitchTemporariness);
							}
						});
				list.step_3_add_button(TXT("name"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								if (auto* l = CONTENT_ACCESS::access_edited_layer(vm))
								{
									String id;
									if (l->get_id().is_valid())
									{
										id = l->get_id().to_string();
									}
									UI::Utils::TextEditWindow edit;
									edit.step_1_create_window(_context.canvas);
									edit.step_2_setup(id,
										[this, l](String _text)
										{
											if (_text.is_empty())
											{
												l->set_id(Name::invalid());
											}
											else
											{
												l->set_id(Name(_text));
											}
											do_operation(NONE, Operation::RenamedLayer);
										});
									edit.step_3_finalise();
								}
							}
						});
				list.step_3_add_button(TXT("show"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::S, UI::ShortcutParams().with_shift())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								if (auto* l = CONTENT_ACCESS::access_edited_layer(vm))
								{
									int vl = l->get_is_visible();
									if (l->get_visibility_only() || ! l->is_visible(vm))
									{
										l->set_visible(true);
									}
									else
									{
										l->set_visible(!vl);
									}
									l->set_visibility_only(true, vm); // to clear other
									l->set_visibility_only(false, nullptr);
									combine_pixels();
									do_operation(NONE, Operation::ChangedLayerVisibility);
								}								
							}
						});
				list.step_3_add_button(TXT("only"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::O, UI::ShortcutParams().with_shift())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								if (auto* l = CONTENT_ACCESS::access_edited_layer(vm))
								{
									l->set_visibility_only(! l->get_visibility_only(), vm);
									combine_pixels();
									do_operation(NONE, Operation::ChangedLayerVisibility);
								}								
							}
						});
				list.step_3_add_button(TXT("depth <"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::Home, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::change_depth_of_edited_layer(vm, -1);
								combine_pixels();
								do_operation(NONE, Operation::ChangedDepthOfLayer);
							}
						});
				list.step_3_add_button(TXT("depth >"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::End, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::change_depth_of_edited_layer(vm, 1);
								combine_pixels();
								do_operation(NONE, Operation::ChangedDepthOfLayer);
							}
						});
				list.step_3_add_button(TXT("sel ^"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageUp, UI::ShortcutParams().with_shift())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::set_edited_layer_index(vm, CONTENT_ACCESS::get_edited_layer_index(vm) - 1);
								highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
								combine_pixels();
								populate_layer_list();
							}
						});
				list.step_3_add_button(TXT("sel v"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageDown, UI::ShortcutParams().with_shift())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::set_edited_layer_index(vm, CONTENT_ACCESS::get_edited_layer_index(vm) + 1);
								highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
								combine_pixels();
								populate_layer_list();
							}
						});
				list.step_3_add_button(TXT("mov ^"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageUp, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::move_edited_layer(vm, -1);
								combine_pixels();
								do_operation(NONE, Operation::MovedLayer);
							}
						});
				list.step_3_add_button(TXT("mov v"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageDown, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::move_edited_layer(vm, 1);
								combine_pixels();
								do_operation(NONE, Operation::MovedLayer);
							}
						});
				list.step_3_add_button(TXT("merge 1"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::M, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::MergeWithNext);
							}
						});
				list.step_3_add_button(TXT("merge children"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								do_operation(NONE, Operation::MergeWithChildren);
							}
						});
				Vector2 cs = c->get_logical_size();
				Vector2 size;
				size.y = cs.y * 0.4f;
				size.x = cs.y * 0.6f;
				list.step_4_finalise_force_size(size, VectorInt2(6,0));

				layerListContext.textScale = textScale;
				populate_layer_list();

				if (windows.layers.atValid)
				{
					window->set_at(windows.layers.at);
					window->set_at(windows.layers.at - Vector2(0.0f, window->get_placement(c).y.length()));
					window->fit_into_canvas(c);
				}
				else
				{
					UI::Utils::place_on_left(c, window, REF_ nextTo, 0.5f);
				}
			}
		}
		
		if (_forcePlaceAt && existing)
		{
			if (windows.layers.atValid)
			{
				existing->set_at(windows.layers.at);
				existing->set_at(windows.layers.at - Vector2(0.0f, existing->get_placement(c).y.length()));
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

	windows.layers.open = _show;

	update_ui_highlight();
}

void Editor::Sprite::reshow_layer_props()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorLayerProps))))
	{
		show_layer_props(false);
		show_layer_props(true, true);
	}
}

void Editor::Sprite::show_layer_props(bool _show, bool _forcePlaceAt)
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	auto* existing = fast_cast<UI::CanvasWindow>(c->find_by_id(NAME(editorLayerProps)));

	if (_show)
	{
		if (!existing)
		{
			Framework::UI::ICanvasElement* above = nullptr;
			Framework::UI::ICanvasElement* topRightPanel = nullptr;
			if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
			{
				windows.layerPropsForLayer = CONTENT_ACCESS::access_edited_layer(vm);
				windows.layerPropsForMode = mode;
				auto* window = Framework::UI::CanvasWindow::get();
				existing = window;
				window->set_closable(true);
				window->set_title(TXT("layer details"))->set_id(NAME(editorLayerProps));
				window->set_on_moved([this](UI::CanvasWindow* _window) { store_windows_at(); });
				c->add(window);
				Vector2 cs = c->get_logical_size();
				Vector2 size;
				size.x = cs.y * 0.3f;
				Vector2 leftPanelWidthSize(size.x, 0.0f);
				Framework::UI::ICanvasElement* belowPanel = nullptr;
				if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
				{
					float textScale = 0.7f;
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						{
							auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("(pixel) transform"));
							b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
							b->set_text_only();
							panel->add(b);
						}
						panel->place_content_on_grid(c, VectorInt2(1, 0), leftPanelWidthSize);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						for_count(int, i, 2)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(i == 0 ? TXT("x") : TXT("y")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("++0++")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								b->set_id(i == 0 ? NAME(editorLayerProps_offsetX) : NAME(editorLayerProps_offsetY));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												--vtl->access_offset().access_element(i);
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("+")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												++vtl->access_offset().access_element(i);
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
						}
						panel->place_content_on_grid(c, VectorInt2(4, 0), leftPanelWidthSize);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("yaw")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("++0++")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_id(NAME(editorLayerProps_rotateYaw));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												auto& e = vtl->access_rotate().access_element(1);
												--e;
												while (e > 2) e -= 4;
												while (e < -2) e += 4;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("+")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												auto& e = vtl->access_rotate().access_element(1);
												++e;
												while (e > 2) e -= 4;
												while (e < -2) e += 4;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
						}
						panel->place_content_on_grid(c, VectorInt2(4, 0), leftPanelWidthSize);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						//mirror axis
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_id(NAME(editorLayerProps_mirrorNone));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().mirrorAxis = NONE;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							for_count(int, i, 2)
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(i == 0 ? TXT("x") : TXT("y")));
								b->set_id(i == 0 ? NAME(editorLayerProps_mirrorX) : NAME(editorLayerProps_mirrorY));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().mirrorAxis = i;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
						}
						// mirror at
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("at")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("++0++")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_id(NAME(editorLayerProps_mirrorAt));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												if (vtl->access_mirror().mirrorAxis != NONE)
												{
													remove_transform_layer_gizmos();
													auto& e = vtl->access_mirror().at;
													--e;
													do_operation(NONE, Operation::ChangedLayerProps);
												}
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("+")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												if (vtl->access_mirror().mirrorAxis != NONE)
												{
													remove_transform_layer_gizmos();
													auto& e = vtl->access_mirror().at;
													++e;
													do_operation(NONE, Operation::ChangedLayerProps);
												}
											}
										}
									});
								panel->add(b);
							}
						}
						// mirror is axis
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("is")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("axis")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("no")));
								b->set_id(NAME(editorLayerProps_mirrorAxisNo));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().axis = false;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("yes")));
								b->set_id(NAME(editorLayerProps_mirrorAxisYes));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().axis = true;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
						}
						// mirror in dir
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("dir")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("<-")));
								b->set_id(NAME(editorLayerProps_mirrorDirN));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().dir = -1;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("both")));
								b->set_id(NAME(editorLayerProps_mirrorDirZ));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().dir = 0;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("->")));
								b->set_id(NAME(editorLayerProps_mirrorDirP));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().dir = 1;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
						}

						panel->place_content_on_grid(c, VectorInt2(4, 0), leftPanelWidthSize);
						UI::Utils::place_on_right(c, panel, topRightPanel, 1.0f, c->get_default_spacing().x);
					}
				}
				else if (auto* vtl = fast_cast<Framework::SpriteFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
				{
					float textScale = 0.7f;
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						{
							auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("full transform"));
							b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
							b->set_text_only();
							panel->add(b);
						}
						panel->place_content_on_grid(c, VectorInt2(1, 0), leftPanelWidthSize);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						for_count(int, i, 2)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(i == 0 ? TXT("x") : (i == 1 ? TXT("y") : TXT("z"))));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("++0++")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								b->set_id(i == 0 ? NAME(editorLayerProps_offsetX) : NAME(editorLayerProps_offsetY));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_full_transform_layer_gizmos();
												float& c = vtl->access_offset().access_element(i);
												float r = 1.0f;
#ifdef AN_STANDARD_INPUT
												if (auto* input = ::System::Input::get())
												{
													bool shiftPressed = input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift);
													if (shiftPressed)
													{
														r = 5.0f;
													}
												}
#endif
												if (abs(c - floor_to(c, r)) < 0.001f)
												{
													c = round(c - r);
												}
												else
												{
													c = floor_to(c, r);
												}
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("+")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_full_transform_layer_gizmos();
												float& c = vtl->access_offset().access_element(i);
												float r = 1.0f;
#ifdef AN_STANDARD_INPUT
												if (auto* input = ::System::Input::get())
												{
													bool shiftPressed = input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift);
													if (shiftPressed)
													{
														r = 5.0f;
													}
												}
#endif
												if (abs(c - ceil_to(c, r)) < 0.001f)
												{
													c = round(c + r);
												}
												else
												{
													c = ceil_to(c, r);
												}
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
						}
						panel->place_content_on_grid(c, VectorInt2(4, 0), leftPanelWidthSize);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("yaw")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("++0++")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_id(NAME(editorLayerProps_rotateYaw));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_full_transform_layer_gizmos();
												float& c = vtl->access_rotate().access_element(1);
												float r = 1.0f;
#ifdef AN_STANDARD_INPUT
												if (auto* input = ::System::Input::get())
												{
													bool shiftPressed = input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift);
													if (shiftPressed)
													{
														r = 5.0f;
													}
												}
#endif
												if (abs(c - floor_to(c, r)) < 0.001f)
												{
													c = round(c - r);
												}
												else
												{
													c = floor_to(c, r);
												}
												c = Rotator3::normalise_axis(c);
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("+")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::SpriteFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_full_transform_layer_gizmos();
												float& c = vtl->access_rotate().access_element(1);
												float r = 1.0f;
#ifdef AN_STANDARD_INPUT
												if (auto* input = ::System::Input::get())
												{
													bool shiftPressed = input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift);
													if (shiftPressed)
													{
														r = 5.0f;
													}
												}
#endif
												if (abs(c - ceil_to(c, r)) < 0.001f)
												{
													c = round(c + r);
												}
												else
												{
													c = ceil_to(c, r);
												}
												c = Rotator3::normalise_axis(c);
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
						}
						panel->place_content_on_grid(c, VectorInt2(4, 0), leftPanelWidthSize);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
				}
				else if (auto* ivml = fast_cast<Framework::IncludeSpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
				{
					float textScale = 1.0f;
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("included:"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("included sprite"));
								b->set_id(NAME(editorLayerProps_includedSprite));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("change included"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										UI::Utils::choose_library_stored<Framework::Sprite>(get_canvas(), NAME(chooseLibraryStored_includeSprite), TXT("include sprite"), lastSpriteIncluded.get(), 1.0f,
											[this](Framework::Sprite* _chosen)
											{
												if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
												{
													if (CONTENT_ACCESS::can_include(vm, _chosen))
													{
														if (auto* ivml = fast_cast<Framework::IncludeSpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
														{
															ivml->set_included(_chosen);
														}
														lastSpriteIncluded = _chosen;
														combine_pixels();
														populate_layer_list();
														update_ui_highlight();
													}
													else
													{
														UI::Utils::QuestionWindow::show_message(get_canvas(), String::printf(TXT("can't include sprite \"%S\""), _chosen->get_name().to_string().to_char()).to_char());
													}
												}
											});
									});
								panel->add(b);
							}
						}
						panel->place_content_vertically(c);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
				}
				else
				{
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						float textScale = 0.7f;
						if (auto* vml = fast_cast<Framework::SpriteLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
						{
							auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("sprite layer"));
							b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
							b->set_text_only();
							panel->add(b);
						}
						if (mode == Mode::Select)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("on selection:"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("fill"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
#ifdef AN_STANDARD_INPUT
								b->set_shortcut(System::Key::F, UI::ShortcutParams().with_alt());
#endif
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										do_operation(NONE, Operation::FillSelection);
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("paint"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
#ifdef AN_STANDARD_INPUT
								b->set_shortcut(System::Key::P, UI::ShortcutParams().with_alt());
#endif
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										do_operation(NONE, Operation::PaintSelection);
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("remove/clear"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
#ifdef AN_STANDARD_INPUT
								b->set_shortcut(System::Key::R, UI::ShortcutParams().with_alt());
#endif
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										do_operation(NONE, Operation::ClearSelection);
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("duplicate to new layer"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
#ifdef AN_STANDARD_INPUT
								b->set_shortcut(System::Key::D, UI::ShortcutParams().with_alt());
#endif
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										do_operation(NONE, Operation::DuplicateSelectionToNewLayer);
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("move to new layer"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
#ifdef AN_STANDARD_INPUT
								b->set_shortcut(System::Key::M, UI::ShortcutParams().with_alt());
#endif
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										do_operation(NONE, Operation::MoveSelectionToNewLayer);
									});
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("+shift to copy"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
						}
						else if (mode == Mode::Move)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("+shift to copy"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
						}
						else if (mode == Mode::Fill)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("+shift to fill in pixel"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("right-click to remove-fill"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
						}
						else if (mode == Mode::GetColour)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("+a/s/d/f to store colour shortcut"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
						}

						if (mode != Mode::Move)
						{
							float textScale = 0.5f;
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("+ctrl to ignore invisible/force empty pixels"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
						}

						panel->place_content_vertically(c);
						UI::Utils::place_below(c, panel, belowPanel, 0.0f, c->get_default_spacing().y);
						if (!topRightPanel) topRightPanel = panel;
					}
				}

				window->set_size_to_fit_all(c, c->get_default_spacing());

				if (windows.layerProps.atValid)
				{
					window->set_at(windows.layerProps.at);
					window->set_at(windows.layerProps.at - Vector2(0.0f, window->get_placement(c).y.length()));
					window->fit_into_canvas(c);
				}
				else
				{
					UI::Utils::place_above(c, window, REF_ above, 0.5f);
				}
			}
		}
		
		if (_forcePlaceAt && existing)
		{
			if (windows.layerProps.atValid)
			{
				existing->set_at(windows.layerProps.at);
				existing->set_at(windows.layerProps.at - Vector2(0.0f, existing->get_placement(c).y.length()));
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

	windows.layerProps.open = _show;

	update_ui_highlight();
}

void Editor::Sprite::populate_layer_list()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	float textScale = layerListContext.textScale;

	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		int layerCount = CONTENT_ACCESS::get_layers(vm).get_size();
		layerListContext.selectedLayerIndex = clamp(CONTENT_ACCESS::get_edited_layer_index(vm), 0, layerCount - 1);
		UI::Utils::ListWindow::populate_list(canvas, NAME(editorLayers_layerList), layerCount, layerListContext.selectedLayerIndex, textScale,
			[this](int idx, UI::CanvasButton* b)
			{
				if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
				{
					if (CONTENT_ACCESS::get_layers(vm).is_index_valid(idx))
					{
						if (auto* l = CONTENT_ACCESS::get_layers(vm)[idx].get())
						{
							int depth = l->get_depth();
							Name id = l->get_id();
							String name;
							name.access_data().make_space_for(depth + 1 + id.to_string().get_length());
							for_count(int, i, depth)
							{
								name += String::space();
							}
							if (l->is_temporary())
							{
								b->set_colour(Colour::orange);
								name += TXT("temporary");
							}
							else
							{
								b->set_colour(NP);
								bool visible = l->is_visible(vm);
								bool visibleOnly = l->get_visibility_only();
								if (!visible)
								{
									name += '(';
								}
								else if (visibleOnly)
								{
									name += TXT("> ");
								}
								if (id.is_valid())
								{
									name += id.to_string();
								}
								else
								{
									name += String::printf(TXT("layer %i"), idx + 1);
								}
								if (!visible)
								{
									name += ')';
								}
								else if (visibleOnly)
								{
									name += TXT(" <");
								}

								if (auto* ivm = fast_cast<IncludeSpriteLayer>(l))
								{
									name += TXT(" [included]");
								}
								if (auto* ivm = fast_cast<SpriteTransformLayer>(l))
								{
									name += TXT(" [mod]");
								}
								if (auto* ivm = fast_cast<SpriteFullTransformLayer>(l))
								{
									name += TXT(" [mod-f]");
								}
							}
							b->set_caption(name);
							return;
						}
					}
				}

				b->set_caption(TXT("??"));
			},
			[this](int _idx, void const * _userData)
			{
				layerListContext.selectedLayerIndex = _idx;
				if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
				{
					CONTENT_ACCESS::merge_temporary_layers(vm);
					CONTENT_ACCESS::set_edited_layer_index(vm, _idx);
					highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
					combine_pixels();
					populate_layer_list();
				}
			},
			[this](int _idx, void const* _userData)
			{
				layerListContext.selectedLayerIndex = _idx;
				if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
				{
					CONTENT_ACCESS::merge_temporary_layers(vm);
					CONTENT_ACCESS::set_edited_layer_index(vm, _idx);
					highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
					combine_pixels();
					populate_layer_list();
				}
			});
	}
	else
	{
		UI::Utils::ListWindow::populate_list(canvas, NAME(editorLayers_layerList), 0, NP, NP, nullptr);
	}

	update_edited_layer_name();
	update_ui_highlight();
}

void Editor::Sprite::update_edited_layer_name()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_editedLayerName))))
	{
		String layerName;
		if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
		{
			if (auto* l = CONTENT_ACCESS::access_edited_layer(vm))
			{
				if (l->get_id().is_valid())
				{
					layerName = l->get_id().to_string();
				}
				else
				{
					layerName = String::printf(TXT("layer %i"), CONTENT_ACCESS::get_edited_layer_index(vm) + 1);
				}
			}
		}
		b->set_caption(layerName);
		b->set_auto_width(canvas);
		b->set_default_height(canvas);
	}
	if (auto* w = fast_cast<Framework::UI::CanvasWindow>(c->find_by_id(NAME(editorSprite_editedLayerNameWindow))))
	{
		Range2 wRange = w->get_placement(canvas);
		w->place_content_horizontally(canvas);
		Range2 wNewRange = w->get_placement(canvas);
		w->offset_placement_by(wRange.get_at(Vector2(0.0f, 1.0f)) - wNewRange.get_at(Vector2(0.0f, 1.0f)));
	}
}

void Editor::Sprite::update_ui_highlight()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	colourPaletteComponent.update_ui_highlight();

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_addRemove))))
	{
		b->set_highlighted(mode == Mode::AddRemove);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_add))))
	{
		b->set_highlighted(mode == Mode::Add);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_remove))))
	{
		b->set_highlighted(mode == Mode::Remove);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_fill))))
	{
		b->set_highlighted(mode == Mode::Fill);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_paint))))
	{
		b->set_highlighted(mode == Mode::Paint);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_paintFill))))
	{
		b->set_highlighted(mode == Mode::PaintFill);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_replaceColour))))
	{
		b->set_highlighted(mode == Mode::ReplaceColour);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_colourPicker))))
	{
		b->set_highlighted(mode == Mode::GetColour);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_select))))
	{
		b->set_highlighted(mode == Mode::Select);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_move))))
	{
		b->set_highlighted(mode == Mode::Move);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_drawEdges))))
	{
		b->set_highlighted(drawEdges);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_onlySolidPixels))))
	{
		b->set_highlighted(onlySolidPixels);
	}
	Framework::SpriteTransformLayer* vtl = nullptr;
	Framework::SpriteFullTransformLayer* vftl = nullptr;
	Framework::IncludeSpriteLayer* ivml = nullptr;
	auto* vm = fast_cast<Framework::Sprite>(get_edited_asset());
	if (vm)
	{
		if (auto* l = CONTENT_ACCESS::access_edited_layer(vm))
		{
			vtl = fast_cast<Framework::SpriteTransformLayer>(l);
			vftl = fast_cast<Framework::SpriteFullTransformLayer>(l);
			ivml = fast_cast<Framework::IncludeSpriteLayer>(l);
		}
	}

	// SpriteTransformLayer / SpriteFullTransformLayer
	{
		for_count(int, i, 2)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(i == 0 ? NAME(editorLayerProps_offsetX) : NAME(editorLayerProps_offsetY))))
			{
				if (vtl)
				{
					int e = vtl->access_offset().access_element(i);
					b->set_caption(String::printf(TXT("%c%i"), e == 0 ? ' ' : (e < 0 ? '-' : '+'), abs(e)));
				}
				else if (vftl)
				{
					float e = vftl->access_offset().access_element(i);
					b->set_caption(String::printf(TXT("%c%.2f"), e == 0.0f ? ' ' : (e < 0.0f ? '-' : '+'), abs(e)));
				}
				else
				{
					b->set_caption(TXT("-"));
				}
			}
		}
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_rotateYaw))))
			{
				if (vtl)
				{
					int e = vtl->access_rotate().access_element(1);
					b->set_caption(String::printf(TXT("%c%i"), e == 0 ? ' ' : (e < 0 ? '-' : '+'), abs(e)));
				}
				else if (vftl)
				{
					float e = vftl->access_rotate().access_element(1);
					b->set_caption(String::printf(TXT("%c%.2f"), e == 0.0f ? ' ' : (e < 0.0f ? '-' : '+'), abs(e)));
				}
				else
				{
					b->set_caption(TXT("-"));
				}
			}
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorNone))))
		{
			b->set_highlighted(vtl && vtl->access_mirror().mirrorAxis == NONE);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorX))))
		{
			b->set_highlighted(vtl && vtl->access_mirror().mirrorAxis == 0);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorY))))
		{
			b->set_highlighted(vtl && vtl->access_mirror().mirrorAxis == 1);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorAt))))
		{
			if (vtl && vtl->access_mirror().mirrorAxis != NONE)
			{
				int e = vtl->access_mirror().at;
				b->set_caption(String::printf(TXT("%c%i"), e == 0 ? ' ' : (e < 0 ? '-' : '+'), abs(e)));
			}
			else
			{
				b->set_caption(TXT("-"));
			}
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorAxisNo))))
		{
			b->set_highlighted(vtl&& vtl->access_mirror().mirrorAxis != NONE && ! vtl->access_mirror().axis);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorAxisYes))))
		{
			b->set_highlighted(vtl&& vtl->access_mirror().mirrorAxis != NONE && vtl->access_mirror().axis);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorDirN))))
		{
			b->set_highlighted(vtl && vtl->access_mirror().mirrorAxis != NONE && vtl->access_mirror().dir < 0);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorDirZ))))
		{
			b->set_highlighted(vtl && vtl->access_mirror().mirrorAxis != NONE && vtl->access_mirror().dir == 0);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorDirP))))
		{
			b->set_highlighted(vtl && vtl->access_mirror().mirrorAxis != NONE && vtl->access_mirror().dir > 0);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_includedSprite))))
		{
			b->set_caption(ivml&& ivml->get_included() ? ivml->get_included()->get_name().to_string().to_char() : TXT("--"));
		}
	}

	{
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSprite_props_spriteGridDrawPriority))))
		{
			b->set_caption(vm ? String::printf(TXT("%i"), vm->get_sprite_grid_draw_priority()).to_char() : TXT("--"));
		}
		for_count(int, i, 2)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(i == 0? NAME(editorSprite_props_pixelSizeX) : NAME(editorSprite_props_pixelSizeY))))
			{
				b->set_caption(vm ? String::printf(TXT("%.3f"), vm->get_pixel_size().get_element(i)).to_char() : TXT("--"));
			}
		}
		for_count(int, i, 2)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(i == 0 ? NAME(editorSprite_props_zeroOffsetX) : NAME(editorSprite_props_zeroOffsetY))))
			{
				b->set_caption(vm ? (vm->get_zero_offset().get_element(i) < 0.25f ? TXT("no") : TXT("centred")) : TXT("--"));
			}
		}
	}

	base::update_ui_highlight();
}

void Editor::Sprite::show_asset_props()
{
	auto* c = get_canvas();

	if (auto* e = c->find_by_id(NAME(editorSprite_props)))
	{
		e->remove();
		return;
	}

	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		UI::Utils::Grid2Menu menu;

		auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("sprite props"));
		window->set_id(NAME(editorSprite_props));
		window->set_modal(false)->set_closable(true);
		add_common_props_to_asset_props(menu);
		colourPaletteComponent.add_to_asset_props(menu);
		menu.step_2_add_text_and_button(TXT("sprite grid draw priority"), TXT("--"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::G)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
					{
						UI::Utils::edit_text(_context.canvas, TXT("sprtie grid draw priority"), String::printf(TXT("%i"), vm->get_sprite_grid_draw_priority()), [this, vm](String const& _text)
							{
								int v = ParserUtils::parse_int(_text);
								CONTENT_ACCESS::set_sprite_grid_draw_priority(vm, v);
								do_operation(NONE, Operation::ChangedSpriteProps);
							});
					}
				})
			->set_id(NAME(editorSprite_props_spriteGridDrawPriority));
		for_count(int, i, 2)
		{
			menu.step_2_add_text_and_button(String::printf(TXT("pixel size %c"), i == 0? 'x' : 'y').to_char(), TXT("--"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(i == 0 ? System::Key::S : System::Key::D)
#endif
				->set_on_press([this, i](Framework::UI::ActContext const& _context)
					{
						if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
						{
							UI::Utils::edit_text(_context.canvas, TXT("pixel size"), String::printf(TXT("%.3f"), vm->get_pixel_size().get_element(i)), [this, i, vm](String const& _text)
								{
									float v = ParserUtils::parse_float(_text);
									if (v > 0.0f)
									{
										Vector2 ps = vm->get_pixel_size();
										ps.access_element(i) = v;
										CONTENT_ACCESS::set_pixel_size(vm, ps);
										do_operation(NONE, Operation::ChangedSpriteProps);
									}
								});
						}
					})
				->set_id(i == 0? NAME(editorSprite_props_pixelSizeX) : NAME(editorSprite_props_pixelSizeY));
		}
		for_count(int, i, 2)
		{
			menu.step_2_add_text_and_button(String::printf(TXT("zero offset %c"), i == 0? 'x' : 'y').to_char(), TXT("--"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(i == 0? System::Key::X : System::Key::Y)
#endif
				->set_on_press([this, i](Framework::UI::ActContext const& _context)
					{
						if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
						{
							Vector2 zeroOffset = vm->get_zero_offset();
							float n = zeroOffset.get_element(i) < 0.25f ? 0.5f : 0.0f;
							zeroOffset.access_element(i) = n;
							CONTENT_ACCESS::set_zero_offset(vm, zeroOffset);
							do_operation(NONE, Operation::ChangedSpriteProps);
						}
					})
				->set_id(i == 0? NAME(editorSprite_props_zeroOffsetX) : NAME(editorSprite_props_zeroOffsetY));
		}
		menu.step_3_add_button(TXT("rescale"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::R)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					show_rescale_selection();
				});
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

void Editor::Sprite::show_rescale_selection()
{
	auto* c = get_canvas();

	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		UI::Utils::Grid2Menu menu;

		auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("rescale"));
		window->set_closable(true);
		menu.step_2_add_text_and_button(TXT("pixel size"), String::printf(TXT("%.3f"), vm->get_pixel_size()).to_char())
			->set_text_only();
		{
			Array<int> rescale;
			rescale.push_back(5);
			rescale.push_back(4);
			rescale.push_back(3);
			rescale.push_back(2);
			for_every(r, rescale)
			{
				int s = *r;
				menu.step_2_add_text_and_button(TXT("more pixels"), String::printf(TXT("x%i"), s).to_char())
					->set_on_press([this, window, s](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::rescale_more_pixels(vm, s);
								combine_pixels();
								update_ui_highlight();
								window->remove();
							}
						});
			}
		}
		{
			Array<int> rescale;
			rescale.push_back(2);
			rescale.push_back(3);
			rescale.push_back(4);
			for_every(r, rescale)
			{
				int s = *r;
				menu.step_2_add_text_and_button(TXT("less pixels"), String::printf(TXT("x%i"), s).to_char())
					->set_on_press([this, window, s](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
							{
								CONTENT_ACCESS::rescale_less_pixels(vm, s);
								combine_pixels();
								update_ui_highlight();
								window->remove();
							}
						});
			}
		}
		menu.step_3_add_button(TXT("back"))
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

void Editor::Sprite::combine_pixels()
{
	if (auto* vm = fast_cast<Framework::Sprite>(get_edited_asset()))
	{
		CONTENT_ACCESS::combine_pixels(vm);
	}
}

void Editor::Sprite::remove_transform_layer_gizmos()
{
	transformLayerGizmos = TransformLayerGizmos(); // will clear all
}

void Editor::Sprite::remove_full_transform_layer_gizmos()
{
	fullTransformLayerGizmos = FullTransformLayerGizmos(); // will clear all
}

void Editor::Sprite::remove_pixel_layer_gizmos()
{
	pixelLayerGizmos = PixelLayerGizmos(); // will clear all
}

int Editor::Sprite::get_selected_colour_index() const
{
	return colourPaletteComponent.get_selected_colour_index();
}

void Editor::Sprite::set_selected_colour_index(int _idx)
{
	colourPaletteComponent.set_selected_colour_index(_idx);
}
