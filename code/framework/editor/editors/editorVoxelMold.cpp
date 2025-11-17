#include "editorVoxelMold.h"

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
#include "..\..\voxel\voxelMold.h"

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

#define CONTENT_ACCESS VoxelMoldContentAccess

//

using namespace Framework;

//

// settings
DEFINE_STATIC_NAME(editorVoxelMold_mode);
DEFINE_STATIC_NAME(editorVoxelMold_limitToPlaneAxis);
DEFINE_STATIC_NAME(editorVoxelMold_limitToPlaneOrigin);
DEFINE_STATIC_NAME(editorVoxelMold_drawEdges);
DEFINE_STATIC_NAME(editorVoxelMold_onlySolidVoxels);
DEFINE_STATIC_NAME(editorLayers_open);
DEFINE_STATIC_NAME(editorLayers_at);
DEFINE_STATIC_NAME(editorLayers_atValid);
DEFINE_STATIC_NAME(editorLayerProps_open);
DEFINE_STATIC_NAME(editorLayerProps_at);
DEFINE_STATIC_NAME(editorLayerProps_atValid);

// ui
DEFINE_STATIC_NAME(editorVoxelMold_props);
DEFINE_STATIC_NAME(editorVoxelMold_props_voxelSize);
DEFINE_STATIC_NAME(editorVoxelMold_props_zeroOffsetX);
DEFINE_STATIC_NAME(editorVoxelMold_props_zeroOffsetY);
DEFINE_STATIC_NAME(editorVoxelMold_props_zeroOffsetZ);
DEFINE_STATIC_NAME(editorVoxelMold_limitToPlane_XY);
DEFINE_STATIC_NAME(editorVoxelMold_limitToPlane_YZ);
DEFINE_STATIC_NAME(editorVoxelMold_limitToPlane_XZ);
DEFINE_STATIC_NAME(editorVoxelMold_addRemove);
DEFINE_STATIC_NAME(editorVoxelMold_add);
DEFINE_STATIC_NAME(editorVoxelMold_remove);
DEFINE_STATIC_NAME(editorVoxelMold_fill);
DEFINE_STATIC_NAME(editorVoxelMold_paint);
DEFINE_STATIC_NAME(editorVoxelMold_paintFill);
DEFINE_STATIC_NAME(editorVoxelMold_replaceColour);
DEFINE_STATIC_NAME(editorVoxelMold_colourPicker);
DEFINE_STATIC_NAME(editorVoxelMold_select);
DEFINE_STATIC_NAME(editorVoxelMold_move);

DEFINE_STATIC_NAME(editorVoxelMold_editedLayerName);
DEFINE_STATIC_NAME(editorVoxelMold_editedLayerNameWindow);

DEFINE_STATIC_NAME(editorLayers);
DEFINE_STATIC_NAME(editorLayers_layerList);

DEFINE_STATIC_NAME(editorLayerProps);
DEFINE_STATIC_NAME(editorLayerProps_offsetX);
DEFINE_STATIC_NAME(editorLayerProps_offsetY);
DEFINE_STATIC_NAME(editorLayerProps_offsetZ);
DEFINE_STATIC_NAME(editorLayerProps_rotatePitch);
DEFINE_STATIC_NAME(editorLayerProps_rotateYaw);
DEFINE_STATIC_NAME(editorLayerProps_rotateRoll);
DEFINE_STATIC_NAME(editorLayerProps_mirrorNone);
DEFINE_STATIC_NAME(editorLayerProps_mirrorX);
DEFINE_STATIC_NAME(editorLayerProps_mirrorY);
DEFINE_STATIC_NAME(editorLayerProps_mirrorZ);
DEFINE_STATIC_NAME(editorLayerProps_mirrorAt);
DEFINE_STATIC_NAME(editorLayerProps_mirrorAxisYes);
DEFINE_STATIC_NAME(editorLayerProps_mirrorAxisNo);
DEFINE_STATIC_NAME(editorLayerProps_mirrorDirN);
DEFINE_STATIC_NAME(editorLayerProps_mirrorDirZ);
DEFINE_STATIC_NAME(editorLayerProps_mirrorDirP);
DEFINE_STATIC_NAME(editorLayerProps_includedVoxelMold);

DEFINE_STATIC_NAME(chooseLibraryStored_includeVoxelMold);

//--

REGISTER_FOR_FAST_CAST(Editor::VoxelMold);

Editor::VoxelMold::VoxelMold()
: colourPaletteComponent(this)
{
}

Editor::VoxelMold::~VoxelMold()
{
}

#define STORE_RESTORE(_op) \
	_op(_setup, mode, NAME(editorVoxelMold_mode)); \
	_op(_setup, limitToPlaneAxis, NAME(editorVoxelMold_limitToPlaneAxis)); \
	_op(_setup, limitToPlaneOrigin, NAME(editorVoxelMold_limitToPlaneOrigin)); \
	_op(_setup, drawEdges, NAME(editorVoxelMold_drawEdges)); \
	_op(_setup, onlySolidVoxels, NAME(editorVoxelMold_onlySolidVoxels)); \
	_op(_setup, windows.layers.open, NAME(editorLayers_open)); \
	_op(_setup, windows.layers.at, NAME(editorLayers_at)); \
	_op(_setup, windows.layers.atValid, NAME(editorLayers_atValid)); \
	_op(_setup, windows.layerProps.open, NAME(editorLayerProps_open)); \
	_op(_setup, windows.layerProps.at, NAME(editorLayerProps_at)); \
	_op(_setup, windows.layerProps.atValid, NAME(editorLayerProps_atValid)); \
	;

void Editor::VoxelMold::edit(LibraryStored* _asset, String const& _filePathToSave)
{
	base::edit(_asset, _filePathToSave);

	update_edited_layer_name();

	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
	{
		if (CONTENT_ACCESS::get_layers(vm).is_empty())
		{
			auto* l = new Framework::VoxelMoldLayer();
			CONTENT_ACCESS::add_layer(vm, l, 0);
			populate_layer_list();
		}
	}
}


void Editor::VoxelMold::restore_for_use(SimpleVariableStorage const& _setup)
{
	base::restore_for_use(_setup);
	STORE_RESTORE(read);
	colourPaletteComponent.restore_for_use(_setup);
	update_ui_highlight();
	restore_windows(true);
}

void Editor::VoxelMold::store_for_later(SimpleVariableStorage& _setup) const
{
	base::store_for_later(_setup);
	STORE_RESTORE(write);
	colourPaletteComponent.store_for_later(_setup);
}

void Editor::VoxelMold::restore_windows(bool _forcePlaceAt)
{
	colourPaletteComponent.restore_windows(_forcePlaceAt);
	if (auto* c = get_canvas())
	{
		show_layers(windows.layers.open, _forcePlaceAt);
		show_layer_props(windows.layerProps.open, _forcePlaceAt);
	}
}
void Editor::VoxelMold::store_windows_at()
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

