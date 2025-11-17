#pragma once

#include "..\..\..\..\core\math\math.h"

//

class SimpleVariableStorage;

namespace Framework
{
	class ColourPalette;

	namespace UI
	{
		struct CanvasButton;
		namespace Utils
		{
			struct Grid2Menu;
		};
	};

	namespace Editor
	{
		class Base;

		namespace Component
		{
			struct ColourPalette
			{
			public:
				ColourPalette(Editor::Base* _editor);

				int get_selected_colour_index() const { return selectedColourIdx; }
				void set_selected_colour_index(int _idx);

				void reshow_palette();
				void show_palette(bool _show, bool _forcePlaceAt = false);
				void show_or_hide_palette();

				void add_to_asset_props(UI::Utils::Grid2Menu& menu);

			public:
				void process_controls();

				void update_ui_highlight();

				void restore_for_use(SimpleVariableStorage const& _setup);
				void store_for_later(SimpleVariableStorage& _setup) const;

				void restore_windows(bool _forcePlaceAt);
				void store_windows_at();

			private:
				Editor::Base* editor = nullptr;

				void set_colour_palette(Framework::ColourPalette* _palette, bool _convertToNewOne) const;
				Framework::ColourPalette const * get_colour_palette() const;

				int selectedColourIdx = 0;
				int uiColourCount = 0; // as added to ui

				struct ColourShortCut
				{
					tchar keyVisible;
					int keyCode;
					int selectedColourIdx = NONE;
				};
				Array<ColourShortCut> colourShortCuts;

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
				void setup_palette_colour_button(Framework::UI::CanvasButton* b, int colourIdx);
			};
		};
	};
};
