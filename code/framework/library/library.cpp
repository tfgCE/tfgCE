#include "library.h"

#include "..\..\core\mainConfig.h"
#include "..\..\core\mainSettings.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\concurrency\asynchronousJobList.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\concurrency\scopedCounter.h"
#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\types\names.h"
#include "..\..\core\io\assetDir.h"
#include "..\..\core\io\assetFile.h"
#include "..\..\core\io\dir.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\physicalSensations\bhaptics\bh_library.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\video\primitivesPipeline.h"
#include "..\..\core\wheresMyPoint\iWMPTool.h"
#include "..\..\core\vr\vrOffsets.h"

#include "..\video\spriteTextureProcessor.h"

#include "..\vr\vrMeshes.h"

#include "libraryStore.h"

#include "..\ai\aiSocialRules.h"

#include "..\pipelines\renderingPipeline.h"
#include "..\pipelines\postProcessPipeline.h"

#include "..\text\localisedString.h"

#include "tools\libraryTools.h"

#include "..\debug\testConfig.h"

#include "..\game\game.h"
#include "..\game\gameInput.h"
#include "..\game\gameConfig.h"

#include "..\gameScript\registeredGameScriptConditions.h"
#include "..\gameScript\registeredGameScriptElements.h"

#include "..\module\custom\mc_lcdLetters.h"

#include "..\debug\previewGame.h"

#include "..\jobSystem\jobSystem.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
	#ifndef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		//#define OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	#endif
	#ifndef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED_SINGLE
		//#define OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED_SINGLE
	#endif
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(generateTexture);
DEFINE_STATIC_NAME(generateFontTexture);
DEFINE_STATIC_NAME(modifyCurrentEnvironment);
DEFINE_STATIC_NAME_STR(loadingXML, TXT("~ loading xml"));
DEFINE_STATIC_NAME_STR(parsing, TXT("~ parsing/loading from xml"));

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
Concurrency::MRSWLock skipLoadingLock;
Array<String> skipLoadingFolder;
#endif

//

void Framework::store_library_loading_time(Library* _library, Name const& _name, float _time)
{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	if (_library)
	{
		Concurrency::ScopedSpinLock lock(_library->libraryLoadInfoLock);
		_library->libraryLoadInfo[_name].add(_time);
	}
#endif
}

//

struct CustomAttributeInfo
{
	Name info;
};
ArrayStatic<CustomAttributeInfo, sizeof(int) * 8> libraryCustomAttributeInfos;

void Library::set_custom_attribute_info_for_bit(int _bit, Name const & _info)
{
	an_assert(!_info.to_string().does_contain(' '));
	if (libraryCustomAttributeInfos.get_size() <= _bit)
	{
		libraryCustomAttributeInfos.set_size(_bit + 1);
	}

	libraryCustomAttributeInfos[_bit].info = _info;
}

Name const & Library::get_custom_attribute_info_for_bit(int _bit)
{
	if (_bit < libraryCustomAttributeInfos.get_size())
	{
		return libraryCustomAttributeInfos[_bit].info;
	}
	return Name::invalid();
}

tchar const* Library::get_custom_attribute_info_text_for_bit(int _bit)
{
	tchar const* result = nullptr;
	if (_bit < libraryCustomAttributeInfos.get_size())
	{
		result = libraryCustomAttributeInfos[_bit].info.to_char();
	}
	return result ? result : TXT("");
}

int Library::get_custom_attribute_info_bits_count()
{
	return libraryCustomAttributeInfos.get_size();
}

String Library::custom_attributes_to_string(int customAttributes)
{
	String result;
	uint v = (uint)customAttributes;
	int bitIdx = 0;
	for_count(int, i, 2 * sizeof(v))
	{
		if (v & 0x01)
		{
			if (!result.is_empty())
			{
				result += TXT(" ");
			}
			result += Library::get_custom_attribute_info_for_bit(bitIdx).to_string();
		}
		++bitIdx;
		v = v >> 1;
		if (v == 0)
		{
			break;
		}
	}
	return result;
}

int Library::parse_custom_attributes(String const & _value)
{
	int customAttributes = 0;
	String trimmed;
	List<String> attributes;
	_value.split(String::space(), attributes);
	for_every(a, attributes)
	{
		trimmed = a->trim();
		for_every(ca, libraryCustomAttributeInfos)
		{
			if (ca->info == trimmed.to_char())
			{
				set_flag(customAttributes, bit(for_everys_index(ca)));
				break;
			}
		}
	}
	return customAttributes;
}

//

Library* Library::s_main = nullptr;
Library* Library::s_current = nullptr;
#ifdef AN_DEVELOPMENT_OR_PROFILER
Library::LibraryLoadSetup* Library::s_libraryLoadSetup = nullptr;
#endif
bool Library::s_loadingOnDemandAllowed = false;

bool Library::does_allow_loading_on_demand()
{
	return s_loadingOnDemandAllowed;
}

void Library::allow_loading_on_demand(bool _allowLoadingOnDemand)
{
	s_loadingOnDemandAllowed = _allowLoadingOnDemand;
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool Library::is_skip_loading()
{
	return !skipLoadingFolder.is_empty();
}
#endif

void Library::close_static()
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	delete_and_clear(s_libraryLoadSetup);
#endif
	if (s_current)
	{
		if (s_main == s_current)
		{
			s_main = nullptr;
		}
		delete_and_clear(s_current);
	}
	delete_and_clear(s_main);
}

Library::Library()
: vrMeshes(new VRMeshes(this))
, spriteTextureProcessor(new SpriteTextureProcessor())
, game(nullptr)
{			
}

Library::~Library()
{
	defaultTexture.clear();
	delete_and_clear(spriteTextureProcessor);
	delete_and_clear(vrMeshes);
	globalReferences.clear();
#ifdef AN_DEVELOPMENT
	if (LibraryStored::does_any_exist())
	{
		output(TXT("not all library stored objects destroyed"));
		LibraryStored::output_all();
		an_assert(!LibraryStored::does_any_exist(), TXT("not all library stored objects destroyed"));
	}
#endif
	for_every_reverse_ptr(store, stores)
	{
		delete store;
	}
	stores.clear();
	for_every_ptr(tool, tools)
	{
		delete tool;
	}
	tools.clear();
	for_every_ptr(tweakTool, tweakTools)
	{
		delete tweakTool;
	}
	tweakTools.clear();
}

LibraryState::Type Library::get_state(bool _checkLoadingOutsideQueue) const
{
	if (libraryLoading != 0)
	{
		return LibraryState::Loading;
	}
	if (!_checkLoadingOutsideQueue && libraryState == LibraryState::LoadingOutsideQueue)
	{
		return LibraryState::Idle;
	}
	return libraryState;
}

LibraryState::Type Library::set_state(LibraryState::Type _state)
{
	auto prevState = libraryState;
#ifdef AN_ASSERT
	if (libraryState != _state)
	{
		an_assert(_state != LibraryState::LoadingOutsideQueue || libraryState == LibraryState::Idle, TXT("switching to loading outside queue possible only while idling"));
		an_assert(_state == LibraryState::Idle || libraryState != LibraryState::LoadingOutsideQueue, TXT("switching from loading outside queue possible only to idle"));
	}
#endif
	libraryState = _state;
	return prevState;
}

void Library::initialise()
{
	// combined go first
	initialise_combined_stores();

	initialise_stores();

	// build combined map
	for_every_ptr(store, stores)
	{
		if (CombinedLibraryStoreBase* combinedStore = fast_cast<CombinedLibraryStoreBase>(store))
		{
			combinedStores[combinedStore->get_type_name()] = combinedStore;
		}
	}
}

void Library::initialise_combined_stores()
{
	ADD_COMBINED_LIBRARY_STORE(Mesh, meshes, mesh);
	ADD_COMBINED_LIBRARY_STORE(ObjectType, objectTypes, objectType);
}

