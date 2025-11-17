#include "editorContext.h"

#include "editorWorkspace.h"

#include "editors\editorBase.h"
#include "editors\editorBase3D.h"

#include "..\game\game.h"

#include "..\library\library.h"
#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStoredAny.h"

#include "..\ui\uiCanvas.h"
#include "..\ui\uiCanvasButton.h"
#include "..\ui\uiCanvasText.h"
#include "..\ui\uiCanvasWindow.h"
#include "..\ui\utils\uiGrid1Menu.h"
#include "..\ui\utils\uiGrid2Menu.h"
#include "..\ui\utils\uiListWindow.h"
#include "..\ui\utils\uiQuestionWindow.h"
#include "..\ui\utils\uiTextEditWindow.h"

#include "..\..\core\io\dir.h"
#include "..\..\core\io\ioDialogs.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define WORKSPACE_DIRECTORY TXT("_editor")

#define DEFAULT_CANVAS_LOGICAL_SIZE Vector2(1000.0f, 800.0f)

//

using namespace Framework;

//

// ui
DEFINE_STATIC_NAME(editor_assetList);
DEFINE_STATIC_NAME(editor_assetType);

//

EditorContext::EditorContext()
{
	canvas = new UI::Canvas();
	canvasLogicalSize = DEFAULT_CANVAS_LOGICAL_SIZE;
	canvas->set_logical_size(canvasLogicalSize);

	EditorWorkspace::provide_available_asset_types(OUT_ availableAssetTypes);
}

EditorContext::~EditorContext()
{
	delete_and_clear(canvas);
}

void EditorContext::refresh_canvas_for_current_state(Optional<State> const& _onlyIfState)
{
	if (state == ShowEditor &&
		state == _onlyIfState.get(state))
	{
		if (editor.get())
		{
			Editor::SetupUIContext setupUIContext;
			editor->setup_ui(REF_ setupUIContext);
			editor->update_ui_highlight();
		}
	}
	if (state == ShowMenu &&
		state == _onlyIfState.get(state))
	{
		open_menu(false);
	}
	if (state == ShowAssetList &&
		state == _onlyIfState.get(state))
	{
		open_asset_list();
	}
	if (state == ShowAssetManager &&
		state == _onlyIfState.get(state))
	{
		open_asset_manager();
	}
	if (state == ShowAddNewAsset &&
		state == _onlyIfState.get(state))
	{
		open_add_new_asset();
	}
	if (state == ShowCopyAsset &&
		state == _onlyIfState.get(state))
	{
		open_copy_asset();
	}
}

void EditorContext::update_canvas_real_size(Vector2 const& _realSize)
{
	Concurrency::ScopedSpinLock lock(canvas->access_lock(), true);
	canvas->set_real_size(_realSize);
	Vector2 useLogicalSize = canvasLogicalSize;
	float aspectRatio = aspect_ratio(_realSize);
	useLogicalSize.x = useLogicalSize.y * aspectRatio;
	if (canvasLogicalSize != useLogicalSize)
	{
		canvas->set_logical_size(useLogicalSize);
		canvasLogicalSize = useLogicalSize;
		refresh_canvas_for_current_state(ShowEditor);
		if (state == ShowMenu)
		{
			open_menu(false);
		}
	}
}

void EditorContext::update_canvas_logical_size(Optional<Vector2> const& _logicalSize)
{
	Vector2 logicalSize = _logicalSize.get(DEFAULT_CANVAS_LOGICAL_SIZE);
	if (canvasLogicalSize != logicalSize)
	{
		canvasLogicalSize = logicalSize;
		// use well defined path
		update_canvas_real_size(canvas->get_real_size());
	}
}

void EditorContext::load_last_workspace_or_default()
{
	{
		IO::File file;
		file.open(String::printf(TXT("%S%S_last"), WORKSPACE_DIRECTORY, IO::get_directory_separator()));
		if (file.is_open())
		{
			file.set_type(IO::InterfaceType::Text);
			List<String> lines;
			file.read_lines(lines);
			if (!lines.is_empty())
			{
				String filePathToLastWorkspace = lines.get_first();
				if (load_workspace(filePathToLastWorkspace))
				{
					return;
				}
			}
		}
	}

	// check any xml and load first
	{
		IO::Dir dir;
		if (dir.list(String(WORKSPACE_DIRECTORY)))
		{
			for_every(f, dir.get_files())
			{
				if (String::compare_icase(f->get_right(4), TXT(".xml")))
				{
					if (load_workspace(String::printf(TXT("%S%S%S"), WORKSPACE_DIRECTORY, IO::get_directory_separator(), f->to_char())))
					{
						return;
					}
				}
			}
		}
	}

	// create default
	{
		String filePath = String::printf(TXT("%S%S_default.xml"), WORKSPACE_DIRECTORY, IO::get_directory_separator());
		switch_to_workspace(new EditorWorkspace(), filePath);
		if (workspace.get())
		{
			workspace->set_synchronised_with_library(true);
		}
		save_workspace();
	}
}

