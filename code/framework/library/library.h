#pragma once

#include "..\framework.h"

#include "..\debugSettings.h"

#ifdef AN_CLANG
#define AN_SKIP_LIBRARY_STORE_INL
#endif
#include "libraryStore.h"
#ifdef AN_CLANG
#undef AN_SKIP_LIBRARY_STORE_INL
#endif
#include "libraryStored.h"
#include "libraryCombinedStore.h"
#include "libraryGlobalReferences.h"
#include "libraryLoadSummary.h"

#include "customLibraryStoredData.h"

#include "assetPackage.h"
#include "..\object\actorType.h"
#include "..\object\itemType.h"
#include "..\object\temporaryObjectType.h"
#include "..\object\sceneryType.h"
#include "..\ai\aiMind.h"
#include "..\appearance\animationGraph.h"
#include "..\appearance\animationSet.h"
#include "..\appearance\meshDepot.h"
#include "..\appearance\meshLibInstance.h"
#include "..\appearance\meshParticles.h"
#include "..\appearance\meshSkinned.h"
#include "..\appearance\meshStatic.h"
#include "..\appearance\skeleton.h"
#include "..\collision\collisionModel.h"
#include "..\collision\physicalMaterial.h"
#include "..\collision\physicalMaterialMap.h"
#include "..\gameScript\gameScript.h"
#include "..\world\doorType.h"
#include "..\world\doorTypeReplacer.h"
#include "..\world\roomPart.h"
#include "..\world\roomType.h"
#include "..\world\environmentOverlay.h"
#include "..\world\environmentType.h"
#include "..\world\regionType.h"
#include "..\world\regionLayouts\regionLayout.h"
#include "..\world\roomLayouts\objectLayout.h"
#include "..\world\roomLayouts\roomLayout.h"
#include "..\world\roomLayouts\wallLayout.h"
#include "..\video\shader.h"
#include "..\video\shaderProgram.h"
#include "..\video\shaderFunctionLib.h"
#include "..\video\texture.h"
#include "..\video\textureArray.h"
#include "..\video\texturePart.h"
#include "..\video\colourClashSprite.h"
#include "..\video\material.h"
#include "..\video\font.h"
#include "..\video\colourPalette.h"
#include "..\video\sprite.h"
#include "..\video\spriteGrid.h"
#include "..\video\spriteLayout.h"
#include "..\postProcess\postProcess.h"
#include "..\postProcess\chain\postProcessChainDefinition.h"
#include "..\sound\reverb.h"
#include "..\sound\soundSample.h"
#include "..\meshGen\meshGenerator.h"
#include "..\template\template.h"
#include "..\template\createFromTemplate.h"
#include "..\display\displayType.h"
#include "..\text\textBlock.h"
#include "..\general\libraryBasedParameters.h"
#include "..\voxel\voxelMold.h"

#include "..\minigames\platformer\platformerCharacter.h"
#include "..\minigames\platformer\platformerRoom.h"
#include "..\minigames\platformer\platformerInfo.h"
#include "..\minigames\platformer\platformerTile.h"

#include "libraryTool.h"

#include "libraryUtils.h"

//

class DebugVisualiser;

namespace Framework
{
	class Game;
	class Library;
	class PreviewGame;
	class SpriteTextureProcessor;
	class VRMeshes;

	void store_library_loading_time(Library* _library, Name const& _name, float _time);

	struct LibraryLoadInfo
	{
		float totalTime = 0.0f;
		Range rangeTime = Range::empty;
		int totalCount = 0;

		float get_total_time() const { return totalTime; }
		float get_average_time() const { return totalCount != 0 ? totalTime / (float)totalCount : 0.0f; }
		float get_min_time() const { return rangeTime.min; }
		float get_max_time() const { return rangeTime.max; }

		void add(float _time) { totalTime += _time; ++totalCount; rangeTime.include(_time); }
	};

