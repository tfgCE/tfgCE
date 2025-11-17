#include "editorWorkspace.h"

#include "..\game\game.h"
#include "..\library\library.h"

#include "..\loaders\loaderCircles.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

void EditorWorkspace::provide_available_asset_types(OUT_ Array<Name>& _availableAssetTypes)
{
	_availableAssetTypes.push_back(Framework::AssetPackage::library_type()); // although no proper editor yet
	_availableAssetTypes.push_back(Framework::Sprite::library_type());
	_availableAssetTypes.push_back(Framework::SpriteGrid::library_type());
	_availableAssetTypes.push_back(Framework::SpriteLayout::library_type());
	_availableAssetTypes.push_back(Framework::VoxelMold::library_type());
}

EditorWorkspace::EditorWorkspace()
{
}

EditorWorkspace::~EditorWorkspace()
{
}

void EditorWorkspace::update_all()
{
	for_every(asset, assets)
	{
		if (!asset->libraryStored.get())
		{
			asset->libraryStored.find(Library::get_current());
		}
	}
}

LibraryStored* EditorWorkspace::get_edited_asset() const
{
	if (assets.is_index_valid(editedAssetIndex))
	{
		return assets[editedAssetIndex].libraryStored.get();
	}
	return nullptr;
}

String const& EditorWorkspace::get_edited_asset_file_path() const
{
	if (assets.is_index_valid(editedAssetIndex))
	{
		return assets[editedAssetIndex].get_file_path_to_save();
	}
	return String::empty();
}

void EditorWorkspace::set_synchronised_with_library(bool _synchronisedWithLibrary)
{
	synchronisedWithLibrary = _synchronisedWithLibrary;

	if (synchronisedWithLibrary)
	{
		Loader::Circles loaderCircles;

		Game::get()->do_in_background_while_showing_loader(loaderCircles, 
			[this]()
			{
				auto* lib = Library::get_current();
				Array<Name> availableAssetTypes;
				provide_available_asset_types(OUT_ availableAssetTypes);
				for_every(aat, availableAssetTypes)
				{
					if (auto* store = lib->get_store(*aat))
					{
						store->do_for_every([this](LibraryStored* _libraryStored)
							{
								if (_libraryStored->is_temporary()) return;

								for_every(a, assets)
								{
									if (a->libraryStored == _libraryStored) // may point at it or not
									{
										a->libraryStored = _libraryStored; // update in case we just compared names
										a->set_file_path_to_save(NP); // clear
										return;
									}
								}
								assets.push_back();
								assets.get_last().libraryStored = _libraryStored;
							});
					}
				}
			});
	}
}

bool EditorWorkspace::load_from(IO::Interface * _io)
{
	bool result = false;

	// clear/reset
	//
	assets.clear();
	variables.clear();

	// load
	//
	IO::XML::Document doc;
	if (doc.load_xml(_io))
	{
		if (auto* rootNode = doc.get_root())
		{
			if (auto* node = rootNode->first_child_named(TXT("workspace")))
			{
				result = true;

				synchronisedWithLibrary = node->get_bool_attribute_or_from_child_presence(TXT("synchronisedWithLibrary"), synchronisedWithLibrary);

				for_every(assetsNode, node->children_named(TXT("assets")))
				{
					editedAssetIndex = assetsNode->get_int_attribute(TXT("editedAssetIndex"), editedAssetIndex);
					for_every(na, assetsNode->children_named(TXT("asset")))
					{
						Asset asset;
						if (asset.libraryStored.load_from_xml(na, TXT("type"), TXT("name")))
						{
							assets.push_back(asset);
						}
						else
						{
							result = false;
						}
					}
				}

				result &= variables.load_from_xml_child_node(node, TXT("variables"));

				if (synchronisedWithLibrary)
				{
					set_synchronised_with_library(synchronisedWithLibrary);
				}
			}
		}
	}

	return result;
}