void EditorContext::edit_current_asset()
{
	if (auto* w = workspace.get())
	{
		int idx = w->get_edited_asset_index();
		if (w->get_assets().is_index_valid(idx))
		{
			auto& asset = w->get_assets()[idx];
			edit(asset.libraryStored.get(), asset.get_file_path_to_save());
		}
	}
}

void EditorContext::change_asset(int _by)
{
	if (auto* w = workspace.get())
	{
		int idx = w->get_edited_asset_index();
		int amount = w->get_assets().get_size();
		idx = amount > 0? mod(idx + _by, amount) : 0;
		w->set_edited_asset_index(idx);
		edit_current_asset();
	}
}

void EditorContext::edit(LibraryStored* _asset, String const& _filePathToSave, bool _storeForLater)
{
	// select new editor
	{
		Framework::Editor::Base* newEditor = nullptr;
		if (_asset)
		{
			newEditor = _asset->create_editor(editor.get());
		}
		switch_to_editor(newEditor, _storeForLater);
	}
	if (editor.get())
	{
		editor->set_editor_context(this);
		editor->edit(_asset, _filePathToSave);
		refresh_canvas_for_current_state(ShowEditor);
	}
}

void EditorContext::switch_to_editor(Framework::Editor::Base* _editor, bool _storeForLater)
{
	// always keep values on switch
	SimpleVariableStorage variables;
	SimpleVariableStorage& useVariables = workspace.get() ? workspace->access_variables() : variables;
	if (editor.get() && _storeForLater)
	{
		editor->store_for_later(REF_ useVariables);
	}
	if (editor.get() && editor == _editor && _editor)
	{
		// just keep using it
		return;
	}
	if (!_editor)
	{
		_editor = new Framework::Editor::Base3D();
	}
	editor = _editor;
	if (editor.get())
	{
		editor->set_editor_context(this);
		editor->restore_for_use(REF_ useVariables);
		refresh_canvas_for_current_state(ShowEditor);
	}
}

void EditorContext::switch_to_workspace(EditorWorkspace* _workspace, String const& _filePathToSave)
{
	RefCountObjectPtr<EditorWorkspace> prevWorkspace = workspace; // keep it till we switch editor

	save_workspace();

	bool opened = false;

	workspace = _workspace;
	filePathToSaveWorkspace = _filePathToSave;
	if (auto* w = workspace.get())
	{
		if (!w->get_assets().is_empty())
		{
			int idx = w->get_edited_asset_index();
			idx = clamp(idx, 0, w->get_assets().get_size() - 1);

			auto& asset = w->access_assets()[idx];
			if (asset.libraryStored.find(Library::get_current()))
			{
				edit(asset.libraryStored.get(), asset.get_file_path_to_save(), false); // don't store the existing editor for later
				if (editor.get())
				{
					// force setup/restore editor for current workspace
					editor->set_editor_context(this);
					editor->restore_for_use(w->access_variables());
					refresh_canvas_for_current_state(ShowEditor);
				}
				opened = true;
			}
		}
	}

	if (!opened)
	{
		edit(nullptr, String::empty(), false); // don't store the existing editor for later
	}
}

void EditorContext::close_menu()
{
	canvas->clear();
	state = ShowEditor;
	edit_current_asset();
	refresh_canvas_for_current_state();
}

void EditorContext::save_workspace()
{
	if (workspace.get() && !filePathToSaveWorkspace.is_empty())
	{
		if (editor.get())
		{
			editor->store_for_later(REF_ workspace->access_variables());
		}
		{
			IO::File file;
			file.create(filePathToSaveWorkspace);
			if (file.is_open())
			{
				file.set_type(IO::InterfaceType::Text);
				workspace->save_to(&file);
			}
			file.close();
		}
		{
			IO::File file;
			file.create(String::printf(TXT("%S%S_last"), WORKSPACE_DIRECTORY, IO::get_directory_separator()));
			if (file.is_open())
			{
				file.set_type(IO::InterfaceType::Text);
				file.write_text(filePathToSaveWorkspace);
			}
		}
	}
}

bool EditorContext::load_workspace(String const& _filePath)
{
	bool result = false;
	if (! _filePath.is_empty())
	{
		IO::File file;
		file.open(_filePath);
		if (file.is_open())
		{
			file.set_type(IO::InterfaceType::Text);
			RefCountObjectPtr<EditorWorkspace> w;
			w = new EditorWorkspace();
			if (w->load_from(&file))
			{
				w->update_all();
				switch_to_workspace(w.get(), _filePath);
				result = true;
			}
			else
			{
				switch_to_workspace(nullptr, String::empty());
			}
		}
		file.close();
	}
	return result;
}

void EditorContext::show_busy_canvas()
{
	canvas->clear();

	{
		auto* menu = new UI::CanvasWindow();
		menu->set_closable(false)->set_movable(false)->set_modal(true);
		canvas->add(menu);

		{
			auto* t = new UI::CanvasText();
			t->set_text(String(TXT("busy")));
			menu->add(t);
		}

		menu->place_content_on_grid(canvas);
		menu->set_at_pt(canvas, Vector2::half);
	}
}