	namespace LibraryState
	{
		enum Type
		{
			Idle,
			Loading,
			Preparing,
			LoadingOutsideQueue
		};
	};

	class Library
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		static bool is_skip_loading();
		static bool may_ignore_missing_references();
#endif
	public:
		// tl;dr custom attributes are light weight tags with limited amount of possible options
		// all library stored objects have custom attributes, the game may setup its own custom attributes that may be later set up conveniently in the editor
		// once defined, the text values should not be changed as they are used for storing/loading
		// custom attributes should not contain spaces
		static void set_custom_attribute_info_for_bit(int _bit, Name const & _info);
		static Name const & get_custom_attribute_info_for_bit(int _bit); // guaranteed to return valid char pointer
		static tchar const* get_custom_attribute_info_text_for_bit(int _bit); // guaranteed to return valid char pointer
		static int get_custom_attribute_info_bits_count();
		static String custom_attributes_to_string(int _customAttributes);
		static int parse_custom_attributes(String const & _value);

	public:
		template <typename LibraryClass>
		static LibraryClass* get_current_as() { return up_cast<LibraryClass>(s_current); }
		static Library* get_current() { return s_current; }
		static Library* get_main() { return s_main; }

		template <typename LibraryClass>
		static void initialise_static();
		static void close_static();

		static bool load_global_config_from_file(String const & _fileName, LibrarySource::Type _source);

		static bool load_global_settings_from_directory(String const & _dir, LibrarySource::Type _source);
		static bool load_global_settings_from_directories(Array<String> const & _dirs, LibrarySource::Type _source);

		static bool does_allow_loading_on_demand();
		static void allow_loading_on_demand(bool _allowLoadingOnDemand = true);

		bool load_tweaks_from_file(String const & _dir, LibrarySource::Type _source);
#ifdef AN_ALLOW_BULLSHOTS
		bool load_bullshots_from_file(String const & _dir, LibrarySource::Type _source);
#endif

		Library();
		virtual ~Library();

		LibraryState::Type get_state(bool _checkLoadingOutsideQueue = false) const;

		void add_defaults(); // add all default objects that should be in library

		virtual bool prepare_for_game();
		bool prepare_for_game_during_async(LibraryStored* _storedObject); // prepare for game just one object
#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool prepare_for_game_dev(LibraryStored* _storedObject);
#endif
		bool reload_from_file(String const& _fileName, LibrarySource::Type _source, LibraryStored* _storedObject); // reload just this one
		bool reload_from_file(String const & _fileName, LibrarySource::Type _source, LibraryStoredReloadRequiredCheck _reload_required_check); // check what to reload
		bool load_from_directory(String const & _dir, LibrarySource::Type _source, LibraryLoadLevel::Type _libraryLoadLevel, OPTIONAL_ LibraryLoadSummary* _summary = nullptr);
		bool load_from_directories(Array<String> const & _dirs, LibrarySource::Type _source, LibraryLoadLevel::Type _libraryLoadLevel, OPTIONAL_ LibraryLoadSummary* _summary = nullptr);
		bool load_from_file(String const & _file, LibrarySource::Type _source, LibraryLoadLevel::Type _libraryLoadLevel, OPTIONAL_ LibraryLoadSummary * _summary = nullptr);
		void wait_till_loaded(); // as it may be loading through workers, especially if loading from multiple directories, always call after requesting to load
		virtual void before_reload() {}
		virtual bool unload(LibraryLoadLevel::Type _libraryLoadLevel);
		LibraryStored * find_stored(LibraryName const & _name, Name const & _ofType = Name::invalid()) const;
		bool is_type_combined(Name const & _type) const;
		void do_for_type(Name const & _type, DoForLibraryType _do_for_library_type);
		void do_for_every_of_type(Name const & _type, DoForLibraryStored _do_for_library_stored);

		void unload_and_remove(LibraryStored* _libStored);

		int remove_unused_temporary_objects(Optional<int> const & _howMany = NP); // returns how many were removed

