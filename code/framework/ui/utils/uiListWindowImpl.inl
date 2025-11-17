#include "..\..\library\library.h"

//

DEFINE_STATIC_NAME(chooseLibraryStored_list);

//

template <typename Class>
void Framework::UI::Utils::choose_library_stored(Canvas* _canvas, Optional<Name> const & _id, tchar const* _title, Class const * _currentlySelected, Optional<float> const& _textScale, std::function<void(Class* _chosen)> _onChosen)
{
	Name id = _id.get(NAME(chooseLibraryStored_list));
	UI::Utils::ListWindow list;
	auto* window = list.step_1_create_window(_canvas, _textScale, NP)->set_title(_title);
	list.step_2_setup_list(id);
	list.step_3_add_button(TXT("select"))
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::Return)
#endif
		->set_on_press([_onChosen, window, id](Framework::UI::ActContext const& _context)
			{
				if (_onChosen)
				{
					void const* userData = Framework::UI::Utils::ListWindow::get_highlighted_list_element_user_data(_context.canvas, id);
					Class* ls = fast_cast<Class>((LibraryStored*)cast_to_nonconst(userData));
					_onChosen(ls);
				}
				window->remove();
			});
	list.step_3_add_button(TXT("back"))
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::Esc)
#endif
		->set_on_press([window](Framework::UI::ActContext const& _context)
			{
				window->remove();
			});

	list.step_4_finalise_force_size(_canvas->get_logical_size() * 0.75f);

	Array<Class const *> libraryStored;
	if (auto* store = Framework::Library::get_current()->get_store(Class::library_type()))
	{
		store->do_for_every([&libraryStored](LibraryStored* _libraryStored)
		{
			if (auto* ls = fast_cast<Class>(_libraryStored))
			{
				libraryStored.push_back(ls);
			}
		});
	}
	sort(libraryStored, LibraryStored::compare_ptr);
	Optional<int> selectedIdx;
	int found = libraryStored.find_index(_currentlySelected);
	if (found != NONE)
	{
		selectedIdx = found;
	}
	UI::Utils::ListWindow::populate_list(_canvas, id, libraryStored.get_size(), selectedIdx, _textScale,
		[libraryStored](int idx, UI::CanvasButton* b)
		{
			if (libraryStored.is_index_valid(idx))
			{
				auto* ls = libraryStored[idx];
				b->set_caption(ls->get_name().to_string());
				b->set_user_data(ls);
				return;
			}

			b->set_caption(TXT("??"));
		},
		nullptr,
		nullptr,
		[_onChosen, window](int _idx, void const* _userData)
		{
			if (_onChosen)
			{
				Class* ls = fast_cast<Class>((LibraryStored*)cast_to_nonconst(_userData));
				_onChosen(ls);
			}
			window->remove();
		});
}