void Library::initialise_stores()
{
	// normal
	ADD_LIBRARY_STORE_WITH_DEFAULTS(ActorType, actorTypes);
	ADD_LIBRARY_STORE_WITH_DEFAULTS(ItemType, itemTypes);
	ADD_LIBRARY_STORE_WITH_DEFAULTS(SceneryType, sceneryTypes);

	ADD_LIBRARY_STORE_WITH_DEFAULTS(TemporaryObjectType, temporaryObjectTypes);

	ADD_LIBRARY_STORE(TextBlock, textBlocks);
	
	ADD_LIBRARY_STORE(LibraryBasedParameters, libraryBasedParameters);

	ADD_LIBRARY_STORE(CustomLibraryStoredData, customDatas);

	ADD_LIBRARY_STORE(AssetPackage, assetPackages);
	
	ADD_LIBRARY_STORE(Template, templates);
	ADD_LIBRARY_STORE(CreateFromTemplate, createFromTemplates);

	ADD_LIBRARY_STORE(AI::Mind, aiMinds);

	ADD_LIBRARY_STORE(Texture, textures);
	ADD_LIBRARY_STORE(TextureArray, textureArrays);
	ADD_LIBRARY_STORE(TexturePart, textureParts);
	ADD_LIBRARY_STORE(ColourClashSprite, colourClashSprites);
	ADD_LIBRARY_STORE(ShaderFunctionLib, shaderFunctionLibs);
	ADD_LIBRARY_STORE(FragmentShader, fragmentShaders);
	ADD_LIBRARY_STORE(VertexShader, vertexShaders);
	ADD_LIBRARY_STORE(ShaderProgram, shaderPrograms);
	ADD_LIBRARY_STORE(Material, materials);
	ADD_LIBRARY_STORE(PostProcess, postProcesses);
	ADD_LIBRARY_STORE(PostProcessChainDefinition, postProcessChainDefinitions);
	ADD_LIBRARY_STORE(Font, fonts);
	ADD_LIBRARY_STORE(DisplayType, displayTypes);
	ADD_LIBRARY_STORE(ColourPalette, colourPalettes);
	ADD_LIBRARY_STORE(Sprite, sprites);
	ADD_LIBRARY_STORE(SpriteGrid, spriteGrids);
	ADD_LIBRARY_STORE(SpriteLayout, spriteLayouts);

	ADD_LIBRARY_STORE(MeshGenerator, meshGenerators);
	
	ADD_LIBRARY_STORE(VoxelMold, voxelMolds);

	ADD_LIBRARY_STORE(Reverb, reverbs);
	ADD_LIBRARY_STORE(Sample, samples);

	ADD_LIBRARY_STORE(Skeleton, skeletons);

	ADD_LIBRARY_STORE(AnimationGraph, animationGraphs);
	ADD_LIBRARY_STORE(AnimationSet, animationSets);

	ADD_LIBRARY_STORE(MeshStatic, meshesStatic);
	ADD_LIBRARY_STORE(MeshSkinned, meshesSkinned);
	ADD_LIBRARY_STORE(MeshParticles, meshesParticles);

	ADD_LIBRARY_STORE(MeshLibInstance, meshLibInstances);

	ADD_LIBRARY_STORE(MeshDepot, meshDepots);
	
	ADD_LIBRARY_STORE(CollisionModel, collisionModels);
	ADD_LIBRARY_STORE(PhysicalMaterial, physicalMaterials);
	ADD_LIBRARY_STORE(PhysicalMaterialMap, physicalMaterialMaps);

	ADD_LIBRARY_STORE(RoomPart, roomParts);
	ADD_LIBRARY_STORE(DoorType, doorTypes);
	ADD_LIBRARY_STORE(DoorTypeReplacer, doorTypeReplacers);
	ADD_LIBRARY_STORE(RoomType, roomTypes);
	ADD_LIBRARY_STORE(EnvironmentType, environmentTypes);
	ADD_LIBRARY_STORE(EnvironmentOverlay, environmentOverlays);
	ADD_LIBRARY_STORE(RegionType, regionTypes);

	ADD_LIBRARY_STORE(ObjectLayout, objectLayouts);
	ADD_LIBRARY_STORE(RegionLayout, regionLayouts);
	ADD_LIBRARY_STORE(RoomLayout, roomLayouts);
	ADD_LIBRARY_STORE(WallLayout, wallLayouts);

	ADD_LIBRARY_STORE(GameScript::Script, gameScripts);

#ifdef AN_MINIGAME_PLATFORMER
	ADD_LIBRARY_STORE(Platformer::Character, minigamePlatformerCharacters);
	ADD_LIBRARY_STORE(Platformer::Room, minigamePlatformerRooms);
	ADD_LIBRARY_STORE(Platformer::Info, minigamePlatformerInfos);
	ADD_LIBRARY_STORE(Platformer::Tile, minigamePlatformerTiles);
#endif

	// define combined
	meshes->add(meshesParticles);
	meshes->add(meshesSkinned);
	meshes->add(meshesStatic);
	//
	objectTypes->add(actorTypes);
	objectTypes->add(itemTypes);
	objectTypes->add(sceneryTypes);

	// tools
	ADD_LIBRARY_TOOL(NAME(generateTexture), [](IO::XML::Node const * _node, LibraryLoadingContext & _lc) { return ::Framework::LibraryTools::generate_texture(_node, _lc); });
	ADD_LIBRARY_TOOL(NAME(generateFontTexture), [](IO::XML::Node const * _node, LibraryLoadingContext & _lc) { return ::Framework::LibraryTools::generate_font_texture(_node, _lc); });

	// tweaks
	ADD_LIBRARY_TWEAK_TOOL(NAME(modifyCurrentEnvironment), [](IO::XML::Node const * _node, LibraryLoadingContext & _lc) { return ::Framework::LibraryTools::modify_current_environment(_node, _lc); });
}

bool Library::unload(LibraryLoadLevel::Type _libraryLoadLevel)
{
	if (_libraryLoadLevel <= Framework::LibraryLoadLevel::Main)
	{
#ifdef AN_ALLOW_BULLSHOTS
		Framework::BullshotSystem::close_static();
#endif
	}
	bool result = true;
	defaultTexture.clear();
	spriteTextureProcessor->clear();
	result &= vrMeshes->unload(_libraryLoadLevel);
	result &= globalReferences.unload(_libraryLoadLevel);
	result &= LocalisedStrings::unload(_libraryLoadLevel);
	for_every_reverse_ptr(store, stores)
	{
		result &= store->unload(_libraryLoadLevel);
	}
	return result;
}

void Library::preparing_asynchronous_job_added()
{
	preparingAsynchronousJobCount.increase();
}

void Library::preparing_asynchronous_job_done()
{
	an_assert(preparingAsynchronousJobCount.get() > 0, TXT("there were some jobs that didn't call preparing_asynchronous_job_added before adding job"));
	preparingAsynchronousJobCount.decrease();
}

void Library::loading_asynchronous_job_added()
{
	loadingAsynchronousJobCount.increase();
}

void Library::loading_asynchronous_job_done()
{
	an_assert(loadingAsynchronousJobCount.get() > 0, TXT("there were some jobs that didn't call loading_asynchronous_job_added before adding job"));
	loadingAsynchronousJobCount.decrease();
}

bool Library::prepare_for_game()
{
	bool result = true;
	
	libraryState = LibraryState::Preparing;

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	output_colour(0, 0, 0, 1);
	output(TXT("prepare for game - begin"));
	output_colour();
	::System::TimeStamp prepareTotalTimeStamp;
#endif

	update_default_texture();

	LibraryPrepareContext pfgContext;
	while (pfgContext.start_next_requested_level())
	{
		pfgContext.request_level(LibraryPrepareLevel::Auto_SpriteTextureProcessor);

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		::System::TimeStamp prepareTimeStamp;
#endif
		if (pfgContext.get_current_level() == LibraryPrepareLevel::Resolve)
		{
			prepare_globals_for_game(pfgContext);
		}
		if (pfgContext.get_current_level() == LibraryPrepareLevel::Auto_SpriteTextureProcessor)
		{
			prepare_sprite_texture_processor(pfgContext);
		}

		for_every_ptr(store, stores)
		{
			// combined are already prepared
			if (fast_cast<CombinedLibraryStoreBase>(store) == nullptr)
			{
				result = store->prepare_for_game(this, pfgContext) && result;
			}
		}

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		float mainPrepareTime = prepareTimeStamp.get_time_since();
#endif
		// wait for all asynchronous jobs to end, do all available async worker jobs
		while (preparingAsynchronousJobCount != 0)
		{
			if (auto* game = get_game())
			{
				// allow background jobs to perform
				{
					auto* asyncList = game->get_job_system()->get_asynchronous_list(game->game_background_jobs());
					if (asyncList->execute_job_if_available(nullptr))
					{
						continue;
					}
				}
				{
					auto* asyncList = game->get_job_system()->get_asynchronous_list(game->background_jobs());
					if (asyncList->execute_job_if_available(nullptr))
					{
						continue;
					}
				}
				// if we have more than that, leave the job for them and don't block the main thread
				if (auto* list = game->get_job_system()->get_asynchronous_list(game->preparing_worker_jobs()))
				{
					if (list->execute_job_if_available(nullptr))
					{
						continue;
					}
				}
			}
			::System::Core::sleep(0.001f);
		}
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		float totalPrepareTime = prepareTimeStamp.get_time_since();
		output_colour(0, 0, 0, 1);
		output(TXT("  %03i : %5.3f = %5.3f + %5.3f"), pfgContext.get_current_level(), totalPrepareTime, mainPrepareTime, totalPrepareTime - mainPrepareTime);
		output_colour();
#endif

		result &= !pfgContext.has_failed();
	}
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	output_colour(0, 0, 0, 1);
	output(TXT("prepared for game in %.3f"), prepareTotalTimeStamp.get_time_since());
	output(TXT("prepare for game - end"));
	output_colour();
#endif

	for_every_ptr(store, stores)
	{
		// combined are already prepared
		if (fast_cast<CombinedLibraryStoreBase>(store) == nullptr)
		{
			store->mark_prepared_for_game();
		}
	}

	if (!result)
	{
#ifdef AN_DEVELOPMENT
		if (is_preview_game())
		{
			output(TXT("library was not properly prepared for game"));
		}
		else
#endif
		{
			error(TXT("library was not properly prepared for game"));
		}
	}

	libraryState = LibraryState::Idle;

#ifdef AN_DEVELOPMENT
	an_assert(should_ignore_preparing_problems() || result);
	return should_ignore_preparing_problems() || result;
#else
	an_assert(result);
	return result;
#endif
}