void Editor::VoxelMold::on_hover(int _cursorIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
{
	update_cursor_ray(_cursorIdx, _rayStart, _rayDir);
}

void Editor::VoxelMold::on_press(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
{
	update_cursor_ray(_cursorIdx, _rayStart, _rayDir);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left)
	{
		if (mode == Mode::AddRemove)
		{
			do_operation(_cursorIdx, Operation::AddVoxel);
			return;
		}
		if (mode == Mode::Fill)
		{
			do_operation(_cursorIdx, Operation::FillAtVoxel);
			return;
		}
	}

	base::on_press(_cursorIdx, _buttonIdx, _rayStart, _rayDir);
}

void Editor::VoxelMold::on_click(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
{
	update_cursor_ray(_cursorIdx, _rayStart, _rayDir);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Right)
	{
		if (mode == Mode::AddRemove)
		{
			do_operation(_cursorIdx, Operation::RemoveVoxel);
			return;
		}
		if (mode == Mode::Fill)
		{
			do_operation(_cursorIdx, Operation::FillRemoveAtVoxel);
			return;
		}
	}

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left)
	{
		if (mode == Mode::Select)
		{
			if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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

	base::on_click(_cursorIdx, _buttonIdx, _rayStart, _rayDir);
}

bool Editor::VoxelMold::on_hold_held(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
{
	update_cursor_ray(_cursorIdx, _rayStart, _rayDir);

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left &&
		(mode == Mode::Add || mode == Mode::Remove || mode == Mode::Paint || mode == Mode::PaintFill || mode == Mode::ReplaceColour || mode == Mode::GetColour || mode == Mode::Select))
	{
		if (cursorRays.is_index_valid(_cursorIdx))
		{
			auto& cr = cursorRays[_cursorIdx];

			if (mode == Mode::Add)
			{
				do_operation(_cursorIdx, Operation::AddVoxel, true);
			}
			if (mode == Mode::Remove)
			{
				do_operation(_cursorIdx, Operation::RemoveVoxel, true);
			}
			if (mode == Mode::Paint)
			{
				do_operation(_cursorIdx, Operation::PaintVoxel, true);
			}
			if (mode == Mode::PaintFill)
			{
				do_operation(_cursorIdx, Operation::PaintFillVoxel, true);
			}
			if (mode == Mode::ReplaceColour)
			{
				do_operation(_cursorIdx, Operation::ReplaceColour, true);
			}
			if (mode == Mode::GetColour)
			{
				do_operation(_cursorIdx, Operation::GetColourFromVoxel, true);
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
				selection.range = Range3::empty;
				if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
				{
					if (CONTENT_ACCESS::merge_temporary_layers(vm))
					{
						populate_layer_list();
					}
					if (selection.startedAt.is_set())
					{
						selection.range.include((selection.startedAt.get().to_vector3() - vm->get_zero_offset()) * vm->get_voxel_size());
					}
					if (selection.draggingTo.is_set())
					{
						selection.range.include((selection.draggingTo.get().to_vector3() - vm->get_zero_offset()) * vm->get_voxel_size());
					}
					selection.range.x.max += vm->get_voxel_size();
					selection.range.y.max += vm->get_voxel_size();
					selection.range.z.max += vm->get_voxel_size();

					if (camera.orthogonal)
					{
						RangeInt3 rv = CONTENT_ACCESS::get_combined_range(vm);
						Range3 r = vm->to_full_range(rv);
						if (camera.orthogonalAxis == 0)
						{
							selection.range.x = r.x;
						}
						if (camera.orthogonalAxis == 1)
						{
							selection.range.y = r.y;
						}
						if (camera.orthogonalAxis == 2)
						{
							selection.range.z = r.z;
						}
						if (limitToPlaneAxis != NONE)
						{
							VectorInt3 l2p = vm->to_voxel_coord(limitToPlaneOrigin);
							RangeInt3 l2pr = RangeInt3::empty;
							l2pr.include(l2p);
							Range3 l2prf = vm->to_full_range(l2pr);
							if (limitToPlaneAxis == 0)
							{
								selection.range.x = l2prf.x;
							}
							if (limitToPlaneAxis == 1)
							{
								selection.range.y = l2prf.y;
							}
							if (limitToPlaneAxis == 2)
							{
								selection.range.z = l2prf.z;
							}
						}
					}
				}
			}

			inputGrabbed = false;
		}

		return true;
	}

	return false;
}

bool Editor::VoxelMold::on_hold(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
{
	if (on_hold_held(_cursorIdx, _buttonIdx, _rayStart, _rayDir))
	{
		return true;
	}

	return base::on_hold(_cursorIdx, _buttonIdx, _rayStart, _rayDir);
}

bool Editor::VoxelMold::on_held(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir, Vector3 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised)
{
	if (on_hold_held(_cursorIdx, _buttonIdx, _rayStart, _rayDir))
	{
		return true;
	}

	return base::on_held(_cursorIdx, _buttonIdx, _rayStart, _rayDir, _movedBy, _cursorMovedBy2DNormalised);
}

void Editor::VoxelMold::on_release(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
{
	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == ::System::MouseButton::Left)
	{
		lastOperationCell.clear();
		// selection.range should be already set
		selection.startedAt.clear();
		selection.draggingTo.clear();
	}

	base::on_release(_cursorIdx, _buttonIdx, _rayStart, _rayDir);
}

void Editor::VoxelMold::process_controls()
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

	// limit to plane
	{
		if (limitToPlaneAxis != NONE)
		{
			if (!limitToPlaneGizmo.get())
			{
				limitToPlaneGizmo = Gizmo3D::get_one();
				limitToPlaneGizmo->atWorld = limitToPlaneOrigin;
				add_gizmo(limitToPlaneGizmo.get());
			}
			Vector3 axis = Vector3::zero;
			axis.access_element(limitToPlaneAxis) = 1.0f;
			limitToPlaneGizmo->axisWorld = axis;
			limitToPlaneOrigin = limitToPlaneGizmo->atWorld; // will store if moved
			limitToPlaneGizmo->fromWorld = limitToPlaneGizmo->atWorld;
			limitToPlaneGizmo->fromWorld.access().access_element(limitToPlaneAxis) = 0.0f;
		}
		else
		{
			limitToPlaneGizmo.clear(); // will auto remove
		}
	}

	// active voxel layer
	{
		Framework::VoxelMoldLayer * activeVoxelLayer = nullptr;
		auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset());
		if (vm && (mode == Mode::Move || mode == Mode::Select))
		{
			activeVoxelLayer = fast_cast<Framework::VoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm));
		}
		Range3 forSelectionRangeNow = selection.range;
		if (activeVoxelLayer)
		{
			if (voxelLayerGizmos.forLayer != activeVoxelLayer ||
				voxelLayerGizmos.forSelectionRange != forSelectionRangeNow)
			{
				remove_voxel_layer_gizmos();
			}

			// create if required
			if (!voxelLayerGizmos.offsetGizmo.is_in_use())
			{
				if (! forSelectionRangeNow.is_empty())
				{
					voxelLayerGizmos.zeroOffsetAt = forSelectionRangeNow.centre();
				}
				else
				{
					RangeInt3 vr = activeVoxelLayer->get_range();
					if (vr.is_empty())
					{
						vr = CONTENT_ACCESS::get_combined_range(vm);
					}
					if (vr.is_empty())
					{
						voxelLayerGizmos.zeroOffsetAt = Vector3::zero;
					}
					else
					{
						VectorInt3 cLocal = vr.is_empty() ? VectorInt3::zero : vr.centre();
						VectorInt3 cWorld = activeVoxelLayer->to_world(vm, cLocal);
						voxelLayerGizmos.zeroOffsetAt = vm->to_full_coord(cWorld);
					}
				}
				voxelLayerGizmos.offsetGizmo.atWorld = voxelLayerGizmos.zeroOffsetAt;
				voxelLayerGizmos.offsetGizmo.fromWorld = Vector3::zero;
				voxelLayerGizmos.forLayer = activeVoxelLayer;
				voxelLayerGizmos.forSelectionRange = forSelectionRangeNow;
			}

			// update
			bool didSomethingChange = false;
			{
				// update main offset gizmo now
				voxelLayerGizmos.offsetGizmo.update(this);

				{
					bool makeCopy = false;
					{
						// outside offset is zero as we want to update this even if not holding/grabbing
						if (!grabbedAnyGizmo)
						{
							voxelLayerGizmos.allowMakeCopy = true;
						}
						if (auto* input = ::System::Input::get())
						{
#ifdef AN_STANDARD_INPUT
							if (input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift))
							{
								if (voxelLayerGizmos.allowMakeCopy)
								{
									makeCopy = true;
								}
							}
							else
#endif
							{
								voxelLayerGizmos.allowMakeCopy = true;
							}
						}
					}

					VectorInt3 offset = TypeConversions::Normal::f_i_closest((voxelLayerGizmos.offsetGizmo.atWorld - voxelLayerGizmos.zeroOffsetAt) / vm->get_voxel_size());
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
							activeVoxelLayer = fast_cast<VoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm));
							if (activeVoxelLayer && makeTemporary)
							{
								activeVoxelLayer->be_temporary();
							}
							CONTENT_ACCESS::merge_temporary_layers(vm, activeVoxelLayer);
							selection = Selection();

							// but keep gizmos
							voxelLayerGizmos.forLayer = activeVoxelLayer;
							voxelLayerGizmos.forSelectionRange = selection.range;
							voxelLayerGizmos.allowMakeCopy = !makeCopy;

							combine_voxels();
							populate_layer_list();
						}
						else
						{
							activeVoxelLayer->offset_placement(offset);
							voxelLayerGizmos.zeroOffsetAt += offset.to_vector3() * vm->get_voxel_size();
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
			remove_voxel_layer_gizmos();
		}
	}

	// active transform layer
	{
		Framework::VoxelMoldTransformLayer * activeTransformLayer = nullptr;
		auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset());
		if (vm)
		{
			activeTransformLayer = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm));
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
				transformLayerGizmos.offsetGizmo.atWorld = activeTransformLayer->access_offset().to_vector3() * vm->get_voxel_size();
				transformLayerGizmos.offsetGizmo.fromWorld = Vector3::zero;
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
					transformLayerGizmos.mirrorAtGizmo = Gizmo3D::get_one();
					auto* g = transformLayerGizmos.mirrorAtGizmo.get();
					add_gizmo(g);
					g->colour = Colour::c64LightBlue;
					g->radius2d *= 0.8f;
					g->axisWorld = Vector3::zero;
					g->axisWorld.access().access_element(transformLayerGizmos.mirrorAtGizmoForAxis) = 1.0f;
					g->atWorld = Vector3::zero;
					g->atWorld.access_element(transformLayerGizmos.mirrorAtGizmoForAxis) = (float)(mirror.at + (mirror.axis? 0.5f: 0.0f)) * vm->get_voxel_size();
					g->atWorld += transformLayerGizmos.offsetGizmo.atWorld;
				}
			}

			// update
			bool didSomethingChange = false;
			{

				Vector3 mirrorAtRelative = Vector3::zero; // use this to move against main offset gizmo

				// read before we update offset gizmo, so we have the latest movement
				if (auto* g = transformLayerGizmos.mirrorAtGizmo.get())
				{
					int prevAt = mirror.at;
					mirrorAtRelative = g->atWorld - transformLayerGizmos.offsetGizmo.atWorld;
					float use = mirrorAtRelative.get_element(transformLayerGizmos.mirrorAtGizmoForAxis);
					if (mirror.axis)
					{
						mirror.at = TypeConversions::Normal::f_i_cells(use / vm->get_voxel_size());
					}
					else
					{
						mirror.at = TypeConversions::Normal::f_i_closest(use / vm->get_voxel_size());
					}
					didSomethingChange |= mirror.at != prevAt;
				}

				// update main offset gizmo now
				transformLayerGizmos.offsetGizmo.update(this);

				{
					VectorInt3& offset = activeTransformLayer->access_offset();
					VectorInt3 prevOffset = offset;
					offset = TypeConversions::Normal::f_i_closest(transformLayerGizmos.offsetGizmo.atWorld / vm->get_voxel_size());
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
		Framework::VoxelMoldFullTransformLayer * activeTransformLayer = nullptr;
		auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset());
		if (vm)
		{
			activeTransformLayer = fast_cast<Framework::VoxelMoldFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm));
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
				fullTransformLayerGizmos.offsetAndRotationGizmo.atWorld = activeTransformLayer->access_offset() * vm->get_voxel_size();
				fullTransformLayerGizmos.offsetAndRotationGizmo.fromWorld = Vector3::zero;
				fullTransformLayerGizmos.offsetAndRotationGizmo.rotation = activeTransformLayer->access_rotate();
				fullTransformLayerGizmos.forLayer = activeTransformLayer;
			}

			// update
			bool didSomethingChange = false;
			{
				// update main offset gizmo now
				fullTransformLayerGizmos.offsetAndRotationGizmo.update(this);

				{
					Vector3& offset = activeTransformLayer->access_offset();
					Rotator3& rotate = activeTransformLayer->access_rotate();
					Vector3 prevOffset = offset;
					Rotator3 prevRotate = rotate;
					offset = fullTransformLayerGizmos.offsetAndRotationGizmo.atWorld / vm->get_voxel_size();
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
					selection.gizmos.rangeXMinGizmo = Gizmo3D::get_one();
					auto* g = selection.gizmos.rangeXMinGizmo.get();
					g->atWorld.x = selection.range.x.min + selection.gizmosOffDistance.x.min;
					g->atWorld.y = selection.range.y.centre();
					g->atWorld.z = selection.range.z.centre();
					g->axisWorld = Vector3::xAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeXMaxGizmo.get())
				{
					selection.gizmos.rangeXMaxGizmo = Gizmo3D::get_one();
					auto* g = selection.gizmos.rangeXMaxGizmo.get();
					g->atWorld.x = selection.range.x.max + selection.gizmosOffDistance.x.max;
					g->atWorld.y = selection.range.y.centre();
					g->atWorld.z = selection.range.z.centre();
					g->axisWorld = Vector3::xAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeYMinGizmo.get())
				{
					selection.gizmos.rangeYMinGizmo = Gizmo3D::get_one();
					auto* g = selection.gizmos.rangeYMinGizmo.get();
					g->atWorld.x = selection.range.x.centre();
					g->atWorld.y = selection.range.y.min + selection.gizmosOffDistance.y.min;
					g->atWorld.z = selection.range.z.centre();
					g->axisWorld = Vector3::yAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeYMaxGizmo.get())
				{
					selection.gizmos.rangeYMaxGizmo = Gizmo3D::get_one();
					auto* g = selection.gizmos.rangeYMaxGizmo.get();
					g->atWorld.x = selection.range.x.centre();
					g->atWorld.y = selection.range.y.max + selection.gizmosOffDistance.y.max;
					g->atWorld.z = selection.range.z.centre();
					g->axisWorld = Vector3::yAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeZMinGizmo.get())
				{
					selection.gizmos.rangeZMinGizmo = Gizmo3D::get_one();
					auto* g = selection.gizmos.rangeZMinGizmo.get();
					g->atWorld.x = selection.range.x.centre();
					g->atWorld.y = selection.range.y.centre();
					g->atWorld.z = selection.range.z.min + selection.gizmosOffDistance.z.min;
					g->axisWorld = Vector3::zAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
				if (!selection.gizmos.rangeZMaxGizmo.get())
				{
					selection.gizmos.rangeZMaxGizmo = Gizmo3D::get_one();
					auto* g = selection.gizmos.rangeZMaxGizmo.get();
					g->atWorld.x = selection.range.x.centre();
					g->atWorld.y = selection.range.y.centre();
					g->atWorld.z = selection.range.z.max + selection.gizmosOffDistance.z.max;
					g->axisWorld = Vector3::zAxis;
					g->colour = SELECTION_COLOUR;
					add_gizmo(g);
				}
			}
			selection.range = Range3::empty;
			selection.range.x.include(selection.gizmos.rangeXMinGizmo->atWorld.x - selection.gizmosOffDistance.x.min);
			selection.range.x.include(selection.gizmos.rangeXMaxGizmo->atWorld.x - selection.gizmosOffDistance.x.max);
			selection.range.y.include(selection.gizmos.rangeYMinGizmo->atWorld.y - selection.gizmosOffDistance.y.min);
			selection.range.y.include(selection.gizmos.rangeYMaxGizmo->atWorld.y - selection.gizmosOffDistance.y.max);
			selection.range.z.include(selection.gizmos.rangeZMinGizmo->atWorld.z - selection.gizmosOffDistance.z.min);
			selection.range.z.include(selection.gizmos.rangeZMaxGizmo->atWorld.z - selection.gizmosOffDistance.z.max);

			selection.gizmos.rangeXMinGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeXMaxGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeYMinGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeYMaxGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeZMinGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeZMaxGizmo->atWorld = selection.range.centre();
			selection.gizmos.rangeXMinGizmo->atWorld.x = selection.range.x.min;
			selection.gizmos.rangeXMaxGizmo->atWorld.x = selection.range.x.max;
			selection.gizmos.rangeYMinGizmo->atWorld.y = selection.range.y.min;
			selection.gizmos.rangeYMaxGizmo->atWorld.y = selection.range.y.max;
			selection.gizmos.rangeZMinGizmo->atWorld.z = selection.range.z.min;
			selection.gizmos.rangeZMaxGizmo->atWorld.z = selection.range.z.max;

			selection.gizmosOffDistance.x.min = -calculate_radius_for(selection.gizmos.rangeXMinGizmo->atWorld, selection.gizmos.rangeXMinGizmo->radius2d);
			selection.gizmosOffDistance.x.max =  calculate_radius_for(selection.gizmos.rangeXMaxGizmo->atWorld, selection.gizmos.rangeXMaxGizmo->radius2d);
			selection.gizmosOffDistance.y.min = -calculate_radius_for(selection.gizmos.rangeYMinGizmo->atWorld, selection.gizmos.rangeYMinGizmo->radius2d);
			selection.gizmosOffDistance.y.max =  calculate_radius_for(selection.gizmos.rangeYMaxGizmo->atWorld, selection.gizmos.rangeYMaxGizmo->radius2d);
			selection.gizmosOffDistance.z.min = -calculate_radius_for(selection.gizmos.rangeZMinGizmo->atWorld, selection.gizmos.rangeZMinGizmo->radius2d);
			selection.gizmosOffDistance.z.max =  calculate_radius_for(selection.gizmos.rangeZMaxGizmo->atWorld, selection.gizmos.rangeZMaxGizmo->radius2d);

			selection.gizmos.rangeXMinGizmo->atWorld.x += selection.gizmosOffDistance.x.min;
			selection.gizmos.rangeXMaxGizmo->atWorld.x += selection.gizmosOffDistance.x.max;
			selection.gizmos.rangeYMinGizmo->atWorld.y += selection.gizmosOffDistance.y.min;
			selection.gizmos.rangeYMaxGizmo->atWorld.y += selection.gizmosOffDistance.y.max;
			selection.gizmos.rangeZMinGizmo->atWorld.z += selection.gizmosOffDistance.z.min;
			selection.gizmos.rangeZMaxGizmo->atWorld.z += selection.gizmosOffDistance.z.max;
		}
		else
		{
			selection.gizmos = Selection::Gizmos();
		}
	}

	// update windows
	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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

