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
		class SpriteGrid
		: public Base2D
		{
			FAST_CAST_DECLARE(SpriteGrid);
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
					GetSprite,

					MAX
				};

				static tchar const* to_char(Type _mode)
				{
					if (_mode == AddRemove) return TXT("addRemove");
					if (_mode == Add) return TXT("add");
					if (_mode == Remove) return TXT("remove");
					if (_mode == GetSprite) return TXT("getSprite");
					// default
					return TXT("addRemove");
				}
			};

		public:
			SpriteGrid();
			virtual ~SpriteGrid();

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
				Optional<VectorInt2> gridCoord;
				bool hitNode = false;
				Optional<Vector2> rayStart;
			};
			Array<CursorRay> cursorRays;

			void update_cursor_ray(int _cursorIdx, Vector2 const& _rayStart);

			Mode::Type mode = Mode::AddRemove;
			
			enum Operation
			{
				AddNode,
				RemoveNode,
				GetSpriteFromNode,

				ChangedSpriteGridProps,
				ChangedSpritePalette,
			};
			Optional<VectorInt2> lastOperationCell;
			void do_operation(int _cursorIdx, Operation _operation, bool _checkLastOperationCell = false);

			int selectedSpriteIdx = 0;
			bool swappingPalette = false;

			UsedLibraryStored<Framework::Sprite> lastSpriteAdded;

			struct SpriteShortCut
			{
				tchar keyVisible;
				int keyCode;
				int selectedSpriteIdx = NONE;
			};
			Array<SpriteShortCut> spriteShortCuts;

			struct WindowLayout
			{
				bool open = false;
				bool atValid = false;
				Vector2 at = Vector2::zero;
			};
			struct Windows
			{
				WindowLayout palette;
			} windows;

		protected:
			void reshow_palette();
			void show_palette(bool _show, bool _forcePlaceAt = false);

			void restore_windows(bool _forcePlaceAt);
			void store_windows_at();

			void setup_palette_button(Framework::UI::CanvasButton* b, int spriteIdx);

			void scroll_to_show_selected_sprite_on_palette();
		};
	};
};

DECLARE_REGISTERED_TYPE(Framework::Editor::SpriteGrid::Mode::Type);