		static bool create_standalone_object_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, OUT_ RefCountObjectPtr<LibraryStored> & _object); // creates standalone, if result is true and there is no object, it was not recognised as any library stored

		virtual bool can_prepare_level_multithreaded(int _prepareLevel) const;

		void add_store(LibraryStoreBase * _store);
		LibraryStoreBase * get_store(Name const & _ofType) const;

		void add_tool(LibraryTool * _tool);
		void add_tweak_tool(LibraryTool * _tool);

		void connect_to_game(Game* _game) { game = _game; }
		Game* get_game() const { return game; }

		// to allow waiting for asynchronous jobs done before proceeding
		void preparing_asynchronous_job_added(); // call before adding so if job is extremely short, we won't miss it
		void preparing_asynchronous_job_done();

		// same for loading
		void loading_asynchronous_job_added(); // call before adding so if job is extremely short, we won't miss it
		void loading_asynchronous_job_done();

		LibraryGlobalReferences & access_global_references() { return globalReferences; }
		LibraryGlobalReferences const & get_global_references() const { return globalReferences; }

		VRMeshes& access_vr_meshes() { return *vrMeshes; }
		
		SpriteTextureProcessor& access_sprite_texture_processor() { return *spriteTextureProcessor; }

		LibraryLoadLevel::Type get_current_library_load_level() const { return currentLibraryLoadLevel; }
		void set_current_library_load_level(LibraryLoadLevel::Type _libraryLoadLevel) { currentLibraryLoadLevel = _libraryLoadLevel; }

		void list_all_objects(LogInfoContext & _log) const;
		void log_short_info(LogInfoContext & _log) const;

		Texture* get_default_texture() const;

