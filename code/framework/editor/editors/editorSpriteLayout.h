#pragma once

#include "editorBase2D.h"

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
		class SpriteLayout
		: public Base2D
		{
			FAST_CAST_DECLARE(SpriteLayout);
			FAST_CAST_BASE(Base2D);
			FAST_CAST_END();

			typedef Base2D base;

		public:
			SpriteLayout();
			virtual ~SpriteLayout();

		protected:
			override_ void render_edited_asset();

		protected: // Base2D
			override_ bool on_hold(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart);
			override_ bool on_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt, Vector2 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised);

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

		protected: // Base2D, Base, (ui)
			override_ void setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale);

		protected:
			struct CursorRay
			{
				// per cursor
				bool hitPixel = false;
				Optional<int> entryIdx;
				Optional<VectorInt2> cellCoord;
				Optional<Vector2> rayStart;

				Vector2 movedByAccumulated = Vector2::zero;
			};
			Array<CursorRay> cursorRays;

			void update_cursor_ray(int _cursorIdx, Vector2 const& _rayStart);

			float highlightEditedEntryFor = 0.0f;

			struct TransformGizmos
			{
				void const * forEntry = nullptr; // should not be accessed directly! just reference
				Gizmo2DTwoAxes offsetGizmo;
			} transformGizmos;
			void remove_transform_gizmos();

			enum Operation
			{
				AddEntry,
				DuplicateEntry,
				RemoveEntry,
				MovedEntry,

				ChangedEntry,
				ChangedEntryContent,
			};
			void do_operation(Operation _operation);

		protected:
			UsedLibraryStored<Framework::Sprite> lastSpriteIncluded;
		};
	};
};
