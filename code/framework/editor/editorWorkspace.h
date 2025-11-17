#pragma once

#include "..\library\usedLibraryStoredAny.h"
#include "..\..\core\other\simpleVariableStorage.h"

namespace IO
{
	class Interface;
};

namespace Framework
{
	class EditorWorkspace
	: public RefCountObject
	{
	public:
		static void provide_available_asset_types(OUT_ Array<Name>& _availableAssetTypes);

	public:
		struct Asset
		{
			UsedLibraryStoredAny libraryStored;

			String const& get_file_path_to_save() const;
			void set_file_path_to_save(Optional<String> const& _filePathToSave) { filePathToSave = _filePathToSave; }

			void mark_requires_saving(bool _requiresSaving);
			bool does_require_saving() const;

			static inline int compare(void const* _a, void const* _b)
			{
				Asset const& a = *plain_cast<Asset>(_a);
				Asset const& b = *plain_cast<Asset>(_b);
				return UsedLibraryStoredAny::compare(&a.libraryStored, &b.libraryStored);
			}

		private:
			Optional<String> filePathToSave; // not saved
		};

	public:
		EditorWorkspace();
		virtual ~EditorWorkspace();

	public:
		Array<Asset>& access_assets() { return assets; }
		Array<Asset> const & get_assets() const { return assets; }

		void update_all();

		LibraryStored* get_edited_asset() const;
		String const& get_edited_asset_file_path() const;

		bool is_synchronised_with_library() const { return synchronisedWithLibrary; }
		void set_synchronised_with_library(bool _synchronisedWithLibrary);

		int get_edited_asset_index() const { return editedAssetIndex; }
		void set_edited_asset_index(int _index) { editedAssetIndex = _index; }
		void update_edited_asset_index(LibraryStored* _edited);

		SimpleVariableStorage& access_variables() { return variables; }

		bool load_asset_from_file(String const & _filePath, Optional<int> const & _atIdx = NP);
		bool load_asset_from_files(Array<String> const & _files, Optional<int> const& _atIdx = NP);

		bool save_asset(LibraryStored* _libraryStored = nullptr); // will save this asset and all that belong to the same file
		bool save_asset_to_file(LibraryStored* _libraryStored, String const& _toFile); // will save only this asset (!)
		bool save_all_assets(bool _onlyIfRequire = false);
		
		bool reload_all_assets();

		bool mark_save_required();
		bool is_save_required() const;
		bool is_save_all_required() const;

		bool check_if_unique(String const& _groupName, String const& _assetName) const; // group+asset have to be unique

		String const& get_common_file_path_to_save() const;

	public:
		bool load_from(IO::Interface * _io);
		bool save_to(IO::Interface * _io);

	private:
		bool synchronisedWithLibrary = true; // auto loads assets on open

		int editedAssetIndex = 0;
		Array<Asset> assets;

		SimpleVariableStorage variables;
	};
};