#ifdef AN_DEVELOPMENT
		void ignore_preparing_problems(bool _ignore = true) { ignorePreparingProblems = _ignore; }
		bool should_ignore_preparing_problems() const { return ignorePreparingProblems; }
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		void setup(DebugVisualiser* _dv);
#endif

	protected: friend class LibraryStoreBase; // this is internal interface and for LibraryStored<>
			   friend class PreviewGame;
		bool prepare_globals_for_game(LibraryPrepareContext& _pfgContext);
		bool prepare_sprite_texture_processor(LibraryPrepareContext& _pfgContext);
		bool prepare_for_game(LibraryStored* _storedObject); // prepare for game just one object
		bool prepare_for_game(LibraryStored* _storedObject, LibraryPrepareContext& _pfgContext); // prepare for game as part of preparation, at single level

	protected: friend class LibraryStored;
		LibraryState::Type set_state(LibraryState::Type _state); // returns previous state

	protected:
		virtual void initialise_combined_stores();
		virtual void initialise_stores();

	protected:
		virtual bool load_gameplay(IO::XML::Node const * _node, LibraryLoadingContext & _lc, String const & _fileName, OUT_ bool & _loadResult);

		virtual bool does_allow_loading(LibraryStoreBase* _store, LibraryLoadingContext& _lc) const;

	protected:
		static Library* s_main;
		static Library* s_current;
		
		LibraryLoadLevel::Type currentLibraryLoadLevel = LibraryLoadLevel::Init;

		Concurrency::Counter preparingAsynchronousJobCount;
		Concurrency::Counter loadingAsynchronousJobCount;

		// combined stores
		COMBINED_LIBRARY_STORE(Mesh, meshes, meshes);
		COMBINED_LIBRARY_STORE(ObjectType, objectTypes, object_types);
		
		// stores
		LIBRARY_STORE(AssetPackage, assetPackages, asset_packages);

		LIBRARY_STORE(Template, templates, templates);
		LIBRARY_STORE(CreateFromTemplate, createFromTemplates, create_from_templates);

		LIBRARY_STORE(TextBlock, textBlocks, text_blocks);
		
		LIBRARY_STORE(LibraryBasedParameters, libraryBasedParameters, library_based_parameters);

		LIBRARY_STORE(ActorType, actorTypes, actor_types);
		LIBRARY_STORE(ItemType, itemTypes, item_types);
		LIBRARY_STORE(SceneryType, sceneryTypes, scenery_types);

		LIBRARY_STORE(TemporaryObjectType, temporaryObjectTypes, temporary_object_types);

		LIBRARY_STORE(CustomLibraryStoredData, customDatas, custom_datas);

		LIBRARY_STORE(AI::Mind, aiMinds, ai_minds);

		LIBRARY_STORE(Reverb, reverbs, reverbs);
		LIBRARY_STORE(Sample, samples, samples);

		LIBRARY_STORE(AnimationGraph, animationGraphs, animation_graphs);
		LIBRARY_STORE(AnimationSet, animationSets, animation_sets);

		LIBRARY_STORE(MeshParticles, meshesParticles, meshes_particles);
		LIBRARY_STORE(MeshSkinned, meshesSkinned, meshes_skinned);
		LIBRARY_STORE(MeshStatic, meshesStatic, meshes_static);

		LIBRARY_STORE(MeshLibInstance, meshLibInstances, mesh_lib_instances);

		LIBRARY_STORE(MeshDepot, meshDepots, mesh_depots);

		LIBRARY_STORE(Skeleton, skeletons, skeletons);

		LIBRARY_STORE(CollisionModel, collisionModels, collision_models);
		LIBRARY_STORE(PhysicalMaterial, physicalMaterials, physical_materials);
		LIBRARY_STORE(PhysicalMaterialMap, physicalMaterialMaps, physical_material_maps);

		LIBRARY_STORE(RoomPart, roomParts, room_parts);
		LIBRARY_STORE(DoorType, doorTypes, door_types);
		LIBRARY_STORE(DoorTypeReplacer, doorTypeReplacers, door_type_replacers);
		LIBRARY_STORE(RoomType, roomTypes, room_types);
		LIBRARY_STORE(EnvironmentType, environmentTypes, environment_types);
		LIBRARY_STORE(EnvironmentOverlay, environmentOverlays, environment_overlays);
		LIBRARY_STORE(RegionType, regionTypes, region_types);
		LIBRARY_STORE(ObjectLayout, objectLayouts, object_layouts);
		LIBRARY_STORE(RegionLayout, regionLayouts, region_layouts);
		LIBRARY_STORE(RoomLayout, roomLayouts, room_layouts);
		LIBRARY_STORE(WallLayout, wallLayouts, wall_layouts);

		LIBRARY_STORE(ShaderFunctionLib, shaderFunctionLibs, shader_function_libs);
		LIBRARY_STORE(FragmentShader, fragmentShaders, fragment_shaders);
		LIBRARY_STORE(VertexShader, vertexShaders, vertex_shaders);
		LIBRARY_STORE(ShaderProgram, shaderPrograms, shader_programs);
		LIBRARY_STORE(Texture, textures, textures);
		LIBRARY_STORE(TextureArray, textureArrays, texture_arrays);
		LIBRARY_STORE(TexturePart, textureParts, texture_parts);
		LIBRARY_STORE(ColourClashSprite, colourClashSprites, colour_clash_sprites);
		LIBRARY_STORE(Material, materials, materials);
		LIBRARY_STORE(PostProcess, postProcesses, post_processes);
		LIBRARY_STORE(PostProcessChainDefinition, postProcessChainDefinitions, post_process_chain_definitions);
		LIBRARY_STORE(Font, fonts, fonts);
		LIBRARY_STORE(DisplayType, displayTypes, display_types);
		LIBRARY_STORE(ColourPalette, colourPalettes, colour_palettes);
		LIBRARY_STORE(Sprite, sprites, sprites);
		LIBRARY_STORE(SpriteGrid, spriteGrids, sprite_grids);
		LIBRARY_STORE(SpriteLayout, spriteLayouts, sprite_layouts);

		LIBRARY_STORE(GameScript::Script, gameScripts, game_scripts);

		LIBRARY_STORE(MeshGenerator, meshGenerators, mesh_generators);
		
		LIBRARY_STORE(VoxelMold, voxelMolds, voxel_molds);