bool EditorWorkspace::save_to(IO::Interface* _io)
{
	bool result = true;

	IO::XML::Document doc;
	if (auto* node = doc.get_root()->add_node(TXT("workspace")))
	{
		node->set_bool_attribute(TXT("synchronisedWithLibrary"), synchronisedWithLibrary);

		if (auto* assetsNode = node->add_node(TXT("assets")))
		{
			assetsNode->set_int_attribute(TXT("editedAssetIndex"), editedAssetIndex);

			for_every(asset, assets)
			{
				auto* n = assetsNode->add_node(TXT("asset"));
				result &= asset->libraryStored.save_to_xml(n, TXT("type"), TXT("name"));
			}
		}

		result &= variables.save_to_xml_child_node(node, TXT("variables"));
	}

	if (result)
	{
		result &= doc.save_xml(_io);
	}

	return result;
}

void EditorWorkspace::update_edited_asset_index(LibraryStored* _edited)
{
	for_every(asset, assets)
	{
		if (!asset->libraryStored.get())
		{
			asset->libraryStored.find(Library::get_current());
		}
		if (asset->libraryStored.get() == _edited)
		{
			editedAssetIndex = for_everys_index(asset);
			return;
		}
	}
}

bool EditorWorkspace::save_asset(LibraryStored* _libraryStored)
{
	if (!_libraryStored)
	{
		if (assets.is_index_valid(editedAssetIndex))
		{
			_libraryStored = assets[editedAssetIndex].libraryStored.get();
		}
	}

	if (!_libraryStored)
	{
		return true; // no error
	}

	String toFile;
	for_every(asset, assets)
	{
		if (asset->libraryStored.get() == _libraryStored)
		{
			asset->mark_requires_saving(false);
			toFile = asset->get_file_path_to_save();
			break;
		}
	}

	if (toFile.is_empty())
	{
		return false;
	}

	Array<LibraryStored const*> saveLibraryStored;
	for_every(asset, assets)
	{
		if (asset->get_file_path_to_save() == toFile)
		{
			if (auto* ls = asset->libraryStored.get())
			{
				saveLibraryStored.push_back(ls);
			}
		}
	}

	bool result = true;

	{
		IO::File file;
		file.create(toFile);
		if (file.is_open())
		{
			file.set_type(IO::InterfaceType::Text);
			result &= LibraryStored::save_to_file(&file, saveLibraryStored);
		}
	}

	return result;
}

bool EditorWorkspace::save_asset_to_file(LibraryStored* _libraryStored, String const & _toFile)
{
	if (!_libraryStored || _toFile.is_empty())
	{
		return false;
	}

	bool result = true;

	Array<LibraryStored const*> saveLibraryStored;
	saveLibraryStored.push_back(_libraryStored);

	{
		IO::File file;
		file.create(_toFile);
		if (file.is_open())
		{
			file.set_type(IO::InterfaceType::Text);
			result &= LibraryStored::save_to_file(&file, saveLibraryStored);
		}
	}

	return result;
}