void Editor::VoxelMold::update_cursor_ray(int _cursorIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
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
		cr.rayDir.is_set() && cr.rayDir == _rayDir &&
		cr.onlyActualContent.is_set() && cr.onlyActualContent == onlyActualContent)
	{
		// already cached
		return;
	}

	cr.rayStart = _rayStart;
	cr.rayDir = _rayDir;
	cr.onlyActualContent = onlyActualContent;

	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
	{
		Framework::VoxelHitInfo hitInfo;
		if (limitToPlaneAxis != NONE)
		{
			if (CONTENT_ACCESS::ray_cast_plane(vm, _rayStart, _rayDir, hitInfo, limitToPlaneAxis, limitToPlaneOrigin.get_element(limitToPlaneAxis)))
			{
				cr.hitVoxel = false;
				cr.cellCoord = hitInfo.hit;
				cr.hitNormal = hitInfo.normal;
			}
			else
			{
				cr.hitVoxel = false;
				cr.cellCoord.clear();
				cr.hitNormal.clear();
			}
		}
		else if (CONTENT_ACCESS::ray_cast(vm, _rayStart, _rayDir, hitInfo, onlyActualContent))
		{
			cr.hitVoxel = true;
			cr.cellCoord = hitInfo.hit;
			cr.hitNormal = hitInfo.normal;
		}
		else
		{
			int axisPlane = 2;

			if (camera.orthogonal)
			{
				axisPlane = camera.orthogonalAxis;
			}

			if (CONTENT_ACCESS::ray_cast_plane(vm, _rayStart, _rayDir, hitInfo, axisPlane, 0.0f))
			{
				cr.hitVoxel = false;
				cr.cellCoord = hitInfo.hit;
				cr.hitNormal = hitInfo.normal;
			}
			else
			{
				cr.hitVoxel = false;
				cr.cellCoord.clear();
				cr.hitNormal.clear();
			}
		}
	}
}