#ifdef AN_MINIGAME_PLATFORMER
		LIBRARY_STORE(Platformer::Character, minigamePlatformerCharacters, minigame_platformer_characters);
		LIBRARY_STORE(Platformer::Room, minigamePlatformerRooms, minigame_platformer_rooms);
		LIBRARY_STORE(Platformer::Info, minigamePlatformerInfos, minigame_platformer_infos);
		LIBRARY_STORE(Platformer::Tile, minigamePlatformerTiles, minigame_platformer_tiles);
#endif

	private:
		static bool s_loadingOnDemandAllowed;

		LibraryState::Type libraryState = LibraryState::Idle;
		Concurrency::Counter libraryLoading;

		// list and map
		List<LibraryStoreBase*> stores; // to store all stores, makes it easier to load, etc.
		Map<Name, CombinedLibraryStoreBase*> combinedStores; // to handle all combined, indexed with typeName
		List<LibraryTool*> tools; // all tools (non tweak)
		List<LibraryTool*> tweakTools; // tweaks tools

		LibraryGlobalReferences globalReferences;
		VRMeshes* vrMeshes;
		SpriteTextureProcessor* spriteTextureProcessor;

		RefCountObjectPtr<Texture> defaultTexture;

#ifdef AN_DEVELOPMENT
		bool ignorePreparingProblems = false;
#endif

		Game* game;

		void initialise();
		static bool load_from_directory_internal(Library* _forLibrary, String const & _dir, LibrarySource::Type _source, LibraryLoadingMode::Type _loadingMode, OPTIONAL_ LibraryLoadSummary* _summary); // if no library, only settings are loaded
		static bool load_from_subdirectory_internal(Library* _forLibrary, String const & _dir, LibrarySource::Type _source, LibraryLoadingMode::Type _loadingMode, REF_ Concurrency::Counter & _failCount, OPTIONAL_ LibraryLoadSummary* _summary); // if no library, only settings are loaded
		static bool load_from_file_internal(Library* _forLibrary, String const & _file, LibrarySource::Type _source, LibraryLoadingMode::Type _loadingMode, OPTIONAL_ LibraryLoadSummary* _summary = nullptr); // if no library, only settings are loaded
		static bool load_from_file(String const & _fileName, LibrarySource::Type _source, LibraryLoadingContext & _lc, OPTIONAL_ LibraryLoadSummary* _summary = nullptr);
		static void load_from_file_worker(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		static bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, String const & _fileName, OPTIONAL_ LibraryLoadSummary* _summary = nullptr); // file name is for ref only

		void update_default_texture();

	protected:
		virtual bool load_special_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc, String const& _fileName, OPTIONAL_ LibraryLoadSummary* _summary, OUT_ bool & _loadResult); // file name is for ref only

#ifdef AN_DEVELOPMENT_OR_PROFILER
	private:
		static bool load_library_load_setup(IO::XML::Node const* _node);
		static bool should_load_directory(String const& _dir);
	public:
		static void skip_loading_directory_with_suffix(String const& _suffix);
#endif

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	public:
		Concurrency::SpinLock libraryLoadInfoLock = Concurrency::SpinLock(TXT("Framework.Library.libraryLoadInfoLock"));
		Map<Name, LibraryLoadInfo> libraryLoadInfo;
		void output_load_info();
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	private:
		struct LibraryLoadSetup
		{
			Array<String> excludeSuffixDirectory;
			Array<String> excludePaths;
			Array<String> includePaths;
		};
		static LibraryLoadSetup* s_libraryLoadSetup;
#endif
	};

	#include "library.inl"

	#ifdef AN_CLANG
	#include "libraryStore.inl"
	#endif
};