void EditorContext::new_workspace_setup()
{
	show_busy_canvas();

	IO::Dir::create(String(WORKSPACE_DIRECTORY));

	IO::Dialogs::Params params;
	IO::Dialogs::setup_for_xml(params);
	params.startInDirectory = String::printf(TXT("%S%S%S"), IO::get_main_directory().to_char(), IO::get_directory_separator(), WORKSPACE_DIRECTORY);
	auto fileToSave = IO::Dialogs::get_file_to_save(params);
	if (fileToSave.is_set())
	{
		fileToSave = IO::Utils::make_relative(fileToSave.get());
		switch_to_workspace(new EditorWorkspace(), fileToSave.get());
		if (workspace.get())
		{
			workspace->set_synchronised_with_library(true);
		}
		save_workspace();
		open_asset_manager();
	}
	else
	{
		open_menu(false);
	}
}

void EditorContext::load_workspace_setup()
{
	show_busy_canvas();

	IO::Dialogs::Params params;
	IO::Dialogs::setup_for_xml(params);
	params.startInDirectory = String::printf(TXT("%S%S%S"), IO::get_main_directory().to_char(), IO::get_directory_separator(), WORKSPACE_DIRECTORY);
	auto fileToLoad = IO::Dialogs::get_file_to_open(params);
	if (fileToLoad.is_set())
	{
		fileToLoad = IO::Utils::make_relative(fileToLoad.get());
		load_workspace(fileToLoad.get());
		open_asset_manager();
	}
	else
	{
		open_menu(false);
	}
}

void EditorContext::copy_workspace_setup()
{
	show_busy_canvas();

	IO::Dir::create(String(WORKSPACE_DIRECTORY));

	IO::Dialogs::Params params;
	IO::Dialogs::setup_for_xml(params);
	params.startInDirectory = String::printf(TXT("%S%S%S"), IO::get_main_directory().to_char(), IO::get_directory_separator(), WORKSPACE_DIRECTORY);
	auto fileToSave = IO::Dialogs::get_file_to_save(params);
	if (fileToSave.is_set())
	{
		fileToSave = IO::Utils::make_relative(fileToSave.get());
		filePathToSaveWorkspace = fileToSave.get();
		save_workspace();
		open_asset_manager();
	}
	else
	{
		open_menu(false);
	}
}

void EditorContext::open_menu(bool _saveWorkspace)
{
	state = ShowMenu;

	canvas->clear();

	if (_saveWorkspace)
	{
		save_workspace();
	}

	{
		UI::Utils::Grid1Menu menu;

		float menuButtonScale = 2.0f;
		menu.step_1_create_window(canvas, menuButtonScale);

		if (!filePathToSaveWorkspace.is_empty())
		{
			menu.step_2_add_text(IO::Utils::get_file(filePathToSaveWorkspace).to_char())->set_scale(lerp(0.25f, 1.0f, menuButtonScale));
		}

		menu.step_2_add_button(TXT("new workspace"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::N)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				new_workspace_setup();
			});
		menu.step_2_add_button(TXT("open workspace"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::O)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				load_workspace_setup();
			});
		menu.step_2_add_button(TXT("copy workspace"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::C)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				copy_workspace_setup();
			});
		menu.step_2_add_button(TXT("assets"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::A)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				open_asset_manager();
			});
		menu.step_2_add_button(TXT("back"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				close_menu();
			});

		menu.step_3_finalise();
	}
}

void EditorContext::open_asset_list()
{
	state = ShowAssetList;

	if (workspace.get())
	{
		assetListContext.selectedAssetIdx = workspace->get_edited_asset_index();
	}
	else
	{
		assetListContext.selectedAssetIdx = 0;
	}

	canvas->clear();

	{
		UI::Utils::ListWindow list;

		float textScale = 1.5f;
		list.step_1_create_window(canvas, textScale)->set_title(TXT("assets"));
		list.step_2_setup_list(NAME(editor_assetList));
		list.step_3_add_button(TXT("select"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Return)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					edit_current_asset();
					close_menu();
				});
		list.step_3_add_button(TXT("back"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					edit_current_asset();
					close_menu();
				});

		list.step_4_finalise_force_size(canvas->get_logical_size() * 0.75f);

		populate_asset_list(true, textScale);
	}
}