void Editor::VoxelMold::do_operation(int _cursorIdx, Operation _operation, bool _checkLastOperationCell)
{
	bool didSomething = false;

	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
	{
		if (cursorRays.is_index_valid(_cursorIdx))
		{
			auto& cr = cursorRays[_cursorIdx];

			if (cr.cellCoord.is_set() &&
				cr.hitNormal.is_set())
			{
				auto* vml = fast_cast<VoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm));
				if (vml)
				{
					RangeInt3 r = CONTENT_ACCESS::get_combined_range(vm);
					if (r.is_empty())
					{
						r = RangeInt3::zero;
					}
					// allow to add only close to already existing blocks
					int extraRange = 20;
					r.expand_by(VectorInt3(extraRange, extraRange, extraRange));
					struct ValidRange
					{
						RangeInt3 validRange = RangeInt3::empty;
						void setup(Framework::VoxelMold* vm, Range3 const& _r)
						{
							if (!_r.is_empty())
							{
								validRange = vm->to_voxel_range(_r, SELECTION_FULLY_INLCUDED);
							}
						}
						bool is_ok(VectorInt3 const& _a) const { return validRange.is_empty() || validRange.does_contain(_a); }
					} validRange;
					validRange.setup(vm, selection.range);
					if (r.does_contain(cr.cellCoord.get()))
					{
						if (_operation == Operation::AddVoxel)
						{
							VectorInt3 at = cr.cellCoord.get();
							VectorInt3 atSide = at + cr.hitNormal.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get() && atSide != lastOperationCell.get()))
							{
								VectorInt3 useAt = cr.hitVoxel ? atSide : at;
								if (validRange.is_ok(useAt))
								{
									VectorInt3 useAtLocal = vml->to_local(vm, useAt);
									auto& voxel = vml->access_at(useAtLocal);
									voxel.voxel = get_selected_colour_index();
									didSomething = true;
									lastOperationCell = atSide;
									CONTENT_ACCESS::prune(vm);
									combine_voxels();
								}
							}
						}
						if (_operation == Operation::FillAtVoxel ||
							_operation == Operation::FillRemoveAtVoxel)
						{
							VectorInt3 at = cr.cellCoord.get();
							VectorInt3 atSide = at + cr.hitNormal.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get() && atSide != lastOperationCell.get()))
							{
								bool shiftPressed = false;
#ifdef AN_STANDARD_INPUT
								if (auto* input = ::System::Input::get())
								{
									shiftPressed = input->is_key_pressed(System::Key::LeftShift) || input->is_key_pressed(System::Key::RightShift);
								}
#endif
								VectorInt3 useAt = cr.hitVoxel && !shiftPressed && _operation != Operation::FillRemoveAtVoxel ? atSide : at;
								if (validRange.is_ok(useAt))
								{
									int useColourIdx = _operation == Operation::FillRemoveAtVoxel ? Framework::Voxel::None : get_selected_colour_index();
									Optional<int> startColour;
									CONTENT_ACCESS::fill_at(vm, vml, useAt, [&startColour, useColourIdx](Voxel& _voxel)
										{
											if (!startColour.is_set())
											{
												startColour = _voxel.voxel;
											}
											if (_voxel.voxel == startColour.get())
											{
												_voxel.voxel = useColourIdx;
												return true;
											}
											return false;
										});
									didSomething = true;
									lastOperationCell = atSide;
									CONTENT_ACCESS::prune(vm);
									combine_voxels();
								}
							}
						}
						if (_operation == Operation::RemoveVoxel)
						{
							VectorInt3 at = cr.cellCoord.get();
							VectorInt3 atSide = at + cr.hitNormal.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get() && atSide != lastOperationCell.get()))
							{
								if (validRange.is_ok(at))
								{
									VectorInt3 atLocal = vml->to_local(vm, at);
									auto& voxel = vml->access_at(atLocal);
									voxel.voxel = Framework::Voxel::None;
									didSomething = true;
									lastOperationCell = at;
									CONTENT_ACCESS::prune(vm);
									combine_voxels();
								}
							}
						}
						if (_operation == Operation::PaintVoxel)
						{
							VectorInt3 at = cr.cellCoord.get();
							VectorInt3 atSide = at + cr.hitNormal.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get() && atSide != lastOperationCell.get()))
							{
								if (validRange.is_ok(at))
								{
									VectorInt3 atLocal = vml->to_local(vm, at);
									auto& voxel = vml->access_at(atLocal);
									if (Framework::Voxel::is_anything(voxel.voxel))
									{
										voxel.voxel = get_selected_colour_index();
										didSomething = true;
										lastOperationCell = at;
										CONTENT_ACCESS::prune(vm);
										combine_voxels();
									}
								}
							}
						}
						if (_operation == Operation::PaintFillVoxel)
						{
							VectorInt3 at = cr.cellCoord.get();
							VectorInt3 atSide = at + cr.hitNormal.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get() && atSide != lastOperationCell.get()))
							{
								VectorInt3 atLocal = vml->to_local(vm, at);
								if (validRange.is_ok(at))
								{
									auto& voxel = vml->access_at(atLocal);
									if (Framework::Voxel::is_anything(voxel.voxel) &&
										voxel.voxel != get_selected_colour_index())
									{
										{
											int replace = voxel.voxel;
											Array<VectorInt3> at;
											at.push_back(atLocal);
											while (!at.is_empty())
											{
												VectorInt3 atLocal = at.get_first();
												at.pop_front();
												auto& voxel = vml->access_at(atLocal);
												if (voxel.voxel == replace)
												{
													voxel.voxel = get_selected_colour_index();
													for_range(int, ox, -1, 1)
													{
														for_range(int, oy, -1, 1)
														{
															for_range(int, oz, -1, 1)
															{
																if (ox != 0 || oy != 0 || oz != 0)
																{
																	at.push_back(atLocal + VectorInt3(ox, oy, oz));
																}
															}
														}
													}
												}
											}
										}
										didSomething = true;
										lastOperationCell = at;
										CONTENT_ACCESS::prune(vm);
										combine_voxels();
									}
								}
							}
						}
						if (_operation == Operation::ReplaceColour)
						{
							VectorInt3 at = cr.cellCoord.get();
							VectorInt3 atSide = at + cr.hitNormal.get();
							if (!_checkLastOperationCell || !lastOperationCell.is_set() || (at != lastOperationCell.get() && atSide != lastOperationCell.get()))
							{
								if (validRange.is_ok(at))
								{
									VectorInt3 atLocal = vml->to_local(vm, at);
									auto& voxel = vml->access_at(atLocal);
									if (Framework::Voxel::is_anything(voxel.voxel) &&
										voxel.voxel != get_selected_colour_index())
									{
										{
											int replace = voxel.voxel;
											for_every_ref(l, CONTENT_ACCESS::get_layers(vm))
											{
												if (auto* vml = fast_cast<VoxelMoldLayer>(l))
												{
													auto range = vml->get_range();
													for_range(int, z, range.z.min, range.z.max)
													{
														for_range(int, y, range.y.min, range.y.max)
														{
															for_range(int, x, range.x.min, range.x.max)
															{
																VectorInt3 atLocal(x, y, z);
																auto& voxel = vml->access_at(atLocal);
																if (voxel.voxel == replace)
																{
																	voxel.voxel = get_selected_colour_index();
																}
															}
														}
													}
												}
											}
										}
										didSomething = true;
										lastOperationCell = at;
										CONTENT_ACCESS::prune(vm);
										combine_voxels();
									}
								}
							}
						}
					}
				}
				if (_operation == Operation::GetColourFromVoxel)
				{
					// colour can be read from any layer, top most first and without range/selection checks
					VectorInt3 at = cr.cellCoord.get();
					bool read = false;
					/*
					if (vml)
					{
						VectorInt3 atLocal = vml->to_local(vm, at);
						auto& voxel = vml->access_at(atLocal);
						if (Framework::Voxel::is_anything(voxel.voxel))
						{
							set_selected_colour_index(voxel.voxel);
							read = true;
						}
					}
					*/
					if (!read)
					{
						// read from any layer
						for_every_ref(l, CONTENT_ACCESS::get_layers(vm))
						{
							if (auto* vml = fast_cast<VoxelMoldLayer>(l))
							{
								VectorInt3 atLocal = vml->to_local(vm, at);
								auto& voxel = vml->access_at(atLocal);
								if (Framework::Voxel::is_anything(voxel.voxel))
								{
									set_selected_colour_index(voxel.voxel);
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

		if (auto* vml = fast_cast<VoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
		{
			if (_operation == Operation::FillSelection ||
				_operation == Operation::ClearSelection ||
				_operation == Operation::PaintSelection ||
				_operation == Operation::DuplicateSelectionToNewLayer ||
				_operation == Operation::MoveSelectionToNewLayer)
			{
				if (!selection.range.is_empty())
				{
					VoxelMoldLayer* nvml = nullptr;
					if (_operation == Operation::DuplicateSelectionToNewLayer ||
						_operation == Operation::MoveSelectionToNewLayer)
					{
						nvml = new Framework::VoxelMoldLayer();
						CONTENT_ACCESS::add_layer(vm, nvml, 0);
						populate_layer_list();
					}
					RangeInt3 range = vm->to_voxel_range(selection.range, SELECTION_FULLY_INLCUDED);
					// first copy to new layer if requested (mirrors may make use remove stuff before we copy it)
					if (nvml)
					{
						for_range(int, z, range.z.min, range.z.max)
						{
							for_range(int, y, range.y.min, range.y.max)
							{
								for_range(int, x, range.x.min, range.x.max)
								{
									VectorInt3 atGlobal(x, y, z);
									VectorInt3 atLocal = vml->to_local(vm, atGlobal, true);
									auto& v = vml->access_at(atLocal);
									if (Framework::Voxel::is_anything(v.voxel))
									{
										VectorInt3 atLocalNVML = nvml->to_local(vm, atGlobal, true);
										auto& dstv = nvml->access_at(atLocalNVML);
										dstv = v;
									}
								}
							}
						}
					}
					for_range(int, z, range.z.min, range.z.max)
					{
						for_range(int, y, range.y.min, range.y.max)
						{
							for_range(int, x, range.x.min, range.x.max)
							{
								VectorInt3 atGlobal(x, y, z);
								VectorInt3 atLocal = vml->to_local(vm, atGlobal);
								auto& v = vml->access_at(atLocal);
								if (_operation == Operation::FillSelection)
								{
									v.voxel = get_selected_colour_index();
								}
								if (_operation == Operation::ClearSelection ||
									_operation == Operation::MoveSelectionToNewLayer)
								{
									v.voxel = Framework::Voxel::None;
								}
								if (_operation == Operation::PaintSelection && Framework::Voxel::is_anything(v.voxel))
								{
									v.voxel = get_selected_colour_index();
								}
							}
						}
					}
				}
			}
			CONTENT_ACCESS::prune(vm);
			combine_voxels();
			didSomething = true;
		}

		if (_operation == Operation::AddVoxelLayer)
		{
			bool createNew = true;
			if (auto* vl = fast_cast<IVoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
			{
				if (vl->is_temporary())
				{
					vl->be_temporary(false);
					createNew = false;
				}
			}
			if (createNew)
			{
				CONTENT_ACCESS::add_layer(vm, new Framework::VoxelMoldLayer(), 1);
			}
			didSomething = true;
			combine_voxels();
			populate_layer_list();
		}
		if (_operation == Operation::AddModLayer)
		{
			CONTENT_ACCESS::change_depth_of_edited_layer_and_its_children(vm, 1);
			auto* l = new Framework::VoxelMoldTransformLayer();
			CONTENT_ACCESS::add_layer(vm, l, 0);
			l->set_depth(l->get_depth() - 1); // as we changed depth of edited and placed where edited was, we need to reverse that small change now
			didSomething = true;
			combine_voxels();
			populate_layer_list();
		}
		if (_operation == Operation::AddFullModLayer)
		{
			CONTENT_ACCESS::change_depth_of_edited_layer_and_its_children(vm, 1);
			auto* l = new Framework::VoxelMoldFullTransformLayer();
			CONTENT_ACCESS::add_layer(vm, l, 0);
			l->set_depth(l->get_depth() - 1); // as we changed depth of edited and placed where edited was, we need to reverse that small change now
			didSomething = true;
			combine_voxels();
			populate_layer_list();
		}
		if (_operation == Operation::AddIncludeLayer)
		{
			UI::Utils::choose_library_stored<Framework::VoxelMold>(get_canvas(), NAME(chooseLibraryStored_includeVoxelMold), TXT("include voxel mold"), lastVoxelMoldIncluded.get(), 1.0f,
				[this, vm](Framework::VoxelMold* _chosen)
				{
					if (CONTENT_ACCESS::can_include(vm, _chosen))
					{
						auto* l = new Framework::IncludeVoxelMoldLayer();
						l->set_included(_chosen);
						lastVoxelMoldIncluded = _chosen;
						l->set_id(_chosen->get_name().get_name());
						CONTENT_ACCESS::add_layer(vm, l, 1);
						combine_voxels();
						populate_layer_list();
						update_ui_highlight();
					}
					else
					{
						UI::Utils::QuestionWindow::show_message(get_canvas(), String::printf(TXT("can't include voxel mold \"%S\""), _chosen->get_name().to_string().to_char()).to_char());
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
			if (auto* vl = fast_cast<IVoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
			{
				vl->be_temporary(!vl->is_temporary());
			}
			didSomething = true;
			populate_layer_list();
		}
		if (_operation == Operation::RemoveLayer)
		{
			CONTENT_ACCESS::remove_edited_layer(vm);
			combine_voxels();
			didSomething = true;
			populate_layer_list();
		}
		if (_operation == Operation::ChangedLayerProps ||
			_operation == Operation::ChangedVoxelMoldProps)
		{
			combine_voxels();
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
			combine_voxels();
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
			combine_voxels();
			populate_layer_list();
		}
	}

	if (didSomething)
	{
		if (editorContext)
		{
			if (_operation != Operation::GetColourFromVoxel)
			{
				if (editorContext->mark_save_required() ||
					_operation == Operation::ChangedVoxelMoldProps)
				{
					update_ui_highlight();
				}
			}
		}

		cursorRays.clear();
	}
}

void Editor::VoxelMold::render_debug(CustomRenderContext* _customRC)
{
#ifdef AN_DEBUG_RENDERER
	debug_gather_in_renderer();

	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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
				(cr->hitVoxel || limitToPlaneAxis != NONE))
			{
				mayRemove = mode == Mode::Remove || mode == Mode::AddRemove || (mode == Mode::Fill);
				mayColour = mode == Mode::Paint || mode == Mode::PaintFill || mode == Mode::ReplaceColour || mode == Mode::GetColour;
			}
			if (cr->cellCoord.is_set())
			{
				maySelect = mode == Mode::Select;
			}
			if (cr->cellCoord.is_set() &&
				cr->hitNormal.is_set())
			{
				mayAdd = mode == Mode::Add || mode == Mode::AddRemove || (mode == Mode::Fill && ! shiftPressed);
			}

			if (mayRemove || mayColour || maySelect)
			{
				vm->debug_draw_voxel(cr->cellCoord.get(), Colour::lerp(::System::Core::get_timed_pulse(1.0f) * 0.2f, mayAdd? Colour::black : Colour::white, Colour::red), true);
			}
			if (mayAdd)
			{
				if (cr->hitVoxel)
				{
					vm->debug_draw_voxel(cr->cellCoord.get() + cr->hitNormal.get(), Colour::lerp(::System::Core::get_timed_pulse(1.0f) * 0.2f, Colour::white, Colour::red), true);
				}
				else
				{
					vm->debug_draw_voxel(cr->cellCoord.get(), Colour::lerp(::System::Core::get_timed_pulse(1.0f) * 0.2f, Colour::white, Colour::red), true);
				}
			}
		}
		if (! selection.range.is_empty())
		{
			Vector3 cmin = selection.range.near_bottom_left();
			Vector3 cmax = selection.range.far_top_right();
			float wider = vm->get_voxel_size() * 0.01f;
			cmin -= Vector3::one * wider;
			cmax += Vector3::one * wider;
			vm->debug_draw_box_region(cmin, cmax, SELECTION_COLOUR, true, 0.2f);
			{
				RangeInt3 voxelRange = vm->to_voxel_range(selection.range, SELECTION_FULLY_INLCUDED);
				Range3 voxeledRange = vm->to_full_range(voxelRange);
				if (!voxeledRange.is_empty())
				{
					vm->debug_draw_box_region(voxeledRange.near_bottom_left(), voxeledRange.far_top_right(), Colour::lerp(0.5f, SELECTION_COLOUR, Colour::red), true);
				}
			}
		}
	}

	if (limitToPlaneAxis != NONE)
	{
		float gridStep = grid.size / 500.0f;

		if (gridStep < 0.05f) gridStep = 0.05f; else
		if (gridStep < 0.10f) gridStep = 0.10f; else
		if (gridStep < 0.25f) gridStep = 0.25f; else
		if (gridStep < 0.50f) gridStep = 0.50f; else
		if (gridStep < 1.00f) gridStep = 1.00f; else
		if (gridStep < 5.00f) gridStep = 5.00f; else
		gridStep = 10.0f;

		float startG = round_to(-grid.size * 0.5f, gridStep);
		float endG = round_to(grid.size * 0.5f, gridStep);

		Vector3 axisP = Vector3::zero;
		axisP.access_element(limitToPlaneAxis) = 1.0f;
		Vector3 axisA = Vector3::zero;
		Vector3 axisB = Vector3::zero;
		axisA.access_element(limitToPlaneAxis == 0? 1 : 0) = 1.0f;
		axisB.access_element(limitToPlaneAxis == 2? 1 : 2) = 1.0f;
		Vector3 gridStart = limitToPlaneOrigin;
		Colour gridColour = Colour::lerp(0.5f, Colour::greyLight, Colour::cyan);
		for_count(int, axisIdx, 2)
		{
			Vector3 axisX = axisIdx == 0 ? axisA : axisB;
			Vector3 axisY = axisIdx == 0 ? axisB : axisA;
			Vector3 axisH = axisY * grid.size * 0.5f;
			for (float g = startG; g <= endG + 0.0001f; g += gridStep)
			{
				float a = 0.2f;
				float ac = g;
				if (abs(ac - round(ac)) < 0.001f)
				{
					a = max(a, 1.0f);
				}
				ac *= 2.0f;
				if (abs(ac - round(ac)) < 0.001f)
				{
					a = max(a, 0.5f);
				}
				Vector3 gAt = gridStart + axisX * g;
				debug_draw_line(false, gridColour.with_set_alpha(a), gAt - axisH, gAt + axisH);
			}
		}
	}

	debug_gather_restore();
#endif

	base::render_debug(_customRC);
}

void Editor::VoxelMold::render_edited_asset()
{
	if (auto* vm = fast_cast<Framework::VoxelMold>(editedAsset.get()))
	{
		Optional<Plane> highlightPlane;
		if (limitToPlaneAxis != NONE)
		{
			Vector3 planeNormal = Vector3::zero;
			planeNormal.access_element(limitToPlaneAxis) = 1.0f;
			highlightPlane = Plane(planeNormal, limitToPlaneOrigin);
		}
		Framework::VoxelMold::RenderContext context;
		context.highlightPlane = highlightPlane;
		if (highlightEditedLayerFor > 0.0f)
		{
			context.editedLayerColour = HIGHLIGHT_EDITED_LAYER_COLOUR;
		}
		if (drawEdges)
		{
			context.voxelEdgeColour = Colour::black.with_alpha(0.5f);
		}
		context.onlySolidVoxels = onlySolidVoxels;
		vm->render(context);
	}
	else
	{
		base::render_edited_asset();
	}
}

void Editor::VoxelMold::setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	base::setup_ui_add_to_bottom_left_panel(c, panel, scale);

	{
		auto* b = Framework::UI::CanvasButton::get();
		b->set_id(NAME(editorVoxelMold_drawEdges));
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
		b->set_id(NAME(editorVoxelMold_onlySolidVoxels));
		b->set_caption(TXT("only solid"));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				onlySolidVoxels = !onlySolidVoxels;
				update_ui_highlight();
			});
		panel->add(b);
	}
}

void Editor::VoxelMold::setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
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
		b->set_id(NAME(editorVoxelMold_editedLayerName));
		b->set_caption(TXT("layer"));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_font_scale(get_name_font_scale());
		b->set_text_only();
		panel->add(b);
	}
}

void Editor::VoxelMold::setup_ui(REF_ SetupUIContext& _setupUIContext)
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
					b->set_id(NAME(editorVoxelMold_addRemove));
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
					b->set_id(NAME(editorVoxelMold_add));
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
					b->set_id(NAME(editorVoxelMold_remove));
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
					b->set_id(NAME(editorVoxelMold_fill));
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
					b->set_id(NAME(editorVoxelMold_colourPicker));
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
					b->set_id(NAME(editorVoxelMold_paint));
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
					b->set_id(NAME(editorVoxelMold_paintFill));
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
					b->set_id(NAME(editorVoxelMold_replaceColour));
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
					b->set_id(NAME(editorVoxelMold_select));
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
					b->set_id(NAME(editorVoxelMold_move));
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
			menu->set_title(TXT("limit to plane"))->set_closable(false)->set_movable(false);
			c->add(menu);
			{
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("clear")))->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							limitToPlaneAxis = NONE;
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("xy")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N1, UI::ShortcutParams().with_ctrl())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorVoxelMold_limitToPlane_XY));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							limitToPlaneAxis = limitToPlaneAxis == 2? NONE : 2;
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("xz")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N2, UI::ShortcutParams().with_ctrl())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorVoxelMold_limitToPlane_XZ));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							limitToPlaneAxis = limitToPlaneAxis == 1 ? NONE : 1;
							update_ui_highlight();
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("yz")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::N3, UI::ShortcutParams().with_ctrl())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_id(NAME(editorVoxelMold_limitToPlane_YZ));
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							limitToPlaneAxis = limitToPlaneAxis == 0? NONE : 0;
							update_ui_highlight();
						});
					menu->add(b);
				}

				menu->place_content_horizontally(c);
			}
			UI::Utils::place_below(c, menu, REF_ below, 1.0f);
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

