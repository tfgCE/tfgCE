#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\optional.h"

#include <functional>

namespace Framework
{
	namespace UI
	{
		interface_class ICanvasElement;
		class Canvas;
		struct CanvasButton;
		struct CanvasWindow;

		namespace Utils
		{
			template <typename Class>
			void choose_library_stored(Canvas* _canvas, Optional<Name> const& _id, tchar const* _title, Class const * _currentlySelected, Optional<float> const& _textScale, std::function<void(Class* _chosen)> _onChosen);

			struct ListWindow
			{
				CanvasWindow* step_1_create_window(Canvas * _canvas, Optional<float> const& _textScale = NP, Optional<Vector2> const& _spacing = NP);
				CanvasWindow* step_2_setup_list(Name const & _listID); // returns list "panel" window
				CanvasButton* step_3_add_button(tchar const * _buttonText); // multiple steps
				CanvasWindow* step_4_finalise_force_size(Vector2 const & _size, Optional<VectorInt2> const & _buttonGrid = NP);

			public:
				static void populate_list(Canvas* _canvas, Name const & _listID, int _count, Optional<int> const& _selectedIdx, Optional<float> const& _textScale,
					std::function<void(int _idx, CanvasButton* _element)> _setupElement,
					std::function<void(int _idx, void const * _userData)> _onSelect = nullptr,
					std::function<void(int _idx, void const * _userData)> _onPress = nullptr, // defaults to _onSelect
					std::function<void(int _idx, void const * _userData)> _onDoubleClick = nullptr);
				static void highlight_list_element(Canvas* _canvas, Name const& _listID, int _idx);
				static int get_highlighted_list_element_index(Canvas* _canvas, Name const& _listID);
				static void const * get_highlighted_list_element_user_data(Canvas* _canvas, Name const& _listID);

			private:
				Canvas* canvas = nullptr;
				CanvasWindow* window = nullptr;
				CanvasWindow* listPanel = nullptr;
				CanvasWindow* bottomPanel = nullptr;
				float textScale = 1.0f;
				Vector2 spacing;

				void place_everything_inside(Optional<VectorInt2> const& _buttonGrid);
			};
		};
	};
};
