#pragma once

#include "editorBase3D.h"
#include "components/ec_colourPalette.h"

//

namespace Framework
{
	class VoxelMold;

	namespace UI
	{
		struct CanvasButton;
	};

	namespace Editor
	{
		class VoxelMold
		: public Base3D
		{
			FAST_CAST_DECLARE(VoxelMold);
			FAST_CAST_BASE(Base3D);
			FAST_CAST_END();

			typedef Base3D base;

		public:
			struct Mode
			{
				enum Type
				{
					AddRemove,
					Add,
					Remove,
					Fill,
					Paint,
					PaintFill,
					ReplaceColour,
					GetColour,
					Select,
					Move,

					MAX
				};

				static tchar const* to_char(Type _mode)
				{
					if (_mode == AddRemove) return TXT("addRemove");
					if (_mode == Add) return TXT("add");
					if (_mode == Remove) return TXT("remove");
					if (_mode == Fill) return TXT("fill");
					if (_mode == Paint) return TXT("paint");
					if (_mode == PaintFill) return TXT("paintFill");
					if (_mode == ReplaceColour) return TXT("replaceColour");
					if (_mode == GetColour) return TXT("getColour");
					if (_mode == Select) return TXT("select");
					if (_mode == Move) return TXT("move");
					// default
					return TXT("addRemove");
				}
			};

		public:
			VoxelMold();
			virtual ~VoxelMold();

		protected: // Base3D
			override_ void on_hover(int _cursorIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);
			override_ void on_press(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);
			override_ void on_click(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);
			override_ bool on_hold(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);
			override_ bool on_held(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir, Vector3 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised);
			override_ void on_release(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);

			bool on_hold_held(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);

		protected:
			override_ void render_edited_asset();

		protected: // Base3D, Base, (ui)
			override_ void setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale);
			override_ void setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale);

		public: // Base
			override_ void edit(LibraryStored* _asset, String const& _filePathToSave);

			override_ void restore_for_use(SimpleVariableStorage const& _setup);
			override_ void store_for_later(SimpleVariableStorage& _setup) const;

		public: // Base
			override_ void process_controls();

			override_ void render_debug(CustomRenderContext* _customRC);

			override_ void setup_ui(REF_ SetupUIContext& _setupUIContext);
			override_ void update_ui_highlight();

			override_ void show_asset_props();

		protected:
			struct CursorRay
			{
				// per cursor
				bool hitVoxel = false;
				Optional<VectorInt3> cellCoord;
				Optional<VectorInt3> hitNormal;
				Optional<Vector3> rayStart;
				Optional<Vector3> rayDir;
				Optional<bool> onlyActualContent;
			};
			Array<CursorRay> cursorRays;

			void update_cursor_ray(int _cursorIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);

			Mode::Type mode = Mode::AddRemove;
			int limitToPlaneAxis = NONE;
			Vector3 limitToPlaneOrigin = Vector3::zero;
			RefCountObjectPtr<Gizmo3D> limitToPlaneGizmo;
			
			bool drawEdges = false;
			bool onlySolidVoxels = false;

			float highlightEditedLayerFor = 0.0f;

			struct TransformLayerGizmos
			{
				void const * forLayer = nullptr; // should not be accessed directly! just reference
				Gizmo3DThreeAxes offsetGizmo;
				int mirrorAtGizmoForAxis = NONE;
				RefCountObjectPtr<Gizmo3D> mirrorAtGizmo;
			} transformLayerGizmos;
			void remove_transform_layer_gizmos();

			struct FullTransformLayerGizmos
			{
				void const * forLayer = nullptr; // should not be accessed directly! just reference
				Gizmo3DThreeAxesAndRotation offsetAndRotationGizmo;
			} fullTransformLayerGizmos;
			void remove_full_transform_layer_gizmos();

			struct VoxelLayerGizmos
			{
				void const * forLayer = nullptr; // should not be accessed directly! just reference
				bool allowMakeCopy = true;
				Range3 forSelectionRange = Range3::empty;
				Vector3 zeroOffsetAt = Vector3::zero; // this can move, then everything moves, we need to know where relatively we are
				Gizmo3DThreeAxes offsetGizmo;
			} voxelLayerGizmos;
			void remove_voxel_layer_gizmos();

			struct Selection
			{
				Optional<VectorInt3> startedAt;
				Optional<VectorInt3> draggingTo;
				Range3 range = Range3::empty; // global
				Range3 gizmosOffDistance = Range3::zero;
				struct Gizmos
				{
					RefCountObjectPtr<Gizmo3D> rangeXMinGizmo;
					RefCountObjectPtr<Gizmo3D> rangeXMaxGizmo;
					RefCountObjectPtr<Gizmo3D> rangeYMinGizmo;
					RefCountObjectPtr<Gizmo3D> rangeYMaxGizmo;
					RefCountObjectPtr<Gizmo3D> rangeZMinGizmo;
					RefCountObjectPtr<Gizmo3D> rangeZMaxGizmo;
				} gizmos;
			} selection;

			enum Operation
			{
				AddVoxel,
				FillAtVoxel,
				FillRemoveAtVoxel,
				RemoveVoxel,
				PaintVoxel,
				PaintFillVoxel,
				ReplaceColour,
				GetColourFromVoxel,

				FillSelection,
				PaintSelection,
				ClearSelection,
				DuplicateSelectionToNewLayer,
				MoveSelectionToNewLayer,
				
				AddVoxelLayer,
				AddModLayer,
				AddFullModLayer,
				AddIncludeLayer,
				DuplicateLayer,
				SwitchTemporariness,
				RemoveLayer,
				RenamedLayer,
				MovedLayer,
				ChangedDepthOfLayer,
				ChangedLayerVisibility,
				ChangedLayerProps,
				ChangedVoxelMoldProps,
				MergeWithNext,
				MergeWithChildren,
			};
			Optional<VectorInt3> lastOperationCell;
			void do_operation(int _cursorIdx, Operation _operation, bool _checkLastOperationCell = false);

			UsedLibraryStored<Framework::VoxelMold> lastVoxelMoldIncluded;

			Component::ColourPalette colourPaletteComponent;

			struct WindowLayout
			{
				bool open = false;
				bool atValid = false;
				Vector2 at = Vector2::zero;
			};
			struct Windows
			{
				WindowLayout layers;
				WindowLayout layerProps;
				void const* layerPropsForLayer = nullptr;
				Mode::Type layerPropsForMode = Mode::AddRemove;
			} windows;

			struct LayerListContext
			{
				int selectedLayerIndex = NONE;
				float textScale = 2.0f;
			} layerListContext;

		protected:
			void show_layers(bool _show, bool _forcePlaceAt = false);
			void reshow_layer_props();
			void show_layer_props(bool _show, bool _forcePlaceAt = false);

			void restore_windows(bool _forcePlaceAt);
			void store_windows_at();

			void populate_layer_list();

			void update_edited_layer_name();

			void show_rescale_selection();

			void combine_voxels();

			int get_selected_colour_index() const;
			void set_selected_colour_index(int _idx);
		};
	};
};

DECLARE_REGISTERED_TYPE(Framework::Editor::VoxelMold::Mode::Type);