void Editor::VoxelMold::show_layers(bool _show, bool _forcePlaceAt)
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
			if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
			{
				float textScale = 0.7f;
				UI::Utils::ListWindow list;
				auto* window = list.step_1_create_window(c, textScale, NP);
				existing = window;
				window->set_title(TXT("layers"))->set_id(NAME(editorLayers));
				window->set_modal(false)->set_closable(true);
				window->set_on_moved([this](UI::CanvasWindow* _window) { store_windows_at(); });
				list.step_2_setup_list(NAME(editorLayers_layerList));
				list.step_3_add_button(TXT("+vox"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddVoxelLayer);
							}
						});
				list.step_3_add_button(TXT("+mod"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddModLayer);
							}
						});
				list.step_3_add_button(TXT("+mod full"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddFullModLayer);
							}
						});
				list.step_3_add_button(TXT("+include"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								do_operation(NONE, Operation::AddIncludeLayer);
							}
						});
				list.step_3_add_button(TXT("copy"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								do_operation(NONE, Operation::DuplicateLayer);
							}
						});
				list.step_3_add_button(TXT("del"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								do_operation(NONE, Operation::SwitchTemporariness);
							}
						});
				list.step_3_add_button(TXT("name"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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
									combine_voxels();
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
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								if (auto* l = CONTENT_ACCESS::access_edited_layer(vm))
								{
									l->set_visibility_only(! l->get_visibility_only(), vm);
									combine_voxels();
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
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::change_depth_of_edited_layer(vm, -1);
								combine_voxels();
								do_operation(NONE, Operation::ChangedDepthOfLayer);
							}
						});
				list.step_3_add_button(TXT("depth >"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::End, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::change_depth_of_edited_layer(vm, 1);
								combine_voxels();
								do_operation(NONE, Operation::ChangedDepthOfLayer);
							}
						});
				list.step_3_add_button(TXT("sel ^"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageUp, UI::ShortcutParams().with_shift())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::set_edited_layer_index(vm, CONTENT_ACCESS::get_edited_layer_index(vm) - 1);
								highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
								combine_voxels();
								populate_layer_list();
							}
						});
				list.step_3_add_button(TXT("sel v"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageDown, UI::ShortcutParams().with_shift())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::set_edited_layer_index(vm, CONTENT_ACCESS::get_edited_layer_index(vm) + 1);
								highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
								combine_voxels();
								populate_layer_list();
							}
						});
				list.step_3_add_button(TXT("mov ^"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageUp, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::move_edited_layer(vm, -1);
								combine_voxels();
								do_operation(NONE, Operation::MovedLayer);
							}
						});
				list.step_3_add_button(TXT("mov v"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageDown, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::merge_temporary_layers(vm);
								CONTENT_ACCESS::move_edited_layer(vm, 1);
								combine_voxels();
								do_operation(NONE, Operation::MovedLayer);
							}
						});
				list.step_3_add_button(TXT("merge 1"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::M, UI::ShortcutParams().with_shift().with_ctrl())
#endif
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								do_operation(NONE, Operation::MergeWithNext);
							}
						});
				list.step_3_add_button(TXT("merge children"))
					->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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