void EditorContext::open_asset_manager()
{
	if (!workspace.get())
	{
		UI::Utils::QuestionWindow::show_message(get_canvas(), TXT("no workspace created"));
		open_menu(false);
		return;
	}

	state = ShowAssetManager;

	bool synchronisedWithLibrary = true;

	if (workspace.get())
	{
		assetListContext.selectedAssetIdx = workspace->get_edited_asset_index();
		synchronisedWithLibrary = workspace->is_synchronised_with_library();
	}
	else
	{
		assetListContext.selectedAssetIdx = 0;
	}

	canvas->clear();

	{
		UI::Utils::ListWindow list;

		float textScale = 1.5f;
		list.step_1_create_window(canvas, textScale)->set_title(TXT("assets"));
		list.step_2_setup_list(NAME(editor_assetList));
		list.step_3_add_button(TXT("create new"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::N)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					start_adding_new_asset();
				});
		list.step_3_add_button(TXT("copy"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::C)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					start_copying_asset();
				});
		if (!synchronisedWithLibrary)
		{
			list.step_3_add_button(TXT("load"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(System::Key::L)
#endif
				->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						load_asset();
					});
		}
		list.step_3_add_button(TXT("reload all"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::R, Framework::UI::ShortcutParams().with_ctrl().with_shift())
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					reload_all_assets();
				});
		if (!synchronisedWithLibrary)
		{
			list.step_3_add_button(TXT("save to 1 file"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(System::Key::S, Framework::UI::ShortcutParams().with_ctrl().with_shift())
#endif
				->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						save_all_assets_to_one_file();
					});
		}
		list.step_3_add_button(TXT("edit name"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::N, Framework::UI::ShortcutParams().with_ctrl())
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					change_name();
				});
/*
		// editing name allows to edit group as well
		list.step_3_add_button(TXT("edit group"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::G, Framework::UI::ShortcutParams().with_ctrl())
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					change_group_name();
				});
*/
		list.step_3_add_button(TXT("edit group all"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::G, Framework::UI::ShortcutParams().with_ctrl().with_shift())
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					change_group_name_all();
				});
		if (!synchronisedWithLibrary)
		{
			list.step_3_add_button(TXT("remove"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(System::Key::Delete)
#endif
				->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						remove_asset();
					});
		}
		list.step_3_add_button(TXT("move up"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::UpArrow, UI::ShortcutParams().with_ctrl().handle_hold_as_presses())
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					move_asset(-1);
				});
		list.step_3_add_button(TXT("move down"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::DownArrow, UI::ShortcutParams().with_ctrl().handle_hold_as_presses())
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					move_asset(1);
				});
		list.step_3_add_button(TXT("sort"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::S, Framework::UI::ShortcutParams().with_alt())
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					sort_all_assets();
				});
		if (!synchronisedWithLibrary)
		{
			list.step_3_add_button(TXT("sync to library"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(System::Key::L, Framework::UI::ShortcutParams().with_ctrl().with_shift())
#endif
				->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						synchronise_with_library(true);
					});
		}
		else
		{
			list.step_3_add_button(TXT("descync from library"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(System::Key::L, Framework::UI::ShortcutParams().with_ctrl().with_shift())
#endif
				->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						synchronise_with_library(false);
					});
		}
		list.step_3_add_button(TXT("menu"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					edit_current_asset();
					close_menu();
				});
		list.step_3_add_button(TXT("edit"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Return)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					edit_current_asset();
					close_menu();
				});

		list.step_4_finalise_force_size(canvas->get_logical_size());

		populate_asset_list(false, textScale);
	}
}

void EditorContext::populate_asset_list(Optional<bool> const& _editOnPress, Optional<float> const& _textScale)
{
	assetListContext.editOnPress = _editOnPress.get(assetListContext.editOnPress);
	assetListContext.textScale = _textScale.get(assetListContext.textScale);

	float textScale = assetListContext.textScale;

	if (workspace.get())
	{
		int assetAmount = workspace->get_assets().get_size();
		UI::Utils::ListWindow::populate_list(canvas, NAME(editor_assetList), assetAmount, assetListContext.selectedAssetIdx, textScale,
			[this](int idx, UI::CanvasButton* b)
			{
				if (workspace->get_assets().is_index_valid(idx))
				{
					auto & ea = workspace->get_assets()[idx];

					if (auto* ls = ea.libraryStored.get())
					{
						b->set_caption(String::printf(TXT("[%S] %S"), ls->get_library_type().to_char(), ls->get_name().to_string().to_char()));
					}
					else
					{
						b->set_caption(ea.get_file_path_to_save());
					}
					return;
				}

				b->set_caption(TXT("??"));
			},
			[this](int _idx, void const * _userData)
			{
				assetListContext.selectedAssetIdx = _idx;
				if (workspace.get())
				{
					workspace->set_edited_asset_index(_idx);
				}
			},
			[this](int _idx, void const* _userData)
			{
				assetListContext.selectedAssetIdx = _idx;
				if (workspace.get())
				{
					workspace->set_edited_asset_index(_idx);
				}
				if (assetListContext.editOnPress)
				{
					close_menu();
				}
			},
			[this](int _idx, void const* _userData)
			{
				assetListContext.selectedAssetIdx = _idx;
				if (workspace.get())
				{
					workspace->set_edited_asset_index(_idx);
				}
				close_menu();
			});
	}
	else
	{
		UI::Utils::ListWindow::populate_list(canvas, NAME(editor_assetList), 0, NP, NP, nullptr);
	}
}

