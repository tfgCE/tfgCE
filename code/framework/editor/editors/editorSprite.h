#pragma once

#include "editorBase2D.h"
#include "components/ec_colourPalette.h"

//

namespace Framework
{
	class Sprite;

	namespace UI
	{
		struct CanvasButton;
	};

	namespace Editor
	{
		class Sprite
		: public Base2D
		{
			FAST_CAST_DECLARE(Sprite);
			FAST_CAST_BASE(Base2D);
			FAST_CAST_END();

			typedef Base2D base;

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
			Sprite();
			virtual ~Sprite();

		protected: // Base2D
			override_ void on_hover(int _cursorIdx, Vector2 const& _rayStart);
			override_ void on_press(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart);
			override_ void on_click(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart);
			override_ bool on_hold(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart);
			override_ bool on_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart, Vector2 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised);
			override_ void on_release(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart);

			bool on_hold_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart);

		protected:
			override_ void render_edited_asset();

		protected: // Base2D, Base, (ui)
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

		protected: // Base
			override_ Range2 get_grid_range() const;
			override_ Vector2 get_grid_step() const;
			override_ Vector2 get_grid_zero_at() const;
			override_ float get_grid_alpha(float _v, float _step) const;

		protected:
			struct CursorRay
			{
				// per cursor
				bool hitPixel = false;
				Optional<VectorInt2> cellCoord;
				Optional<Vector2> rayStart;
				Optional<bool> onlyActualContent;
			};
			Array<CursorRay> cursorRays;

			void update_cursor_ray(int _cursorIdx, Vector2 const& _rayStart);

			Mode::Type mode = Mode::AddRemove;
			
			bool drawEdges = false;
			bool onlySolidPixels = false;

			float highlightEditedLayerFor = 0.0f;

			struct TransformLayerGizmos
			{
				void const * forLayer = nullptr; // should not be accessed directly! just reference
				Gizmo2DTwoAxes offsetGizmo;
				int mirrorAtGizmoForAxis = NONE;
				RefCountObjectPtr<Gizmo2D> mirrorAtGizmo;
			} transformLayerGizmos;
			void remove_transform_layer_gizmos();

			struct FullTransformLayerGizmos
			{
				void const * forLayer = nullptr; // should not be accessed directly! just reference
				Gizmo2DTwoAxesAndRotation offsetAndRotationGizmo;
			} fullTransformLayerGizmos;
			void remove_full_transform_layer_gizmos();

			struct PixelLayerGizmos
			{
				void const * forLayer = nullptr; // should not be accessed directly! just reference
				bool allowMakeCopy = true;
				Range2 forSelectionRange = Range2::empty;
				Vector2 zeroOffsetAt = Vector2::zero; // this can move, then everything moves, we need to know where relatively we are
				Gizmo2DTwoAxes offsetGizmo;
			} pixelLayerGizmos;
			void remove_pixel_layer_gizmos();

			struct Selection
			{
				Optional<VectorInt2> startedAt;
				Optional<VectorInt2> draggingTo;
				Range2 range = Range2::empty; // global
				Range2 gizmosOffDistance = Range2::zero;
				struct Gizmos
				{
					RefCountObjectPtr<Gizmo2D> rangeXMinGizmo;
					RefCountObjectPtr<Gizmo2D> rangeXMaxGizmo;
					RefCountObjectPtr<Gizmo2D> rangeYMinGizmo;
					RefCountObjectPtr<Gizmo2D> rangeYMaxGizmo;
				} gizmos;
			} selection;

			enum Operation
			{
				AddPixel,
				FillAtPixel,
				FillRemoveAtPixel,
				RemovePixel,
				PaintPixel,
				PaintFillPixel,
				ReplaceColour,
				GetColourFromPixel,

				FillSelection,
				PaintSelection,
				ClearSelection,
				DuplicateSelectionToNewLayer,
				MoveSelectionToNewLayer,
				
				AddPixelLayer,
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
				ChangedSpriteProps,
				MergeWithNext,
				MergeWithChildren,
			};
			Optional<VectorInt2> lastOperationCell;
			void do_operation(int _cursorIdx, Operation _operation, bool _checkLastOperationCell = false);

			UsedLibraryStored<Framework::Sprite> lastSpriteIncluded;

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

			void combine_pixels();

			int get_selected_colour_index() const;
			void set_selected_colour_index(int _idx);
		};
	};
};

DECLARE_REGISTERED_TYPE(Framework::Editor::Sprite::Mode::Type);