bool Library::reload_from_file(String const & _fileName, LibrarySource::Type _source, LibraryStored* _storedObject)
{
	LibraryLoadingContext lc = LibraryLoadingContext(this, IO::Utils::get_directory(_fileName), _source, LibraryLoadingMode::Load);
	lc.reload_only(_storedObject);
	return load_from_file(_fileName, _source, lc);
}

bool Library::reload_from_file(String const & _fileName, LibrarySource::Type _source, LibraryStoredReloadRequiredCheck _reload_required_check)
{
	LibraryLoadingContext lc = LibraryLoadingContext(this, IO::Utils::get_directory(_fileName), _source, LibraryLoadingMode::Load);
	lc.reload_only(_reload_required_check);
	return load_from_file(_fileName, _source, lc);
}

bool Library::prepare_for_game_during_async(LibraryStored* _storedObject)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	return prepare_for_game(_storedObject);
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool Library::prepare_for_game_dev(LibraryStored* _storedObject)
{
	return prepare_for_game(_storedObject);
}
#endif

bool Library::prepare_for_game(LibraryStored* _storedObject)
{
	bool result = true;

	LibraryPrepareContext pfgContext;
	while (pfgContext.start_next_requested_level())
	{
		result = _storedObject->prepare_for_game(this, pfgContext) && result;

		// wait for all asynchronous jobs to end, do all available async worker jobs
		while (preparingAsynchronousJobCount != 0)
		{
			if (get_game()->get_job_system()->get_asynchronous_list(get_game()->preparing_worker_jobs())->execute_job_if_available(nullptr))
			{
				continue;
			}
			::System::Core::sleep(0.001f);
		}
	}

	if (!result)
	{
#ifdef AN_DEVELOPMENT
		if (is_preview_game())
		{
			output(TXT("library was not properly prepared for game"));
		}
		else
#endif
		{
			error(TXT("library was not properly prepared for game"));
		}
	}

	an_assert(result);

	return result;
}

struct PrepareStoredObjectData
: public Concurrency::AsynchronousJobData
{
	LibraryStored* storedObject;
	Library* library;
	LibraryPrepareContext& pfgContext;
	PrepareStoredObjectData(LibraryStored* _storedObject, Library* _library, LibraryPrepareContext& _pfgContext)
	: storedObject(_storedObject)
	, library(_library)
	, pfgContext(_pfgContext)
	{}
};

void prepare_stored_object_for_game(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	PrepareStoredObjectData * prepareStoredObjectData = (PrepareStoredObjectData*)_data;
	prepareStoredObjectData->pfgContext.ready_for_next_object();
	scoped_call_stack_info_str_printf(TXT("preparing \"%S\" [%S]"),
		prepareStoredObjectData->storedObject->get_name().to_string().to_char(),
		prepareStoredObjectData->storedObject->get_library_type().to_char());
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
	::System::TimeStamp prepareSingleTimeStamp;
#endif
#endif
	if (! prepareStoredObjectData->storedObject->does_require_further_loading())
	{
		if (prepareStoredObjectData->storedObject->prepare_for_game(prepareStoredObjectData->library, prepareStoredObjectData->pfgContext))
		{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
			output(TXT("prepared [%04i] %S (%.3fms)..."), prepareStoredObjectData->pfgContext.get_current_level(), prepareStoredObjectData->storedObject->get_name().to_string().to_char(), prepareSingleTimeStamp.get_time_since() * 1000.0f);
#endif
#endif
#endif
			if (!prepareStoredObjectData->pfgContext.does_current_object_require_more_work())
			{
				prepareStoredObjectData->storedObject->mark_prepared_for_game();
			}
		}
		else
		{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
			output(TXT("failed to prepare [%04i] %S (%.3fms)..."), prepareStoredObjectData->pfgContext.get_current_level(), prepareStoredObjectData->storedObject->get_name().to_string().to_char(), prepareSingleTimeStamp.get_time_since() * 1000.0f);
#endif
#endif
#endif
			error(TXT("error preparing %S (mt)"), prepareStoredObjectData->storedObject->get_name().to_string().to_char());
			prepareStoredObjectData->pfgContext.mark_fail();
		}
	}
	prepareStoredObjectData->library->preparing_asynchronous_job_done();
}

struct PrepareAsyncData
: public Concurrency::AsynchronousJobData
{
	Library* library;
	LibraryPrepareContext& pfgContext;
	PrepareAsyncData(Library* _library, LibraryPrepareContext& _pfgContext)
	: library(_library)
	, pfgContext(_pfgContext)
	{}
};

void prepare_globals_for_game_from_data(PrepareAsyncData* _prepareAsyncData)
{
	_prepareAsyncData->pfgContext.ready_for_next_object();
	{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
		::System::TimeStamp prepareSingleTimeStamp;
#endif
#endif
		if (LocalisedStrings::prepare_for_game(_prepareAsyncData->library, _prepareAsyncData->pfgContext))
		{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
			output(TXT("prepared localised strings [%04i] (%.3fms)..."), _prepareAsyncData->pfgContext.get_current_level(), prepareSingleTimeStamp.get_time_since() * 1000.0f);
#endif
#endif
#endif
		}
		else
		{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
			output(TXT("failed to prepare localised strings [%04i] (%.3fms)..."), _prepareAsyncData->pfgContext.get_current_level(), prepareSingleTimeStamp.get_time_since() * 1000.0f);
#endif
#endif
#endif
#ifdef AN_DEVELOPMENT
			if (is_preview_game())
			{
				output(TXT("error preparing global references"));
			}
			else
#endif
			{
				error(TXT("error preparing global references"));
			}
			_prepareAsyncData->pfgContext.mark_fail();
		}
	}
	{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
		::System::TimeStamp prepareSingleTimeStamp;
#endif
#endif
		if (_prepareAsyncData->library->access_global_references().prepare_for_game(_prepareAsyncData->library, _prepareAsyncData->pfgContext))
		{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
			output(TXT("prepared global references [%04i] (%.3fms)..."), _prepareAsyncData->pfgContext.get_current_level(), prepareSingleTimeStamp.get_time_since() * 1000.0f);
#endif
#endif
#endif
		}
		else
		{
#ifdef AN_CHECK_MEMORY_LEAKS
#ifdef AN_DEBUG
#ifdef AN_LOG_LOADING_AND_PREPARING
			output(TXT("failed to prepare global references [%04i] (%.3fms)..."), _prepareAsyncData->pfgContext.get_current_level(), prepareSingleTimeStamp.get_time_since() * 1000.0f);
#endif
#endif
#endif
#ifdef AN_DEVELOPMENT
			if (is_preview_game())
			{
				output(TXT("error preparing global references"));
			}
			else
#endif
			{
				error(TXT("error preparing global references"));
			}
			_prepareAsyncData->pfgContext.mark_fail();
		}
	}
}

void prepare_globals_for_game_from_job(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	PrepareAsyncData* prepareAsyncData = (PrepareAsyncData*)_data;
	prepare_globals_for_game_from_data(prepareAsyncData);
	prepareAsyncData->library->preparing_asynchronous_job_done();
	delete prepareAsyncData;
}

bool Library::prepare_globals_for_game(LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	PrepareAsyncData* prepareAsyncData = new PrepareAsyncData(this, _pfgContext);

	if (can_prepare_level_multithreaded(_pfgContext.get_current_level()))
	{
		preparing_asynchronous_job_added();
		Game::preparing_worker_job(get_game(), prepare_globals_for_game_from_job, prepareAsyncData);
	}
	else
	{
		prepare_globals_for_game_from_data(prepareAsyncData);
		delete prepareAsyncData;
	}

	return result;
}

void prepare_sprite_texture_processor_from_data(PrepareAsyncData* _prepareAsyncData)
{
	_prepareAsyncData->pfgContext.ready_for_next_object();

	_prepareAsyncData->library->access_sprite_texture_processor().prepare_actual_textures(); // loading into textures will be handled by the textures
}

void prepare_sprite_texture_processor_from_job(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	PrepareAsyncData* prepareAsyncData = (PrepareAsyncData*)_data;
	prepare_sprite_texture_processor_from_data(prepareAsyncData);
	prepareAsyncData->library->preparing_asynchronous_job_done();
	delete prepareAsyncData;
}