void EditorContext::synchronise_with_library(bool _sync)
{
	if (workspace.get())
	{
		workspace->set_synchronised_with_library(_sync);
	}

	open_asset_manager();
}

void EditorContext::load_all_assets_in_library()
{
	if (workspace.get())
	{
		Array<String> filesToLoad;
		filesToLoad.make_space_for(1000);

#ifdef LIBRARY_STORED_WITH_LOADED_FROM_FILE
		auto* lib = Library::get_current();
		for_every(aat, availableAssetTypes)
		{
			if (auto* store = lib->get_store(*aat))
			{
				store->do_for_every([&filesToLoad](LibraryStored* _libraryStored)
					{
						if (_libraryStored->is_temporary()) return;

						filesToLoad.push_back_unique(_libraryStored->get_loaded_from_file());
					});
			}
		}
#endif

		workspace->load_asset_from_files(filesToLoad);
	}
	populate_asset_list();
}

void EditorContext::sort_all_assets()
{
	if (workspace.get())
	{
		sort(workspace->access_assets());
	}
	populate_asset_list();
}

void EditorContext::move_asset(int _by)
{
	if (workspace.get())
	{
		while (_by < 0)
		{
			int at = workspace->get_edited_asset_index();
			if (at > 0)
			{
				swap(workspace->access_assets()[at], workspace->access_assets()[at - 1]);
				workspace->set_edited_asset_index(at - 1);
				if (assetListContext.selectedAssetIdx == at)
				{
					--assetListContext.selectedAssetIdx;
				}
			}
			++_by;
		}
		while (_by > 0)
		{
			int at = workspace->get_edited_asset_index();
			if (at < workspace->access_assets().get_size() - 1)
			{
				swap(workspace->access_assets()[at], workspace->access_assets()[at + 1]);
				workspace->set_edited_asset_index(at + 1);
				if (assetListContext.selectedAssetIdx == at)
				{
					++assetListContext.selectedAssetIdx;
				}
			}
			--_by;
		}
	}
	populate_asset_list();
}

void EditorContext::remove_asset()
{
	if (workspace.get())
	{
		int idx = workspace->get_edited_asset_index();
		if (workspace->access_assets().is_index_valid(idx))
		{
			workspace->access_assets().remove_at(idx);
			if (idx >= workspace->access_assets().get_size())
			{
				workspace->set_edited_asset_index(max(0, idx - 1));
			}
			save_workspace();
			
			populate_asset_list();
		}
	}
}

void EditorContext::load_asset()
{
	IO::Dialogs::Params params;
	IO::Dialogs::setup_for_xml(params);
	an_assert(workspace.get());
	params.startInDirectory = workspace->get_edited_asset_file_path();
	auto fileToLoad = IO::Dialogs::get_file_to_open(params);
	if (fileToLoad.is_set())
	{
		fileToLoad = IO::Utils::make_relative(fileToLoad.get());
		if (load_asset_from_file(fileToLoad.get()))
		{
			edit_current_asset();
		}
		else
		{
			UI::Utils::QuestionWindow::show_message(canvas, TXT("can't load from file"), [this]() {open_asset_manager(); });
			return;
		}
	}
	open_asset_manager();
}

void EditorContext::load_edited_asset()
{
	an_assert(workspace.get());
	String const & fileToLoad = workspace->get_edited_asset_file_path();
	if (load_asset_from_file(fileToLoad))
	{
		edit_current_asset();
	}
	else
	{
		UI::Utils::QuestionWindow::show_message(canvas, TXT("can't load from file"), [this]() {open_asset_manager(); });
	}
}