void Editor::VoxelMold::reshow_layer_props()
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

void Editor::VoxelMold::show_layer_props(bool _show, bool _forcePlaceAt)
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
			if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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
				if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
				{
					float textScale = 0.7f;
					{
						auto* panel = Framework::UI::CanvasWindow::get();
						window->add(panel);
						{
							auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("(voxel) transform"));
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
						for_count(int, i, 3)
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
								b->set_id(i == 0 ? NAME(editorLayerProps_offsetX) : (i == 1 ? NAME(editorLayerProps_offsetY) : NAME(editorLayerProps_offsetZ)));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
						for_count(int, i, 3)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(i == 0 ? TXT("pitch") : (i == 1 ? TXT("yaw") : TXT("roll"))));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("++0++")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_id(i == 0 ? NAME(editorLayerProps_rotatePitch) : (i == 1 ? NAME(editorLayerProps_rotateYaw) : NAME(editorLayerProps_rotateRoll)));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												auto& e = vtl->access_rotate().access_element(i);
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
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												auto& e = vtl->access_rotate().access_element(i);
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_transform_layer_gizmos();
												vtl->access_mirror().mirrorAxis = NONE;
												do_operation(NONE, Operation::ChangedLayerProps);
											}
										}
									});
								panel->add(b);
							}
							for_count(int, i, 3)
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(i == 0 ? TXT("x") : (i == 1 ? TXT("y") : TXT("z"))));
								b->set_id(i == 0 ? NAME(editorLayerProps_mirrorX) : (i == 1 ? NAME(editorLayerProps_mirrorY) : NAME(editorLayerProps_mirrorZ)));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
				else if (auto* vtl = fast_cast<Framework::VoxelMoldFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
						for_count(int, i, 3)
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
								b->set_id(i == 0 ? NAME(editorLayerProps_offsetX) : (i == 1 ? NAME(editorLayerProps_offsetY) : NAME(editorLayerProps_offsetZ)));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
						for_count(int, i, 3)
						{
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(i == 0 ? TXT("pitch") : (i == 1 ? TXT("yaw") : TXT("roll"))));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("++0++")));
								b->set_text_only();
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_id(i == 0 ? NAME(editorLayerProps_rotatePitch) : (i == 1 ? NAME(editorLayerProps_rotateYaw) : NAME(editorLayerProps_rotateRoll)));
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("-")));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_full_transform_layer_gizmos();
												float& c = vtl->access_rotate().access_element(i);
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
								b->set_on_press([this, i](Framework::UI::ActContext const& _context)
									{
										if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
										{
											if (auto* vtl = fast_cast<Framework::VoxelMoldFullTransformLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
											{
												remove_full_transform_layer_gizmos();
												float& c = vtl->access_rotate().access_element(i);
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
				else if (auto* ivml = fast_cast<Framework::IncludeVoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
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
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("included voxel mold"));
								b->set_id(NAME(editorLayerProps_includedVoxelMold));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_text_only();
								panel->add(b);
							}
							{
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("change included"));
								b->set_scale(textScale)->set_auto_width(c)->set_default_height(c);
								b->set_on_press([this](Framework::UI::ActContext const& _context)
									{
										UI::Utils::choose_library_stored<Framework::VoxelMold>(get_canvas(), NAME(chooseLibraryStored_includeVoxelMold), TXT("include voxel mold"), lastVoxelMoldIncluded.get(), 1.0f,
											[this](Framework::VoxelMold* _chosen)
											{
												if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
												{
													if (CONTENT_ACCESS::can_include(vm, _chosen))
													{
														if (auto* ivml = fast_cast<Framework::IncludeVoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
														{
															ivml->set_included(_chosen);
														}
														combine_voxels();
														populate_layer_list();
														update_ui_highlight();
													}
													else
													{
														UI::Utils::QuestionWindow::show_message(get_canvas(), String::printf(TXT("can't include voxel mold \"%S\""), _chosen->get_name().to_string().to_char()).to_char());
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
						if (auto* vml = fast_cast<Framework::VoxelMoldLayer>(CONTENT_ACCESS::access_edited_layer(vm)))
						{
							auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("voxel mold layer"));
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
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("+shift to fill in voxel"));
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
								auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("+ctrl to ignore invisible/force empty voxels"));
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

void Editor::VoxelMold::populate_layer_list()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	float textScale = layerListContext.textScale;

	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
	{
		int layerCount = CONTENT_ACCESS::get_layers(vm).get_size();
		layerListContext.selectedLayerIndex = clamp(CONTENT_ACCESS::get_edited_layer_index(vm), 0, layerCount - 1);
		UI::Utils::ListWindow::populate_list(canvas, NAME(editorLayers_layerList), layerCount, layerListContext.selectedLayerIndex, textScale,
			[this](int idx, UI::CanvasButton* b)
			{
				if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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

								if (auto* ivm = fast_cast<IncludeVoxelMoldLayer>(l))
								{
									name += TXT(" [included]");
								}
								if (auto* ivm = fast_cast<VoxelMoldTransformLayer>(l))
								{
									name += TXT(" [mod]");
								}
								if (auto* ivm = fast_cast<VoxelMoldFullTransformLayer>(l))
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
				if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
				{
					CONTENT_ACCESS::merge_temporary_layers(vm);
					CONTENT_ACCESS::set_edited_layer_index(vm, _idx);
					highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
					combine_voxels();
					populate_layer_list();
				}
			},
			[this](int _idx, void const* _userData)
			{
				layerListContext.selectedLayerIndex = _idx;
				if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
				{
					CONTENT_ACCESS::merge_temporary_layers(vm);
					CONTENT_ACCESS::set_edited_layer_index(vm, _idx);
					highlightEditedLayerFor = HIGHLIGHT_EDITED_LAYER_TIME;
					combine_voxels();
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

void Editor::VoxelMold::update_edited_layer_name()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_editedLayerName))))
	{
		String layerName;
		if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
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
	if (auto* w = fast_cast<Framework::UI::CanvasWindow>(c->find_by_id(NAME(editorVoxelMold_editedLayerNameWindow))))
	{
		Range2 wRange = w->get_placement(canvas);
		w->place_content_horizontally(canvas);
		Range2 wNewRange = w->get_placement(canvas);
		w->offset_placement_by(wRange.get_at(Vector2(0.0f, 1.0f)) - wNewRange.get_at(Vector2(0.0f, 1.0f)));
	}
}

void Editor::VoxelMold::update_ui_highlight()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	colourPaletteComponent.update_ui_highlight();

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_addRemove))))
	{
		b->set_highlighted(mode == Mode::AddRemove);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_add))))
	{
		b->set_highlighted(mode == Mode::Add);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_remove))))
	{
		b->set_highlighted(mode == Mode::Remove);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_fill))))
	{
		b->set_highlighted(mode == Mode::Fill);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_paint))))
	{
		b->set_highlighted(mode == Mode::Paint);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_paintFill))))
	{
		b->set_highlighted(mode == Mode::PaintFill);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_replaceColour))))
	{
		b->set_highlighted(mode == Mode::ReplaceColour);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_colourPicker))))
	{
		b->set_highlighted(mode == Mode::GetColour);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_select))))
	{
		b->set_highlighted(mode == Mode::Select);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_move))))
	{
		b->set_highlighted(mode == Mode::Move);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_drawEdges))))
	{
		b->set_highlighted(drawEdges);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_onlySolidVoxels))))
	{
		b->set_highlighted(onlySolidVoxels);
	}
	Framework::VoxelMoldTransformLayer* vtl = nullptr;
	Framework::VoxelMoldFullTransformLayer* vftl = nullptr;
	Framework::IncludeVoxelMoldLayer* ivml = nullptr;
	auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset());
	if (vm)
	{
		if (auto* l = CONTENT_ACCESS::access_edited_layer(vm))
		{
			vtl = fast_cast<Framework::VoxelMoldTransformLayer>(l);
			vftl = fast_cast<Framework::VoxelMoldFullTransformLayer>(l);
			ivml = fast_cast<Framework::IncludeVoxelMoldLayer>(l);
		}
	}

	// VoxelMoldTransformLayer / VoxelMoldFullTransformLayer
	{
		for_count(int, i, 3)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(i == 0 ? NAME(editorLayerProps_offsetX) : (i == 1 ? NAME(editorLayerProps_offsetY) : NAME(editorLayerProps_offsetZ)))))
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
		for_count(int, i, 3)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(i == 0 ? NAME(editorLayerProps_rotatePitch) : (i == 1 ? NAME(editorLayerProps_rotateYaw) : NAME(editorLayerProps_rotateRoll)))))
			{
				if (vtl)
				{
					int e = vtl->access_rotate().access_element(i);
					b->set_caption(String::printf(TXT("%c%i"), e == 0 ? ' ' : (e < 0 ? '-' : '+'), abs(e)));
				}
				else if (vftl)
				{
					float e = vftl->access_rotate().access_element(i);
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
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_mirrorZ))))
		{
			b->set_highlighted(vtl && vtl->access_mirror().mirrorAxis == 2);
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
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorLayerProps_includedVoxelMold))))
		{
			b->set_caption(ivml&& ivml->get_included() ? ivml->get_included()->get_name().to_string().to_char() : TXT("--"));
		}
	}

	{
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_props_voxelSize))))
		{
			b->set_caption(vm? String::printf(TXT("%.3f"), vm->get_voxel_size()).to_char() : TXT("--"));
		}
		for_count(int, i, 3)
		{
			if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(i == 0 ? NAME(editorVoxelMold_props_zeroOffsetX) : (i == 1 ? NAME(editorVoxelMold_props_zeroOffsetY) : NAME(editorVoxelMold_props_zeroOffsetZ)))))
			{
				b->set_caption(vm ? (vm->get_zero_offset().get_element(i) < 0.25f ? TXT("no") : TXT("centred")) : TXT("--"));
			}
		}
	}

	{
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_limitToPlane_XY))))
		{
			b->set_highlighted(limitToPlaneAxis == 2);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_limitToPlane_XZ))))
		{
			b->set_highlighted(limitToPlaneAxis == 1);
		}
		if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorVoxelMold_limitToPlane_YZ))))
		{
			b->set_highlighted(limitToPlaneAxis == 0);
		}		
	}

	base::update_ui_highlight();
}

