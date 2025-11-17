#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\string.h"

namespace Framework
{
	class LibraryStored;

	class EditorWorkspace;
	namespace Editor
	{
		class Base;
	};

	namespace UI
	{
		class Canvas;
	};

	struct EditorContext
	{
	public:
		EditorContext();
		~EditorContext();

		Framework::Editor::Base* get_editor() const { return editor.get(); }

		void edit_current_asset();
		void edit(LibraryStored* _asset, String const& _filePathToSave = String::empty(), bool _storeForLater = true);

		void switch_to_editor(Framework::Editor::Base* _editor, bool _storeForLater = true);

		void switch_to_workspace(EditorWorkspace* _workspace, String const& _filePathToSave);

		void open_menu(bool _saveWorkspace = true);

	public:
		void save_workspace();
		bool load_workspace(String const& _filePath);
		void load_last_workspace_or_default();
		
		void save_asset();
		void save_all_assets();
		void save_all_assets_to_one_file();

		void load_edited_asset();

		void synchronise_with_library(bool _sync);

		void load_all_assets_in_library();
		void reload_all_assets();
		
		void change_name();

		void change_group_name();
		void change_group_name_all();

		void open_asset_list();
		void open_asset_manager();
		void change_asset(int _by);

		bool mark_save_required();
		bool is_save_required() const;
		bool is_save_all_required() const;

	public:
		UI::Canvas* get_canvas() const { return canvas; }
		void update_canvas_real_size(Vector2 const& _realSize);
		void update_canvas_logical_size(Optional<Vector2> const& _logicalSize = NP);

	private:
		enum State
		{
			ShowEditor,
			ShowMenu,
			ShowAssetList,
			ShowAssetManager,
			ShowAddNewAsset,
			ShowCopyAsset,
		} state = ShowEditor;

		UI::Canvas* canvas = nullptr;
		Vector2 canvasLogicalSize;

		RefCountObjectPtr<Framework::Editor::Base> editor;
		RefCountObjectPtr<Framework::EditorWorkspace> workspace;
		String filePathToSaveWorkspace;

		struct AssetListContext
		{
			int selectedAssetIdx = 0;
			bool editOnPress = false; // if true will automatically close when asset is pressed (double click always closes and edits)
			float textScale = 2.0f;
		} assetListContext;

		struct AddNewAssetContext
		{
			Name assetType;
			String groupName;
			String assetName;
			String saveToFile;
		} addNewAssetContext;

		struct CopyAssetContext
		{
			int copyAssetIdx = NONE;
			String copyGroupName;
			String copyAssetName;
			String copySaveToFile;
			String groupName;
			String assetName;
			String saveToFile;
		} copyAssetContext;

		void show_busy_canvas();

		void close_menu();

		void new_workspace_setup();
		void load_workspace_setup();
		void copy_workspace_setup();

		UI::Canvas* get_editor_canvas();

		void refresh_canvas_for_current_state(Optional<State> const & _onlyIfState = NP);

		void populate_asset_list(Optional<bool> const & _editOnPress = NP, Optional<float> const & _textScale = NP); // shared by manager too
		void sort_all_assets();
		void move_asset(int _by);
		void load_asset();
		bool load_asset_from_file(String const & _filePath);
		void remove_asset();

		void start_adding_new_asset();
		void open_add_new_asset();
		void create_new_asset();

		void start_copying_asset();
		void open_copy_asset();
		void copy_asset();

	private:
		Array<Name> availableAssetTypes;
		int selectedAssetTypeIdx = 0;

		void open_new_asset_choose_asset_type();
		void populate_new_asset_choose_asset_type();
	};
}