bool EditorWorkspace::reload_all_assets()
{
	if (auto* game = Game::get())
	{
		Array<String> fromDirectories;
		Array<String> fromFiles;
		{
			for_every(asset, assets)
			{
				asset->mark_requires_saving(false);
				String filePath = asset->get_file_path_to_save();
				if (!fromFiles.does_contain(filePath))
				{
					fromFiles.push_back(filePath);
				}
			}
		}
		LibraryLoadSummary summary;
		summary.store_loaded();
		Loader::Circles loaderCircles;
		if (game->load_library(fromDirectories, fromFiles, LibraryLoadLevel::Main, &loaderCircles, &summary))
		{
			if (!summary.get_loaded().is_empty())
			{
				for_every_ptr(ls, summary.get_loaded())
				{
					for_every(a, assets)
					{
						if (a->libraryStored.get_type() == ls->get_library_type() &&
							a->libraryStored.get_name() == ls->get_name())
						{
							a->libraryStored = ls;
							break;
						}
					}
				}
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}

bool EditorWorkspace::save_all_assets(bool _onlyIfRequire)
{
	bool result = true;
	Array<String> saveFiles;
	for_every(asset, assets)
	{
		if (asset->does_require_saving() ||
			!_onlyIfRequire)
		{
			asset->mark_requires_saving(false);
			String filePath = asset->get_file_path_to_save();
			if (!saveFiles.does_contain(filePath))
			{
				saveFiles.push_back(filePath);
				if (auto* ls = asset->libraryStored.get())
				{
					result &= save_asset(ls);
				}
			}
		}
	}
	return result;
}

bool EditorWorkspace::load_asset_from_file(String const & _fileToLoad, Optional<int> const& _atIdx)
{
	if (auto* game = Game::get())
	{
		Array<String> fromFiles;
		fromFiles.push_back(_fileToLoad);
		return load_asset_from_files(fromFiles, _atIdx);
	}
	return false;
}

bool EditorWorkspace::load_asset_from_files(Array<String> const& _files, Optional<int> const& _atIdx)
{
	if (auto* game = Game::get())
	{
		Array<String> fromFiles;
		fromFiles.add_from(_files);

		Array<String> fromDirectories;
		LibraryLoadSummary summary;
		summary.store_loaded();
		Loader::Circles loaderCircles;
		if (game->load_library(fromDirectories, fromFiles, LibraryLoadLevel::Main, &loaderCircles, &summary))
		{
			if (!summary.get_loaded().is_empty())
			{
				for_every_ptr(ls, summary.get_loaded())
				{
					bool exists = false;
					for_every(a, assets)
					{
						if (a->libraryStored.get() == ls)
						{
							exists = true;
							a->set_file_path_to_save(NP);
							a->mark_requires_saving(false);
							break;
						}
					}
					if (!exists)
					{
						Asset* ea = nullptr;
						if (_atIdx.is_set() && _atIdx.get() < access_assets().get_size())
						{
							int idx = max(0, _atIdx.get());
							access_assets().insert_at(idx, Asset());
							ea = &access_assets()[idx];
						}
						else
						{
							access_assets().push_back();
							ea = &access_assets().get_last();
						}
						ea->libraryStored = ls;
						ea->set_file_path_to_save(NP);
					}
				}
				auto* ls = summary.get_loaded().get_first();
				update_edited_asset_index(ls);
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return false;
}

bool EditorWorkspace::mark_save_required()
{
	bool result = false;
	if (assets.is_index_valid(editedAssetIndex))
	{
		auto& a = assets[editedAssetIndex];
		result = !a.does_require_saving();
		a.mark_requires_saving(true);
	}
	return result;
}

bool EditorWorkspace::is_save_required() const
{
	bool result = false;
	if (assets.is_index_valid(editedAssetIndex))
	{
		auto& a = assets[editedAssetIndex];
		result = a.does_require_saving();
	}
	return result;
}

bool EditorWorkspace::is_save_all_required() const
{
	for_every(asset, assets)
	{
		if (asset->does_require_saving())
		{
			return true;;

		}
	}
	return false;
}

bool EditorWorkspace::check_if_unique(String const& _groupName, String const& _assetName) const
{
	for_every(asset, assets)
	{
		if (asset->libraryStored.get_name().get_group() == _groupName.to_char() &&
			asset->libraryStored.get_name().get_name() == _assetName.to_char())
		{
			return false;
		}
	}
	return true;
}

String const& EditorWorkspace::get_common_file_path_to_save() const
{
	if (assets.get_size() >= 1)
	{
		String const& filePathToSave = assets[0].get_file_path_to_save();
		for_every(a, assets)
		{
			if (a->get_file_path_to_save() != filePathToSave)
			{
				return String::empty();
			}
		}
		return filePathToSave;
	}
	else
	{
		return String::empty();
	}
}

//--

String const& EditorWorkspace::Asset::get_file_path_to_save() const
{
	if (filePathToSave.is_set())
	{
		return filePathToSave.get();
	}
#ifdef LIBRARY_STORED_WITH_LOADED_FROM_FILE
	if (auto* ls = libraryStored.get())
	{
		return ls->get_loaded_from_file();
	}
#endif

	return String::empty();
}

void EditorWorkspace::Asset::mark_requires_saving(bool _requiresSaving)
{
#ifdef WITH_IN_GAME_EDITOR
	if (auto* ls = libraryStored.get())
	{
		ls->mark_requires_saving(_requiresSaving);
	}
#endif
}

bool EditorWorkspace::Asset::does_require_saving() const
{
#ifdef WITH_IN_GAME_EDITOR
	if (auto* ls = libraryStored.get())
	{
		return ls->does_require_saving();
	}
#endif
	return false;
}