bool Library::prepare_sprite_texture_processor(LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	PrepareAsyncData* prepareGlobalsData = new PrepareAsyncData(this, _pfgContext);

	if (can_prepare_level_multithreaded(_pfgContext.get_current_level()))
	{
		preparing_asynchronous_job_added();
		Game::preparing_worker_job(get_game(), prepare_sprite_texture_processor_from_job, prepareGlobalsData);
	}
	else
	{
		prepare_sprite_texture_processor_from_data(prepareGlobalsData);
		delete prepareGlobalsData;
	}

	return result;
}

bool Library::prepare_for_game(LibraryStored* _storedObject, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (can_prepare_level_multithreaded(_pfgContext.get_current_level()))
	{
		PrepareStoredObjectData * prepareStoredObjectData = new PrepareStoredObjectData(_storedObject, this, _pfgContext);
		preparing_asynchronous_job_added();
		Game::preparing_worker_job(get_game(), prepare_stored_object_for_game, prepareStoredObjectData);
	}
	else if (! _storedObject->does_require_further_loading())
	{
		_pfgContext.ready_for_next_object();
		if (_storedObject->prepare_for_game(this, _pfgContext))
		{
			if (!_pfgContext.does_current_object_require_more_work())
			{
				if (!_storedObject->is_prepared_for_game())
				{
					_storedObject->mark_prepared_for_game();
				}
			}
		}
		else
		{
#ifdef AN_DEVELOPMENT
			if (!Framework::Library::may_ignore_missing_references())
#endif
			{
				error(TXT("error preparing (%S) %S"), _storedObject->get_library_type().to_char(), _storedObject->get_name().to_string().to_char());
				_pfgContext.mark_fail();
			}
		}
	}

	return result;
}

void Library::add_defaults()
{
	FragmentShader::add_defaults(this);
	VertexShader::add_defaults(this);
	Texture::add_defaults(this);
	Material::add_defaults(this);
}

bool Library::load_from_file(String const & _file, LibrarySource::Type _source, LibraryLoadLevel::Type _libraryLoadLevel, OPTIONAL_ LibraryLoadSummary* _summary)
{
	scoped_call_stack_info_str_printf(TXT("load from file \"%S\""), _file.to_char());
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("load from file \"%S\""), _file.to_char());
#endif
	bool result = true;
	set_current_library_load_level(_libraryLoadLevel);
	if (MainSettings::global().should_allow_tools())
	{
		result &= load_from_file_internal(this, _file, _source, LibraryLoadingMode::Tools, _summary);
	}
	result &= load_from_file_internal(this, _file, _source, LibraryLoadingMode::Load, _summary);
	set_current_library_load_level(_libraryLoadLevel + 1); // by default mark that all following objects will have slightly higher load level
	return result;
}

bool Library::load_from_directory(String const & _dir, LibrarySource::Type _source, LibraryLoadLevel::Type _libraryLoadLevel, OPTIONAL_ LibraryLoadSummary* _summary)
{
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("load from directory \"%S\""), _dir.to_char());
#endif
	bool result = true;
	set_current_library_load_level(_libraryLoadLevel);
	if (MainSettings::global().should_allow_tools())
	{
		result &= load_from_directory_internal(this, _dir, _source, LibraryLoadingMode::Tools, _summary);
	}
	result &= load_from_directory_internal(this, _dir, _source, LibraryLoadingMode::Load, _summary);
	set_current_library_load_level(_libraryLoadLevel + 1); // by default mark that all following objects will have slightly higher load level
	return result;
}

bool Library::load_global_settings_from_directory(String const & _dir, LibrarySource::Type _source)
{
	return load_from_directory_internal(nullptr, _dir, _source, LibraryLoadingMode::Settings, nullptr);
}

bool Library::load_tweaks_from_file(String const & _fileName, LibrarySource::Type _source)
{
	an_assert(_source == LibrarySource::Files);
	if (!IO::File::does_exist(IO::get_user_game_data_directory() + _fileName))
	{
		// just ignore
		return true;
	}
	LibraryLoadingContext lc = LibraryLoadingContext(this, IO::Utils::get_directory(_fileName), _source, LibraryLoadingMode::Tweaks);
	return load_from_file(_fileName, _source, lc);
}

#ifdef AN_ALLOW_BULLSHOTS
bool Library::load_bullshots_from_file(String const & _fileName, LibrarySource::Type _source)
{
	an_assert(_source == LibrarySource::Files);
	if (!IO::File::does_exist(IO::get_user_game_data_directory() + _fileName))
	{
		// just ignore
		return true;
	}
	LibraryLoadingContext lc = LibraryLoadingContext(this, IO::Utils::get_directory(_fileName), _source, LibraryLoadingMode::Bullshots);
	return load_from_file(_fileName, _source, lc);
}
#endif

bool Library::load_global_config_from_file(String const & _fileName, LibrarySource::Type _source)
{
	if (_source == LibrarySource::Assets &&
		IO::AssetFile::does_exist(IO::get_asset_data_directory() + _fileName))
	{
		output(TXT("loading \"%S\" config file"), _fileName.to_char());
		LibraryLoadingContext lc = LibraryLoadingContext(nullptr, IO::Utils::get_directory(_fileName), _source, LibraryLoadingMode::Config);
		return load_from_file(_fileName, _source, lc);
	}
	_source = LibrarySource::Files; // force files if no assets
	if (!IO::File::does_exist(IO::get_user_game_data_directory() + _fileName))
	{
		warn(TXT("no \"%S\" config file"), _fileName.to_char());
		// just ignore
		return true;
	}
	output(TXT("loading \"%S\" config file"), _fileName.to_char());
	LibraryLoadingContext lc = LibraryLoadingContext(nullptr, IO::Utils::get_directory(_fileName), _source, LibraryLoadingMode::Config);
	return load_from_file(_fileName, _source, lc);
}

bool Library::load_from_directories(Array<String> const & _dirs, LibrarySource::Type _source, LibraryLoadLevel::Type _libraryLoadLevel, OPTIONAL_ LibraryLoadSummary* _summary)
{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	::System::TimeStamp loadTimeStamp;
	output_colour(0, 0, 0, 1);
	output(TXT("load from directories - begin"));
	output_colour();
#endif
	bool result = true;
	for_every(dir, _dirs)
	{
		if (!load_from_directory(*dir, _source, _libraryLoadLevel, _summary))
		{
			error(TXT("error loading from directory \"%S\""), dir->to_char());
			result = false;
		}
	}
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	output_load_info();
	{
		Concurrency::ScopedSpinLock lock(libraryLoadInfoLock);
		libraryLoadInfo.clear();
	}
	output_colour(0, 0, 0, 1);
	output(TXT("loaded from directories in %.3f"), loadTimeStamp.get_time_since());
	output(TXT("load from directories - end"));
	output_colour();
#endif
	return result;
}

bool Library::load_global_settings_from_directories(Array<String> const & _dirs, LibrarySource::Type _source)
{
	bool result = true;
	for_every(dir, _dirs)
	{
		result &= load_global_settings_from_directory(*dir, _source);
	}
	return result;
}

struct LoadFromFileData
: public Concurrency::AsynchronousJobData
{
	Library* library;
	String filename;
	LibrarySource::Type source;
	LibraryLoadingContext lc;
	Concurrency::Counter & failCount;
	LibraryLoadSummary* summary;
	LoadFromFileData(Library* _library, String const & _filename, LibrarySource::Type _source, LibraryLoadingContext const & _lc, REF_ Concurrency::Counter & _failCount, LibraryLoadSummary* _summary)
	: library(_library)
	, filename(_filename)
	, source(_source)
	, lc(_lc)
	, failCount(_failCount)
	, summary(_summary)
	{}
};

bool Library::load_from_file_internal(Library * _forLibrary, String const & _file, LibrarySource::Type _source, LibraryLoadingMode::Type _loadingMode, OPTIONAL_ LibraryLoadSummary* _summary)
{
	scoped_call_stack_info_str_printf(TXT("load from file internal \"%S\""), _file.to_char());

	Concurrency::ScopedCounter libLoadCount(_forLibrary? &_forLibrary->libraryLoading : nullptr);

	LibraryLoadingContext lc = LibraryLoadingContext(_forLibrary, IO::Utils::get_directory(_file), _source, _loadingMode);

	bool result = true;

	Concurrency::Counter failCount;

	if (_forLibrary && _forLibrary->get_game() && !_forLibrary->get_game()->is_running_single_threaded())
	{
		LoadFromFileData * loadFromFileData = new LoadFromFileData(_forLibrary, _file, _source, lc, failCount, _summary);
		_forLibrary->loading_asynchronous_job_added();
		Game::loading_worker_job(_forLibrary->get_game(), load_from_file_worker, loadFromFileData);
	}
	else
	{
		result &= load_from_file(_file, _source, lc, _summary);
	}

	if (failCount > 0)
	{
		result = false;
	}

	return result;
}