void Editor::VoxelMold::show_asset_props()
{
	auto* c = get_canvas();

	if (auto* e = c->find_by_id(NAME(editorVoxelMold_props)))
	{
		e->remove();
		return;
	}

	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
	{
		UI::Utils::Grid2Menu menu;

		auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("voxel mold props"));
		window->set_id(NAME(editorVoxelMold_props));
		window->set_modal(false)->set_closable(true);
		add_common_props_to_asset_props(menu);
		colourPaletteComponent.add_to_asset_props(menu);
		menu.step_2_add_text_and_button(TXT("voxel size"), TXT("--"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::S)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
					{
						UI::Utils::edit_text(_context.canvas, TXT("voxel size"), String::printf(TXT("%.3f"), vm->get_voxel_size()), [this, vm](String const& _text)
							{
								float v = ParserUtils::parse_float(_text);
								if (v > 0.0f)
								{
									CONTENT_ACCESS::set_voxel_size(vm, v);
									do_operation(NONE, Operation::ChangedVoxelMoldProps);
								}
							});
					}
				})
			->set_id(NAME(editorVoxelMold_props_voxelSize));
		for_count(int, i, 3)
		{
			menu.step_2_add_text_and_button(String::printf(TXT("zero offset %c"), i == 0? 'x' : (i == 1? 'y' : 'z')).to_char(), TXT("--"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(i == 0? System::Key::X : (i== 1? System::Key::Y : System::Key::Z))
#endif
				->set_on_press([this, i](Framework::UI::ActContext const& _context)
					{
						if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
						{
							Vector3 zeroOffset = vm->get_zero_offset();
							float n = zeroOffset.get_element(i) < 0.25f ? 0.5f : 0.0f;
							zeroOffset.access_element(i) = n;
							CONTENT_ACCESS::set_zero_offset(vm, zeroOffset);
							do_operation(NONE, Operation::ChangedVoxelMoldProps);
						}
					})
				->set_id(i == 0? NAME(editorVoxelMold_props_zeroOffsetX) : (i == 1? NAME(editorVoxelMold_props_zeroOffsetY) : NAME(editorVoxelMold_props_zeroOffsetZ)));
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

void Editor::VoxelMold::show_rescale_selection()
{
	auto* c = get_canvas();

	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
	{
		UI::Utils::Grid2Menu menu;

		auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("rescale"));
		window->set_closable(true);
		menu.step_2_add_text_and_button(TXT("voxel size"), String::printf(TXT("%.3f"), vm->get_voxel_size()).to_char())
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
				menu.step_2_add_text_and_button(TXT("more voxels"), String::printf(TXT("x%i"), s).to_char())
					->set_on_press([this, window, s](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::rescale_more_voxels(vm, s);
								combine_voxels();
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
				menu.step_2_add_text_and_button(TXT("less voxels"), String::printf(TXT("x%i"), s).to_char())
					->set_on_press([this, window, s](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
							{
								CONTENT_ACCESS::rescale_less_voxels(vm, s);
								combine_voxels();
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

void Editor::VoxelMold::combine_voxels()
{
	if (auto* vm = fast_cast<Framework::VoxelMold>(get_edited_asset()))
	{
		CONTENT_ACCESS::combine_voxels(vm);
	}
}

void Editor::VoxelMold::remove_transform_layer_gizmos()
{
	transformLayerGizmos = TransformLayerGizmos(); // will clear all
}

void Editor::VoxelMold::remove_full_transform_layer_gizmos()
{
	fullTransformLayerGizmos = FullTransformLayerGizmos(); // will clear all
}

void Editor::VoxelMold::remove_voxel_layer_gizmos()
{
	voxelLayerGizmos = VoxelLayerGizmos(); // will clear all
}

int Editor::VoxelMold::get_selected_colour_index() const
{
	return colourPaletteComponent.get_selected_colour_index();
}

void Editor::VoxelMold::set_selected_colour_index(int _idx)
{
	colourPaletteComponent.set_selected_colour_index(_idx);
}