bool EditorContext::load_asset_from_file(String const & _filePath)
{
	show_busy_canvas();
	an_assert(workspace.get());
	if (workspace->load_asset_from_file(_filePath))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void EditorContext::start_adding_new_asset()
{
	addNewAssetContext = AddNewAssetContext();
	if (workspace.get())
	{
		if (!workspace->get_assets().is_empty())
		{
			auto& ea = workspace->get_assets().get_last();
			addNewAssetContext.assetType = ea.libraryStored.get_type();
			addNewAssetContext.groupName = ea.libraryStored.get_name().get_group().to_string();
			String filePath = ea.get_file_path_to_save();
			if (!filePath.is_empty())
			{
				addNewAssetContext.saveToFile = IO::Utils::get_directory(filePath);
			}
			{
				String const& f = workspace->get_common_file_path_to_save();
				if (!f.is_empty())
				{
					copyAssetContext.saveToFile = f;
				}
			}
		}
	}
	open_add_new_asset();
}

void EditorContext::open_add_new_asset()
{
	state = ShowAddNewAsset;

	canvas->clear();

	{
		UI::Utils::Grid2Menu menu;

		menu.step_1_create_window(canvas, 2.0f)->set_title(TXT("add new asset"));
		menu.step_2_add_text_and_button(TXT("asset type"), addNewAssetContext.assetType.to_char())
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::T)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				open_new_asset_choose_asset_type();
			});
		menu.step_2_add_text_and_button(TXT("group name"), addNewAssetContext.groupName.to_char())
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::G)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				UI::Utils::edit_text(_context.canvas, TXT("group name"), addNewAssetContext.groupName, [this]() {open_add_new_asset(); });
			});
		menu.step_2_add_text_and_button(TXT("asset name"), addNewAssetContext.assetName.to_char())
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::N)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				UI::Utils::edit_text(_context.canvas, TXT("asset name"), addNewAssetContext.assetName, [this]()
					{
						{
							String const& f = workspace->get_common_file_path_to_save();
							if (!f.is_empty() &&
								copyAssetContext.saveToFile == f)
							{
								open_add_new_asset();
								return;
							}
						}
						if (!addNewAssetContext.saveToFile.is_empty())
						{
							addNewAssetContext.saveToFile = IO::Utils::get_directory(addNewAssetContext.saveToFile) + addNewAssetContext.assetName + TXT(".xml");
						}
						open_add_new_asset();
					});
			});
		menu.step_2_add_text_and_button(TXT("file"), addNewAssetContext.saveToFile.to_char())
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::F)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				IO::Dialogs::Params params;
				IO::Dialogs::setup_for_xml(params);
				if (!addNewAssetContext.saveToFile.is_empty())
				{
					params.startInDirectory = IO::Utils::get_directory(addNewAssetContext.saveToFile);
					params.existingFile = addNewAssetContext.saveToFile;
				}
				auto fileToSave = IO::Dialogs::get_file_to_save(params);
				if (fileToSave.is_set())
				{
					fileToSave = IO::Utils::make_relative(fileToSave.get());
					addNewAssetContext.saveToFile = fileToSave.get();
				}
				open_add_new_asset();
			});
		if (addNewAssetContext.assetType.is_valid() &&
			!addNewAssetContext.groupName.is_empty() &&
			!addNewAssetContext.assetName.is_empty() &&
			workspace->check_if_unique(addNewAssetContext.groupName, addNewAssetContext.assetName))
		{
			menu.step_3_add_button(TXT("add"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(System::Key::Return, UI::ShortcutParams().with_ctrl())
#endif
				->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					create_new_asset();
				});
		}
		else
		{
			menu.step_3_add_no_button();
		}
		menu.step_3_add_button(TXT("back"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				open_asset_manager();
			});

		menu.step_4_finalise(canvas->get_logical_size().x * 0.75f);
	}
}

void EditorContext::create_new_asset()
{
	auto* lib = Library::get_current();
	if (!lib)
	{
		return;
	}
	if (auto* s = lib->get_store(addNewAssetContext.assetType))
	{
		LibraryName libName;
		libName = LibraryName(addNewAssetContext.groupName, addNewAssetContext.assetName);
		if (auto* asset = s->find_stored_or_create(libName))
		{
			Game::get()->on_newly_created_library_stored_for_editor(asset);
			if (workspace.get())
			{
				workspace->access_assets().push_back();
				auto& ea = workspace->access_assets().get_last();
				ea.libraryStored = asset;
				ea.set_file_path_to_save(addNewAssetContext.saveToFile);
				workspace->update_edited_asset_index(asset);
				workspace->save_asset(asset);
				save_workspace();
				edit(asset, addNewAssetContext.saveToFile);
				open_asset_manager();
				return;
			}
		}
	}
	open_add_new_asset();
}

void EditorContext::start_copying_asset()
{
	copyAssetContext = CopyAssetContext();
	if (workspace.get())
	{
		if (!workspace->get_assets().is_empty())
		{
			copyAssetContext.copyAssetIdx = workspace->get_edited_asset_index();
			if (workspace->get_assets().is_index_valid(copyAssetContext.copyAssetIdx))
			{
				auto& ea = workspace->get_assets()[copyAssetContext.copyAssetIdx];
				copyAssetContext.copyGroupName = ea.libraryStored.get_name().get_group().to_string();
				copyAssetContext.copyAssetName = ea.libraryStored.get_name().get_name().to_string();
				copyAssetContext.copySaveToFile = ea.get_file_path_to_save();
				copyAssetContext.groupName = ea.libraryStored.get_name().get_group().to_string();
				copyAssetContext.assetName = ea.libraryStored.get_name().get_name().to_string();
				String filePath = ea.get_file_path_to_save();
				if (!filePath.is_empty())
				{
					copyAssetContext.saveToFile = filePath;
				}
				{
					String const& f = workspace->get_common_file_path_to_save();
					if (!f.is_empty())
					{
						copyAssetContext.saveToFile = f;
					}
				}
				open_copy_asset();
			}
		}
	}
}