bool Library::load_from_directory_internal(Library * _forLibrary, String const & _dir, LibrarySource::Type _source, LibraryLoadingMode::Type _loadingMode, OPTIONAL_ LibraryLoadSummary* _summary)
{
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("load from directory internal \"%S\""), _dir.to_char());
#endif

	Concurrency::ScopedCounter libLoadCount(_forLibrary ? &_forLibrary->libraryLoading : nullptr);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (! should_load_directory(_dir))
	{
		return true;
	}
#endif

	bool result = true;

	Concurrency::Counter failCount;

	result &= load_from_subdirectory_internal(_forLibrary, _dir, _source, _loadingMode, REF_ failCount, _summary);

	if (_forLibrary)
	{
		_forLibrary->wait_till_loaded();
	}

	if (failCount > 0)
	{
		error(TXT("number of problems while loading: %i"), failCount.get());
		result = false;
	}

	return result;
}

void Library::wait_till_loaded()
{
	if (auto* game = get_game())
	{
		if (!game->is_running_single_threaded() && loadingAsynchronousJobCount != 0)
		{
#ifdef AN_DEBUG_LOAD_LIBRARY
			output(TXT("waiting till loaded"));
#endif
			while (loadingAsynchronousJobCount != 0)
			{
				// allow background jobs to perform
				{
					auto* asyncList = game->get_job_system()->get_asynchronous_list(game->game_background_jobs());
					if (asyncList->execute_job_if_available(nullptr))
					{
						continue;
					}
				}
				{
					auto* asyncList = game->get_job_system()->get_asynchronous_list(game->background_jobs());
					if (asyncList->execute_job_if_available(nullptr))
					{
						continue;
					}
				}
				// if we have more than that, leave the job for them and don't block the main thread
				if (game->get_job_system_info().loadingWorkerJobsCount <= 4)
				{
					auto* asyncList = game->get_job_system()->get_asynchronous_list(game->loading_worker_jobs());
					if (asyncList->execute_job_if_available(nullptr))
					{
						continue;
					}
				}
				::System::Core::sleep(0.001f);
			}
#ifdef AN_DEBUG_LOAD_LIBRARY
			output(TXT("waiting till loaded [done]"));
#endif
		}
	}
}

void Library::load_from_file_worker(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	LoadFromFileData * loadFromFileData = (LoadFromFileData*)_data;
	if (!load_from_file(loadFromFileData->filename, loadFromFileData->source, loadFromFileData->lc, loadFromFileData->summary))
	{
		loadFromFileData->failCount.increase();
	}
	loadFromFileData->library->loading_asynchronous_job_done();
}

bool Library::load_from_subdirectory_internal(Library * _forLibrary, String const & _dir, LibrarySource::Type _source, LibraryLoadingMode::Type _loadingMode, REF_ Concurrency::Counter & _failCount, OPTIONAL_ LibraryLoadSummary* _summary) // if no library, only settings are loaded
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	{
		Concurrency::ScopedMRSWLockRead lock(skipLoadingLock);
		bool load = true;
		String dir = IO::unify_filepath(_dir);
		for_every(slf, skipLoadingFolder)
		{
			if (String::does_contain_icase(dir.to_char(), slf->to_char()))
			{
				load = false;
				break;
			}
		}
		if (!load)
		{
			return true;
		}
	}
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (!should_load_directory(_dir))
	{
		return true;
	}
#endif

#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("load from subdirectory \"%S\""), _dir.to_char());
#endif

	IO::AssetDir assetDir;
	IO::Dir dir;
#ifndef AN_ANDROID
	_source = LibrarySource::Files; // force files!
#endif

	LibraryLoadingContext lc = LibraryLoadingContext(_forLibrary, _dir, _source, _loadingMode);

	bool dirOk = false;
	if (_source == LibrarySource::Files)
	{
		dirOk = dir.list(_dir);
	}
	else if (_source == LibrarySource::Assets)
	{
		dirOk = assetDir.list(_dir);
	}
	else
	{
		todo_important(TXT("implement!"));
	}
	if (dirOk)
	{
		bool result = true;
		Array<String> const & directories = _source == LibrarySource::Files? dir.get_directories() : assetDir.get_directories();
		for_every(directory, directories)
		{
			result &= load_from_subdirectory_internal(_forLibrary, _dir + TXT("/") + *directory, _source, _loadingMode, REF_ _failCount, _summary);
		}

		Array<String> const & files = _source == LibrarySource::Files ? dir.get_files() : assetDir.get_files();
		for_every(file, files)
		{
			if (_forLibrary && _forLibrary->get_game() && !_forLibrary->get_game()->is_running_single_threaded())
			{
				LoadFromFileData * loadFromFileData = new LoadFromFileData(_forLibrary, _dir + IO::get_directory_separator() + *file, _source, lc, _failCount, _summary);
				_forLibrary->loading_asynchronous_job_added();
				Game::loading_worker_job(_forLibrary->get_game(), load_from_file_worker, loadFromFileData);
			}
			else
			{
				result &= load_from_file(_dir + IO::get_directory_separator() + *file, _source, lc, _summary);
			}
		}

		return result;
	}
	return false;
}
		
bool Library::load_from_file(String const & _fileName, LibrarySource::Type _source, LibraryLoadingContext & _lc, OPTIONAL_ LibraryLoadSummary* _summary)
{
	scoped_call_stack_info_str_printf(TXT("load from file \"%S\""), _fileName.to_char());

#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("load from file \"%S\""), _fileName.to_char());
#endif

	Concurrency::ScopedCounter libLoadCount(_lc.get_library() ? &_lc.get_library()->libraryLoading : nullptr);

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	::System::TimeStamp loadTimeStamp;
#endif
		
#ifndef AN_ANDROID
	_source = LibrarySource::Files; // force files!
#endif

	IO::XML::Document* doc = nullptr;

	if (String::compare_icase(_fileName.get_sub(-3), TXT(".cx")))
	{
		if (_lc.get_loading_mode() == LibraryLoadingMode::Tools ||
			_lc.get_loading_mode() == LibraryLoadingMode::Tweaks ||
#ifdef AN_ALLOW_BULLSHOTS
			_lc.get_loading_mode() == LibraryLoadingMode::Bullshots ||
#endif
			_lc.get_loading_mode() == LibraryLoadingMode::Config)
		{
			// ignore this file as we're loading tools and tweaks and they should have xml always
			return true;
		}

		if (_source == LibrarySource::Files)
		{
			// we should reimport xml file
			if (MainSettings::global().should_force_reimporting())
			{
				return true;
			}

			// allow loading cx file but only if there is no corresponding xml file!
			String xmlFileName = _fileName.get_left(_fileName.get_length() - 3) + TXT(".xml");
			if (IO::File::does_exist(IO::get_user_game_data_directory() + xmlFileName))
			{
				// there is xml file, try loading it
				return true;
			}
			
			{
				scoped_call_stack_info(TXT("load cx file (files)"));
				doc = IO::XML::Document::from_cx_file(IO::get_user_game_data_directory() + _fileName);
			}
		}
		else if (_source == LibrarySource::Assets)
		{
			scoped_call_stack_info(TXT("load cx file (assets)"));
			doc = IO::XML::Document::from_cx_asset_file(IO::get_asset_data_directory() + _fileName);
		}
		else
		{
			todo_important(TXT("implement!"));
		}

		if (!doc)
		{
			// we tried .cx file but it was older? or maybe we didn't want to load it for other reason, we're good with not loading it, though
			return true;
		}
	}
	else
	{
		if (!String::compare_icase(_fileName.get_sub(-4), TXT(".xml")))
		{
			// that's fine, just ignore
			return true;
		}

		if (_lc.get_loading_mode() == LibraryLoadingMode::Tools)
		{
			if (IO::Utils::get_file(_fileName) != TXT("_tools.xml"))
			{
				// ignore other files
				return true;
			}
		}
		if (_lc.get_loading_mode() == LibraryLoadingMode::Tweaks)
		{
			if (IO::Utils::get_file(_fileName) != TXT("_tweaks.xml"))
			{
				// ignore other files
				return true;
			}
		}
#ifdef AN_ALLOW_BULLSHOTS
		if (_lc.get_loading_mode() == LibraryLoadingMode::Bullshots)
		{
			if (IO::Utils::get_file(_fileName) != TXT("_bullshots.xml"))
			{
				// ignore other files
				return true;
			}
		}
#endif
		else if (_lc.get_loading_mode() == LibraryLoadingMode::Load)
		{
			if (IO::Utils::get_file(_fileName) == TXT("_tools.xml") ||
				IO::Utils::get_file(_fileName) == TXT("_tweaks.xml") ||
				IO::Utils::get_file(_fileName) == TXT("_bullshots.xml"))
			{
				// ignore tools file
				return true;
			}
		}

		bool useCx = true;

		if (IO::Utils::get_file(_fileName) == TXT("_tools.xml") ||
			IO::Utils::get_file(_fileName) == TXT("_tweaks.xml") ||
			IO::Utils::get_file(_fileName) == TXT("_bullshots.xml") ||
			_lc.get_loading_mode() == LibraryLoadingMode::Config)
		{
			useCx = false;
		}

		// first try loading .cx file to check if it is up to date, if not, load document and save as cx file

		String cxFileName = _fileName.get_left(_fileName.get_length() - 4) + TXT(".cx");
		if (_source == LibrarySource::Files)
		{
			String fileNameInUserGameData = IO::get_user_game_data_directory() + _fileName;
			String cxFileNameInUserGameData = IO::get_user_game_data_directory() + cxFileName;
			if (useCx && IO::File::does_exist(cxFileNameInUserGameData) && !MainSettings::global().should_force_reimporting())
			{
				scoped_call_stack_info(TXT("load cx file (prefer assets' cx over files' xml?)"));
				doc = IO::XML::Document::from_cx_file(cxFileNameInUserGameData);
				if (doc && !doc->check_if_actual(IO::File(fileNameInUserGameData).temp_ptr(), IO::Digested())) // zero digested definition
				{
					// recreate from xml file
					delete_and_clear(doc);
				}
			}

			if (!doc)
			{
				scoped_call_stack_info(TXT("load xml file (files)"));
				doc = IO::XML::Document::from_xml_file(fileNameInUserGameData);
				if (useCx && doc)
				{
					scoped_call_stack_info(TXT("serialise to cx"));
					doc->digest_source(IO::File(fileNameInUserGameData).temp_ptr());
					doc->serialise_resource_to_file(cxFileNameInUserGameData);
				}
			}
		}
		if (_source == LibrarySource::Assets)
		{
			String fileNameInAssetData = IO::get_asset_data_directory() + _fileName;
			String cxFileNameInAssetData = IO::get_asset_data_directory() + cxFileName;
			if (useCx && IO::AssetFile::does_exist(cxFileNameInAssetData) && !MainSettings::global().should_force_reimporting())
			{
				scoped_call_stack_info(TXT("load cx file (prefer assets' cx over assets' xml)"));
				doc = IO::XML::Document::from_cx_asset_file(cxFileNameInAssetData);
				// should always be up to date
			}
			if (!doc)
			{
				scoped_call_stack_info(TXT("load xml file (assets)"));
				doc = IO::XML::Document::from_xml_asset_file(fileNameInAssetData);
			}
		}
	}

	if (! doc)
	{
		error(TXT("invalid xml file \"%S\" (doc)"), _fileName.to_char());
		return false;
	}

	IO::XML::Node* root = doc->get_root();
	if (! root)
	{
		error(TXT("invalid xml file \"%S\" (root)"), _fileName.to_char());
		return false;
	}

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
	{
		float timeTaken = loadTimeStamp.get_time_since();
		store_library_loading_time(_lc.library, NAME(loadingXML), timeTaken);
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		output(TXT("  %c %8.3fms <loaded< %S"), timeTaken >= 0.1f ? '@' : (timeTaken >= 0.01f ? '^' : ' '), timeTaken * 1000.0f, _fileName.to_char());
#endif
	}
#endif

	bool result;
	{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		::System::TimeStamp parseTimeStamp;
#endif		
		scoped_call_stack_info(TXT("load from xml structure"));
		_lc.set_current_file(IO::Utils::get_file(_fileName));
		result = load_from_xml(root, _lc, _fileName, _summary);
		_lc.set_current_file();
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		{
			float timeTaken = parseTimeStamp.get_time_since();
			store_library_loading_time(_lc.library, NAME(parsing), timeTaken);
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
			output(TXT("  %c %8.3fms @parsed@ %S"), timeTaken >= 0.1f? '@' : (timeTaken >= 0.01f? '^' : ' '), timeTaken * 1000.0f, _fileName.to_char());
#endif
		}
#endif
	}
	if (!result)
	{
		error(TXT("error loading file \"%S\""), _fileName.to_char());
	}

	delete doc;

	return result;
}

bool Library::load_special_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc, String const& _fileName, OPTIONAL_ LibraryLoadSummary* _summary, OUT_ bool& _loadResult)
{
	if (_node->get_name() == TXT("library"))
	{
		Name pushedGroup = _lc.get_current_group();
		if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("group")))
		{
			_lc.set_current_group(Name(attr->get_as_string()));
		}
		for_every(node, _node->all_children())
		{
			load_from_xml(node, _lc, _fileName, _summary);
		}
		_lc.set_current_group(pushedGroup);
		_loadResult = true;
		return true;
	}
	if (_node->get_name() == TXT("libraryLoadSetupExclude") ||
		_node->get_name() == TXT("libraryLoadSetupInclude"))
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		load_library_load_setup(_node);
#endif
		_loadResult = true; 
		return true;
	}
	if (_node->get_name() == TXT("preview") ||
		_node->get_name() == TXT("previewGame") ||
		_node->get_name() == TXT("previewOnly") ||
		_node->get_name() == TXT("previewGameOnly"))
	{
		if (is_preview_game())
		{
			for_every(node, _node->all_children())
			{
				load_from_xml(node, _lc, _fileName, _summary);
			}
		}
		_loadResult = true; 
		return true;
	}
	if (_node->get_name() == TXT("previewSetup"))
	{
		// ignore, preview game handles it
		_loadResult = true; 
		return true;
	}
	if (_node->get_name() == TXT("localisedStrings") && !_lc.is_reloading_only())
	{
		// do not ignore errors
		_loadResult = LocalisedStrings::load_from_xml(_node, _lc);
		return true;
	}
	if (_node->get_name() == TXT("globalReferences") && !_lc.is_reloading_only())
	{
		// do not ignore errors
		_loadResult = _lc.get_library()->globalReferences.load_from_xml(_node, _lc);
		return true;
	}
	if ((_node->get_name() == TXT("wheresMyPointFunction") ||
			_node->get_name() == TXT("wheresMyPointTool")) && !_lc.is_reloading_only())
	{
		_loadResult = WheresMyPoint::RegisteredTools::register_custom_from_xml(_node);
		return true;
	}
	// settings, tools etc.
	if (_node->get_name() == TXT("ratingRules") && !_lc.is_reloading_only())
	{
		todo_important(TXT("_lc.library.ratingRules.load_from_xml(_node, _lc);"));
		_loadResult = true;
		return true;
	}
	if (_node->get_name() == TXT("statsRules") && !_lc.is_reloading_only())
	{
		todo_important(TXT("_lc.library.statsRules.load_from_xml(_node, _lc);"));
		_loadResult = true;
		return true;
	}
	if (_node->get_name() == TXT("aiSocialRules") && !_lc.is_reloading_only())
	{
		_loadResult = AI::SocialRules::load_from_xml(_node);
		return true;
	}
	if (_node->get_name() == TXT("lcdLetters") && !_lc.is_reloading_only())
	{
		_loadResult = LCDLetters::load_from_xml(_node);
		return true;
	}
	if (_node->get_name() == TXT("bhapticsSensation") && !_lc.is_reloading_only())
	{
		_loadResult = an_bhaptics::Library::load_from_xml(_node);
		return true;
	}
	if (_node->get_name() == TXT("bullshot") && !_lc.is_reloading_only())
	{
#ifdef AN_ALLOW_BULLSHOTS
		_loadResult = BullshotSystem::load_from_xml(_node, _lc);
		return true;
#else
		return false;
#endif
	}
	if (_node->get_name() == TXT("mainSettings") ||
		_node->get_name() == TXT("mainConfig") ||
		_node->get_name() == TXT("lowerUpperPairs") ||
		_node->get_name() == TXT("primitivesPipeline") ||
		_node->get_name() == TXT("renderingPipeline") ||
		_node->get_name() == TXT("postProcessPipeline") ||
		_node->get_name() == TXT("collisionMasks") ||
		_node->get_name() == TXT("gameInputDefinition") ||
		_node->get_name() == TXT("vrInputDevices") ||
		_node->get_name() == TXT("vrOffsets") ||
		_node->get_name() == TXT("gameConfig") ||
		_node->get_name() == TXT("testConfig") ||
		_node->get_name() == TXT("skipLoading") ||
		_node->get_name() == TXT("foveatedRenderingPresets"))
	{
		// ignore settings
		_loadResult = true;
		return true;
	}
	if (_node->get_name() == TXT("devTools") && !_lc.is_reloading_only())
	{
		todo_important(TXT("DevTools.process_xml(_node);"));
		_loadResult = true;
		return true;
	}
	if (_node->get_name() == TXT("loadMultipleSamples"))
	{
		if (_lc.get_loading_mode() != LibraryLoadingMode::Load)
		{
			error_loading_xml(_node, TXT("loadMultipleSamples should not be loaded in this file \"%S\""), _node->get_name().to_char(), _lc.get_path_in_dir(_lc.get_current_file()).to_char());
			an_assert(false);
			// ignore
			_loadResult = true;
			return true;
		}
		_loadResult = Sample::load_multiple_from_xml(_node, _lc);
		return true;
	}

	return false;
}