void EditorContext::open_copy_asset()
{
	state = ShowCopyAsset;

	canvas->clear();

	{
		UI::Utils::Grid2Menu menu;

		menu.step_1_create_window(canvas, 2.0f)->set_title(TXT("copy asset"));
		menu.step_2_add_text_and_button(TXT("copy"), (copyAssetContext.groupName + TXT(":") + copyAssetContext.copyAssetName).to_char())
			->set_text_only();
		menu.step_2_add_text_and_button(TXT("group name"), copyAssetContext.groupName.to_char())
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::G)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				UI::Utils::edit_text(_context.canvas, TXT("group name"), copyAssetContext.groupName, [this]() {open_copy_asset(); });
			});
		menu.step_2_add_text_and_button(TXT("asset name"), copyAssetContext.assetName.to_char())
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::N)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				UI::Utils::edit_text(_context.canvas, TXT("asset name"), copyAssetContext.assetName, [this]()
					{
						if (copyAssetContext.saveToFile == copyAssetContext.copySaveToFile)
						{
							open_copy_asset();
							return;
						}
						{
							String const& f = workspace->get_common_file_path_to_save();
							if (!f.is_empty() &&
								copyAssetContext.saveToFile == f)
							{
								open_copy_asset();
								return;
							}
						}
						if (!copyAssetContext.saveToFile.is_empty())
						{
							copyAssetContext.saveToFile = IO::Utils::get_directory(copyAssetContext.saveToFile) + copyAssetContext.assetName + TXT(".xml");
							copyAssetContext.saveToFile = IO::Utils::make_relative(copyAssetContext.saveToFile);
						}
						open_copy_asset();
					});
			});
		menu.step_2_add_text_and_button(TXT("file"), copyAssetContext.saveToFile.to_char())
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::F)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				IO::Dialogs::Params params;
				IO::Dialogs::setup_for_xml(params);
				if (!copyAssetContext.saveToFile.is_empty())
				{
					params.startInDirectory = IO::Utils::get_directory(copyAssetContext.saveToFile);
					params.existingFile = copyAssetContext.saveToFile;
				}
				auto fileToSave = IO::Dialogs::get_file_to_save(params);
				if (fileToSave.is_set())
				{
					fileToSave = IO::Utils::make_relative(fileToSave.get());
					copyAssetContext.saveToFile = fileToSave.get();
				}
				open_copy_asset();
			});
		if (!copyAssetContext.groupName.is_empty() &&
			!copyAssetContext.assetName.is_empty() &&
			workspace->check_if_unique(copyAssetContext.groupName, copyAssetContext.assetName))
		{
			menu.step_3_add_button(TXT("create a copy"))
#ifdef AN_STANDARD_INPUT
				->set_shortcut(System::Key::Return, UI::ShortcutParams().with_ctrl())
#endif
				->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					copy_asset();
				});
		}
		else
		{
			menu.step_3_add_no_button();
		}
		menu.step_3_add_button(TXT("back"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				open_asset_manager();
			});

		menu.step_4_finalise(canvas->get_logical_size().x * 0.75f);
	}
}

void EditorContext::copy_asset()
{
	auto* lib = Library::get_current();
	if (!lib || ! workspace.get())
	{
		return;
	}
	if (workspace->get_assets().is_index_valid(copyAssetContext.copyAssetIdx))
	{
		auto& ea = workspace->get_assets()[copyAssetContext.copyAssetIdx];
		LibraryName libName;
		libName = LibraryName(copyAssetContext.groupName, copyAssetContext.assetName);
		// save to a new file with a changed name
		{
			LibraryName orgName = ea.libraryStored.get_name();
			ea.libraryStored->set_name(libName);
			// save only this asset (as it is not a proper copy, just changed name)
			// save it to the target file, note we might be overwriting stuff now, we will resave it in a moment
			workspace->save_asset_to_file(ea.libraryStored.get(), copyAssetContext.saveToFile);
			ea.libraryStored->set_name(orgName);
		}
		// add/load to have it in the library
		if (workspace->load_asset_from_file(copyAssetContext.saveToFile, copyAssetContext.copyAssetIdx + 1))
		{
			edit_current_asset();
			workspace->save_asset(workspace->get_edited_asset()); // resave (in case we saved to the same file)
			save_workspace();
			close_menu();
			return;
		}
		else
		{
			UI::Utils::QuestionWindow::show_message(canvas, TXT("can't create a copy"), [this]() {open_asset_manager(); });
			return;
		}
	}
	open_asset_manager();
}

void EditorContext::open_new_asset_choose_asset_type()
{
	canvas->clear();

	{
		UI::Utils::ListWindow list;

		selectedAssetTypeIdx = NONE;
		for_every(aat, availableAssetTypes)
		{
			if (addNewAssetContext.assetType == *aat)
			{
				selectedAssetTypeIdx = for_everys_index(aat);
			}
		}

		float textScale = 2.0f;
		list.step_1_create_window(canvas, textScale)->set_title(TXT("asset type"));
		list.step_2_setup_list(NAME(editor_assetType));
		list.step_3_add_button(TXT("select"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Return)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					if (availableAssetTypes.is_index_valid(selectedAssetTypeIdx))
					{
						addNewAssetContext.assetType = availableAssetTypes[selectedAssetTypeIdx];
					}
					open_add_new_asset();
				});
		list.step_3_add_button(TXT("back"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([this](Framework::UI::ActContext const& _context)
				{
					open_add_new_asset();
				});

		list.step_4_finalise_force_size(canvas->get_logical_size());

		populate_new_asset_choose_asset_type();
	}
}

void EditorContext::populate_new_asset_choose_asset_type()
{
	float textScale = 2.0f;

	UI::Utils::ListWindow::populate_list(canvas, NAME(editor_assetType), availableAssetTypes.get_size(), selectedAssetTypeIdx, textScale,
		[this](int idx, UI::CanvasButton* b)
		{
			b->set_caption(availableAssetTypes[idx].to_char());
		},
		[this](int _idx, void const* _userData)
		{
			selectedAssetTypeIdx = _idx;
		},
		[this](int _idx, void const* _userData)
		{
			selectedAssetTypeIdx = _idx;
			if (availableAssetTypes.is_index_valid(selectedAssetTypeIdx))
			{
				addNewAssetContext.assetType = availableAssetTypes[selectedAssetTypeIdx];
				open_add_new_asset();
			}
		},
		[this](int _idx, void const* _userData)
		{
			selectedAssetTypeIdx = _idx;
			if (availableAssetTypes.is_index_valid(selectedAssetTypeIdx))
			{
				addNewAssetContext.assetType = availableAssetTypes[selectedAssetTypeIdx];
				open_add_new_asset();
			}
		});
}

void EditorContext::save_asset()
{
	if (workspace.get())
	{
		workspace->save_asset();
	}
}

void EditorContext::save_all_assets()
{
	if (workspace.get())
	{
		workspace->save_all_assets();
	}
}

void EditorContext::reload_all_assets()
{
	if (workspace.get())
	{
		workspace->reload_all_assets();

		open_asset_manager();
	}
}

void EditorContext::save_all_assets_to_one_file()
{
	if (!workspace.get())
	{
		return;
	}

	show_busy_canvas();

	IO::Dir::create(String(WORKSPACE_DIRECTORY));

	IO::Dialogs::Params params;
	IO::Dialogs::setup_for_xml(params);
	for_every(a, workspace->access_assets())
	{
		params.startInDirectory = IO::Utils::get_directory(a->get_file_path_to_save());
		break;
	}
	auto fileToSave = IO::Dialogs::get_file_to_save(params);
	if (fileToSave.is_set())
	{
		fileToSave = IO::Utils::make_relative(fileToSave.get());
		for_every(a, workspace->access_assets())
		{
			a->set_file_path_to_save(fileToSave.get()); // will save all to a single file
		}
		save_all_assets();
		save_workspace();
		open_asset_manager();
	}

	open_asset_manager();
}

void EditorContext::change_name()
{
	if (!workspace.get())
	{
		return;
	}

	show_busy_canvas();

	UI::Utils::edit_text(get_canvas(), TXT("asset name"), workspace->get_edited_asset()->get_name().to_string(), [this](String const& _accepted)
		{
			workspace->get_edited_asset()->set_name(_accepted);
			save_workspace();
			open_asset_manager();
		},
		[this]() {open_asset_manager(); }
		);
}

void EditorContext::change_group_name()
{
	if (!workspace.get())
	{
		return;
	}

	show_busy_canvas();

	UI::Utils::edit_text(get_canvas(), TXT("group name"), workspace->get_edited_asset()->get_name().get_group().to_string(), [this](String const& _accepted)
		{
			Framework::LibraryName name = workspace->get_edited_asset()->get_name();
			name.set_group(Name(_accepted));
			workspace->get_edited_asset()->set_name(name);
			save_workspace();
			open_asset_manager();
		},
		[this]() {open_asset_manager(); }
		);
}

void EditorContext::change_group_name_all()
{
	if (!workspace.get())
	{
		return;
	}

	show_busy_canvas();

	UI::Utils::edit_text(get_canvas(), TXT("group name"), workspace->get_edited_asset()->get_name().get_group().to_string(), [this](String const& _accepted)
		{
			for_every(a, workspace->access_assets())
			{
				if (auto* ls = a->libraryStored.get())
				{
					Framework::LibraryName name = ls->get_name();
					name.set_group(Name(_accepted));
					ls->set_name(name);
				}
				else
				{
					Framework::LibraryName name = a->libraryStored.get_name();
					name.set_group(Name(_accepted));
					a->libraryStored.set(name, a->libraryStored.get_type());
				}
			}
			save_workspace();
			open_asset_manager();
		},
		[this]() {open_asset_manager(); }
		);
}

bool EditorContext::mark_save_required()
{
	if (workspace.get())
	{
		return workspace->mark_save_required();
	}
	return false;
}

bool EditorContext::is_save_required() const
{
	if (workspace.get())
	{
		return workspace->is_save_required();
	}
	return false;
}

bool EditorContext::is_save_all_required() const
{
	if (workspace.get())
	{
		return workspace->is_save_all_required();
	}
	return false;
}