bool Library::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, String const & _fileName, OPTIONAL_ LibraryLoadSummary* _summary)
{
	if (!_node)
	{
		return false;
	}
	scoped_call_stack_info_str_printf(TXT(" +- "), _node->get_name().to_char());
	if (_node->get_type() == IO::XML::NodeType::Root)
	{
		scoped_call_stack_info(_fileName.to_char());
		for_every(node, _node->all_children())
		{
			load_from_xml(node, _lc, _fileName, _summary);
		}
		return true;
	}
	else if (_node->get_type() == IO::XML::NodeType::Text ||
			 _node->get_type() == IO::XML::NodeType::Comment ||
			 _node->get_type() == IO::XML::NodeType::NodeCommentedOut)
	{
		return true; // some comment or info?
	}
	if (_node->get_name() == TXT("colour")) // can be both setting and library
	{
		Name name = _node->get_name_attribute(TXT("name"));
		if (name.is_valid())
		{
			Colour colour = Colour::full;
			if (colour.load_from_xml(_node))
			{
				RegisteredColours::register_colour(name, colour);
				return true;
			}
			else
			{
				error_loading_xml(_node, TXT("invalid colour"));
				return false;
			}
		}
		else
		{
			error_loading_xml(_node, TXT("no \"name\" provided for colour"));
			return false;
		}
	}
	else if (_lc.is_loading_for_library())
	{
		// loading library
		if (_node->get_type() == IO::XML::NodeType::Node)
		{
			scoped_call_stack_info(_node->get_name().to_char());
#if LOG
			//Utils.Log.log_filtered(Utils.LogType.Loading, TXT("loading %S"), node.name);
#endif
			bool specialNodeResult = true;
			if (_lc.get_library()->load_special_from_xml(_node, _lc, _fileName, _summary, OUT_ specialNodeResult))
			{
				return specialNodeResult;
			}
			for_every_ptr(store, _lc.get_library()->stores)
			{
				if (store->should_load_from_xml(_node))
				{
					if (! _lc.get_library()->does_allow_loading(store, _lc))
					{
						error_loading_xml(_node, TXT("library object \"%S\" should not be loaded in this file \"%S\""), _node->get_name().to_char(), _lc.get_path_in_dir(_lc.get_current_file()).to_char());
						an_assert(false);
						// ignore
						return true;
					}
					//MEASURE_AND_OUTPUT_PERFORMANCE_MIN(0.005f, TXT("loading from %S [%S] %S"), _fileName.to_char(), store->get_type_name().to_string().to_char(), _node->get_name().to_char());
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED_SINGLE
					::System::TimeStamp parseSingleTimeStamp;
#endif		
					bool result = store->load_from_xml(_node, _lc, LibraryName::invalid(), _summary);
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED_SINGLE
					{
						float timeTaken = parseSingleTimeStamp.get_time_since();
						output(TXT("  %c %8.3fms @parsed-stored@ %S \"%S\" from %S"), timeTaken >= 0.1f ? '@' : (timeTaken >= 0.01f ? '^' : ' '), timeTaken * 1000.0f, _node->get_name().to_char(), _node->get_string_attribute(TXT("name")).to_char(), _fileName.to_char());
					}
#endif
					return result;
				}
			}
			for_every_ptr(tool, _lc.get_library()->tools)
			{
				if (tool->should_run_from_xml(_node) && !_lc.is_reloading_only())
				{
					if (_lc.get_loading_mode() != LibraryLoadingMode::Tools)
					{
						error_loading_xml(_node, TXT("tool \"%S\" should not be loaded in this file \"%S\""), _node->get_name().to_char(), _lc.get_path_in_dir(_lc.get_current_file()).to_char());
						an_assert(false);
						// ignore
						return true;
					}
					return tool->run_from_xml(_node, _lc);
				}
			}
			for_every_ptr(tool, _lc.get_library()->tweakTools)
			{
				if (tool->should_run_from_xml(_node) && !_lc.is_reloading_only())
				{
					if (_lc.get_loading_mode() != LibraryLoadingMode::Tweaks)
					{
						error_loading_xml(_node, TXT("tweak tool \"%S\" should not be loaded in this file \"%S\""), _node->get_name().to_char(), _lc.get_path_in_dir(_lc.get_current_file()).to_char());
						an_assert(false);
						// ignore
						return true;
					}
					return tool->run_from_xml(_node, _lc);
				}
			}
			{
				bool loadResult;
				if (_lc.get_library()->load_gameplay(_node, _lc, _fileName, OUT_ loadResult))
				{
					return loadResult;
				}
			}
			error_loading_xml(_node, TXT("type \"%S\" of loaded object not recognised (in \"%S\")"), _node->get_name().to_char(), _fileName.to_char());
			return false;
		}
		else
		{
			// maybe that's comment or plain text - leave it
			return true;
		}
	}
	else
	{
		// loading settings - allow settings nested in library
		if (_node->get_name() == TXT("library"))
		{
			for_every(node, _node->all_children())
			{
				load_from_xml(node, _lc, _fileName);
			}
			return true;
		}
		if (_node->get_name() == TXT("mainSettings"))
		{
			MainSettingsLoadingOptions loadingOptions;
			if (Framework::is_preview_game())
			{
				loadingOptions.ignoreJobSystemForOtherSystem = true;
			}
			if (MainSettings::access_global().load_from_xml(_node, loadingOptions))
			{
				// depending on settings, pipelines could change
				todo_future(TXT("maybe register them as dependant on MainSettings and MainSettings will automatically modify them?"));
				::System::Pipelines::setup_static();
				Pipelines::setup_static();
				return true;
			}
			else
			{
				error_loading_xml(_node, TXT("problem while loading main settings"));
				return false;
			}
		}
		if (_node->get_name() == TXT("mainConfig"))
		{
			if (!MainConfig::load_to_be_copied_to_main_config_xml(_node))
			{
				error_loading_xml(_node, TXT("problem while loading main config"));
				return false;
			}
			if (!MainConfig::global().check_system_tag_required(_node))
			{
				return true;
			}
			if (MainConfig::access_global().load_from_xml(_node))
			{
				return true;
			}
			else
			{
				error_loading_xml(_node, TXT("problem while loading main config"));
				return false;
			}
		}
		if (_node->get_name() == TXT("lowerUpperPairs"))
		{
			return String::load_lower_upper_pairs_from_xml(_node);
		}
		if (_node->get_name() == TXT("primitivesPipeline"))
		{
			return ::System::PrimitivesPipeline::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("renderingPipeline"))
		{
			return RenderingPipeline::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("postProcessPipeline"))
		{
			return PostProcessPipeline::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("gameInputDefinition"))
		{
			return GameInputDefinitions::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("vrInputDevices"))
		{
			return VR::Input::Devices::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("vrOffsets"))
		{
			return VR::Offsets::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("gameConfig"))
		{
			if (!GameConfig::load_to_be_copied_to_main_config_xml(_node))
			{
				error_loading_xml(_node, TXT("problem while loading main config"));
				return false;
			}
			if (!MainConfig::global().check_system_tag_required(_node))
			{
				return true;
			}
			return GameConfig::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("testConfig"))
		{
			if (!MainConfig::global().check_system_tag_required(_node))
			{
				return true;
			}
			return TestConfig::load_from_xml(_node);
		}
		if (_node->get_name() == TXT("skipLoading"))
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (auto* attr = _node->get_attribute(TXT("folder")))
			{
				if (!attr->get_as_string().is_empty())
				{
					Concurrency::ScopedMRSWLockWrite lock(skipLoadingLock);
					skipLoadingFolder.push_back(IO::unify_filepath(attr->get_as_string()));
				}
			}
			return true;
#endif
		}
		if (_node->get_name() == TXT("collisionMasks"))
		{
			return Collision::DefinedFlags::load_masks_from_xml(_node);
		}
		if (_node->get_name() == TXT("preview") ||
			_node->get_name() == TXT("previewSetup"))
		{
			// ignore, preview game handles it
			return true;
		}
		if (_node->get_name() == TXT("libraryLoadSetupExclude") ||
			_node->get_name() == TXT("libraryLoadSetupInclude"))
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			load_library_load_setup(_node);
#endif
			return true;
		}
		if (_node->get_name() == TXT("foveatedRenderingPresets"))
		{
			if (::System::FoveatedRenderingSetup::load_from_xml(_node))
			{
				return true;
			}
			else
			{
				error_loading_xml(_node, TXT("problem while loading foveated rendering presets"));
				return false;
			}
		}
		error_loading_xml(_node, TXT("doesn't know how to handle \"%S\""), _node->get_name().to_char());
		return false;
	}
}

bool Library::does_allow_loading(LibraryStoreBase* _store, LibraryLoadingContext& _lc) const
{
#ifdef AN_ALLOW_BULLSHOTS
	if (_lc.get_loading_mode() == Framework::LibraryLoadingMode::Bullshots &&
		_store == gameScripts)
	{
		return true;
	}
#endif
	return _lc.get_loading_mode() == LibraryLoadingMode::Load;
}

bool Library::create_standalone_object_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, OUT_ RefCountObjectPtr<LibraryStored> & _object)
{
	for_every_ptr(store, _lc.get_library()->stores)
	{
		if (store->should_load_from_xml(_node))
		{
			return store->create_standalone_object_from_xml(_node, _lc, OUT_ _object);
		}
	}

	// we didn't recognise but that's ok, let the caller find proper way to handle this
	_object = nullptr;
	return true;
}

LibraryStored * Library::find_stored(LibraryName const & _name, Name const & _ofType) const
{
	// TODO find if _ofType is invalid
	for_every_ptr(store, stores)
	{
		if (! _ofType.is_valid() || store->does_type_name_match(_ofType))
		{
			if (LibraryStored * found = store->find_stored(_name))
			{
				return found;
			}
		}
	}
	return nullptr;
}

void Library::add_tool(LibraryTool* _tool)
{
	tools.push_back(_tool);
}

void Library::add_tweak_tool(LibraryTool* _tool)
{
	tweakTools.push_back(_tool);
}

void Library::add_store(LibraryStoreBase * _store)
{
	stores.push_back(_store);
}

LibraryStoreBase * Library::get_store(Name const & _ofType) const
{
	if (!_ofType.is_valid())
	{
		return nullptr;
	}
	// TODO find if _ofType is invalid
	for_every_ptr(store, stores)
	{
		if (store->does_type_name_match(_ofType))
		{
			return store;
		}
	}
	return nullptr;
}

bool Library::is_type_combined(Name const & _type) const
{
	return combinedStores.has_key(_type);
}

void Library::do_for_type(Name const & _type, DoForLibraryType _do_for_library_type)
{
	if (_do_for_library_type(_type))
	{
		return;
	}
	if (is_type_combined(_type))
	{
		if (CombinedLibraryStoreBase** combined = combinedStores.get_existing(_type))
		{
			(*combined)->do_for_type(_do_for_library_type);
		}
	}
}
		
void Library::do_for_every_of_type(Name const & _type, DoForLibraryStored _do_for_library_stored)
{
	for_every_ptr(store, stores)
	{
		// skip combined stores, we're looking for particular type (if we're looking for combined, the next step will cover it)
		if (!fast_cast<CombinedLibraryStoreBase>(store))
		{
			if (store->does_type_name_match(_type))
			{
				store->do_for_every(_do_for_library_stored);
			}
		}
	}
	if (is_type_combined(_type))
	{
		if (CombinedLibraryStoreBase** combined = combinedStores.get_existing(_type))
		{
			(*combined)->do_for_every(_do_for_library_stored);
		}
	}
}

bool Library::can_prepare_level_multithreaded(int _prepareLevel) const
{
	if (!get_game() || get_game()->is_running_single_threaded())
	{
		return false;
	}
	if (_prepareLevel == LibraryPrepareLevel::GenerateMeshes)
	{
		return true; 
	}
	else
	{
		return false;
	}
}

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
void Library::output_load_info()
{
	Concurrency::ScopedSpinLock lock(libraryLoadInfoLock);
	output_colour(0, 0, 0, 1);
	output(TXT("load and parse times - begin"));
	float totalTime = 0.0f;
	for_every(loadInfo, libraryLoadInfo)
	{
		output(TXT("%6.3f : %4i : %7.5f [%7.5f-%7.5f] : %S"), loadInfo->get_total_time(), loadInfo->totalCount, loadInfo->get_average_time(), loadInfo->get_min_time(), loadInfo->get_max_time(), for_everys_map_key(loadInfo).to_char());
		totalTime += loadInfo->get_total_time();
	}
	output(TXT("%6.3f : total time"), totalTime);
	output(TXT("load and parse times - end"));
	output_colour();
}
#endif

void Library::list_all_objects(LogInfoContext & _log) const
{
	_log.set_colour(Colour::grey);
	_log.log(TXT("library - begin"));
	_log.set_colour();
	for_every_ptr(store, stores)
	{
		store->list_all_objects(_log);
	}
	_log.set_colour(Colour::grey);
	_log.log(TXT("library - end"));
	_log.set_colour();

}

void Library::log_short_info(LogInfoContext & _log) const
{
	int count = 0;
	for_every_ptr(store, stores)
	{
		count += store->count_all_objects();
	}
	_log.set_colour(Colour::yellow);
	_log.log(TXT("library - loaded %i objects"), count);
	_log.set_colour();
}

bool Library::load_gameplay(IO::XML::Node const * _node, LibraryLoadingContext & _lc, String const & _fileName, OUT_ bool & _loadResult)
{
	return false;
}

void Library::unload_and_remove(LibraryStored* _libStored)
{
	if (auto * store = get_store(_libStored->get_library_type()))
	{
		_libStored->prepare_to_unload();
		store->remove_stored(_libStored);
	}
}

int Library::remove_unused_temporary_objects(Optional<int> const & _howMany)
{
	int removed = 0;
	while (true)
	{
		bool anyRemoved = false;

		for_every_ptr(store, stores)
		{
			Array<LibraryStored*> toRemove;
			store->do_for_every(
				[&toRemove, _howMany, &removed](LibraryStored* _libraryStored)
			{
				if (_libraryStored->is_temporary() &&
					_libraryStored->get_usage_count() == 0)
				{
					if (! _howMany.is_set() || removed < _howMany.get())
					{
						toRemove.push_back(_libraryStored);
					}
				}
			}
			);
			for_every_ptr(tr, toRemove)
			{
				anyRemoved = true;
				tr->prepare_to_unload();
				store->remove_stored(tr);
				++removed;
			}
			if (_howMany.is_set() && removed >= _howMany.get())
			{
				break;
			}
		}

		if (!anyRemoved || _howMany.is_set())
		{
			break;
		}
	}
	return removed;
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
void Library::setup(DebugVisualiser* _dv)
{
	Font* font = nullptr;
	if (Game::get())
	{
		font = Game::get()->get_default_font();
	}
	if (!font)
	{
		if (!fonts->get_all_stored().is_empty())
		{
			font = fonts->get_all_stored().get_first().get();
		}
	}
	if (font)
	{
		_dv->use_font(font->get_font());
	}
}
#endif

void Library::update_default_texture()
{
	defaultTexture = Texture::get_default(this);
	if (!defaultTexture.get())
	{
		error(TXT("no default texture!"));
	}
}

Texture* Library::get_default_texture() const
{
	return defaultTexture.get();
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool Library::load_library_load_setup(IO::XML::Node const* _node)
{
	if (!s_libraryLoadSetup)
	{
		s_libraryLoadSetup = new LibraryLoadSetup();
	}
	if (_node->get_name() == TXT("libraryLoadSetupExclude"))
	{
		if (auto* a = _node->get_attribute(TXT("path")))
		{
			s_libraryLoadSetup->excludePaths.push_back_unique(IO::unify_filepath(a->get_as_string()));
		}
	}
	if (_node->get_name() == TXT("libraryLoadSetupInclude"))
	{
		if (auto* a = _node->get_attribute(TXT("path")))
		{
			s_libraryLoadSetup->includePaths.push_back_unique(IO::unify_filepath(a->get_as_string()));
		}
	}

	return true;
}

bool Library::should_load_directory(String const& _dir)
{
	if (!s_libraryLoadSetup ||
		(s_libraryLoadSetup->excludePaths.is_empty() &&
		 s_libraryLoadSetup->excludeSuffixDirectory.is_empty()))
	{
		return true;
	}
	String dir = IO::unify_filepath(_dir);
	for_every(sfx, s_libraryLoadSetup->excludeSuffixDirectory)
	{
		if (String::compare_icase(dir.get_right(sfx->get_length()), *sfx))
		{
			return false;
		}
	}
	if (!s_libraryLoadSetup->excludePaths.is_empty())
	{
		int exclude = 0;
		for_every(ex, s_libraryLoadSetup->excludePaths)
		{
			if (dir.does_contain(ex->to_char()))
			{
				exclude = max(exclude, ex->get_length());
			}
		}

		if (!exclude)
		{
			return true;
		}

		int include = 0;
		for_every(ex, s_libraryLoadSetup->includePaths)
		{
			if (dir.does_contain(ex->to_char()))
			{
				include = max(include, ex->get_length());
			}
		}

		return include > 0 && include >= exclude;
	}

	return true;
}

void Library::skip_loading_directory_with_suffix(String const& _suffix)
{
	if (!s_libraryLoadSetup)
	{
		s_libraryLoadSetup = new LibraryLoadSetup();
	}
	s_libraryLoadSetup->excludeSuffixDirectory.push_back_unique(_suffix);
}

bool Library::may_ignore_missing_references()
{
	return !s_libraryLoadSetup->excludePaths.is_empty();
}

#endif
