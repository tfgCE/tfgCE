#include "roomType.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\meshGen\meshGenGenerationContext.h"
#include "..\meshGen\meshGenValueDefImpl.inl"

#include "environmentType.h"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"

//

using namespace Framework;

//

SoftDoorRendering::Type SoftDoorRendering::parse(String const & _string)
{
	if (_string == TXT("everyOdd")) { return SoftDoorRendering::EveryOdd; }
	if (_string == TXT("none")) { return SoftDoorRendering::None; }
	error(TXT("soft door rendering \"%S\" not recognised"), _string.to_char());
	return SoftDoorRendering::None;
}

//

bool RoomType::UseEnvironment::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (IO::XML::Node const * useEnvironmentNode = _node->first_child_named(TXT("useEnvironment")))
	{
		useEnvironmentType.load_from_xml(useEnvironmentNode, TXT("type"), _lc);
		useFromRegion = useEnvironmentNode->get_name_attribute(TXT("name"), useFromRegion);
		useFromFirstDoor = useEnvironmentNode->get_bool_attribute_or_from_child_presence(TXT("fromFirstDoor"), useFromFirstDoor);
		parentEnvironment = useEnvironmentNode->get_name_attribute(TXT("parent"), parentEnvironment);
		placement.load_from_xml_child_node(useEnvironmentNode, TXT("placement"));
		anchorPOI = useEnvironmentNode->get_name_attribute_or_from_child(TXT("anchorPOI"), anchorPOI);
		anchor.load_from_xml(useEnvironmentNode, TXT("anchor"));

		for_every(node, useEnvironmentNode->children_named(TXT("pulse")))
		{
			pulseEnvironmentType.load_from_xml(node, TXT("type"), _lc);
			pulsePeriod = node->get_float_attribute_or_from_child(TXT("period"), pulsePeriod);
		}
	}

	return result;
}

bool RoomType::UseEnvironment::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (useEnvironmentType.is_name_valid())
		{
			if (!useEnvironmentType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve))
			{
				error(TXT("could not find \"%S\" environment type for room type"), useEnvironmentType.get_name().to_string().to_char());
				result = false;
			}
		}
		if (pulseEnvironmentType.is_name_valid())
		{
			if (!pulseEnvironmentType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve))
			{
				error(TXT("could not find \"%S\" environment type for room type"), pulseEnvironmentType.get_name().to_string().to_char());
				result = false;
			}
		}
	}
	return result;
}

bool RoomType::UseEnvironment::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _renamer.apply_to(useEnvironmentType, _library);
	result &= _renamer.apply_to(pulseEnvironmentType, _library);

	return result;
}

//

REGISTER_FOR_FAST_CAST(RoomType);
LIBRARY_STORED_DEFINE_TYPE(RoomType, roomType);

RoomType::RoomType(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, softDoorRendering(SoftDoorRendering::None)
, softDoorRenderingEveryOddOffset(0)
, regionPiece( new PieceCombiner::Piece<RegionGenerator>() )
, useFallbackGeneration(false)
{
	regionPiece->def.roomType.lock(this);
}

void RoomType::prepare_to_unload()
{
	base::prepare_to_unload();
	regionPiece->def.roomType.unlock_and_clear();
}

RoomType::~RoomType()
{
}

bool RoomType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);
	
	for_every(node, _node->children_named(TXT("requiresVariables")))
	{
		result &= requiresVariables.load_from_xml(node);
	}
	
	for_every(node, _node->children_named(TXT("wheresMyPointSetupParameters")))
	{
		result &= wheresMyPointProcessorSetupParameters.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("wheresMyPointPreGenerate")))
	{
		result &= wheresMyPointProcessorPreGenerate.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("wheresMyPointOnGenerate")))
	{
		result &= wheresMyPointProcessorOnGenerate.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("setting")))
	{
		if (!setting.is_set())
		{
			setting = new WorldSetting();
		}
		result &= setting->load_from_xml(node, _lc);
	}

	result &= copySettings.load_from_xml(_node, _lc);

	for_every(sNode, _node->children_named(TXT("settings")))
	{
		for_every(node, sNode->children_named(TXT("setting")))
		{
			if (!setting.is_set())
			{
				setting = new WorldSetting();
			}
			result &= setting->load_from_xml(node, _lc);
		}
	}

	for_every(node, _node->children_named(TXT("doorTypeReplacer")))
	{
		if (!doorTypeReplacer.is_set())
		{
			doorTypeReplacer = new DoorTypeReplacer();
		}
		result &= doorTypeReplacer->load_from_xml(node, _lc);
	}

	result &= collisionInfoProvider.load_from_xml(_node, TXT("collisionInfo"));

	result &= useEnvironment.load_from_xml(_node, _lc);
	for_every(node, _node->children_named(TXT("add")))
	{
		UsedLibraryStored<EnvironmentOverlay> eo;
		if (eo.load_from_xml(node, TXT("environmentOverlay"), _lc))
		{
			environmentOverlays.push_back(eo);
		}
		else
		{
			error_loading_xml(node, TXT("no environmentOverlay"));
			result = false;
		}
	}

	if (auto* node = _node->first_child_named(TXT("useReverb")))
	{
		warn_loading_xml(node, TXT("deprecated, use sound reverb pair"));
		result &= useReverb.load_from_xml(_node->first_child_named(TXT("useReverb")), TXT("type"), _lc);
	}
	result &= useReverb.load_from_xml(_node->first_child_named(TXT("sound")), TXT("reverb"), _lc);

	result &= libraryStoredReplacer.load_from_xml_child_node(_node, _lc);

	{	// region generator
		RegionGenerator::LoadingContext loadingContext(_lc);

		result &= regionPiece->load_from_xml(_node->first_child_named(TXT("asRegionPiece")), &loadingContext);
	}


	for_every(roomGeneratorNode, _node->children_named(TXT("asRoomGenerator")))
	{
		useFallbackGeneration = roomGeneratorNode->get_bool_attribute(TXT("useFallbackGeneration"), useFallbackGeneration);
	}

	result &= roomGeneratorInfo.load_from_xml(_node, TXT("asRoomGenerators"), TXT("asRoomGenerator"), _lc, TXT("piece combiner"));
	// no need to set useFallbackGeneration automatically, we assume fallback generation if can't find room generator

	result &= doorVRCorridor.load_from_xml(_node, _lc);

	if (IO::XML::Node const * node = _node->first_child_named(TXT("softDoorRendering")))
	{
		if (IO::XML::Attribute const * attr = node->get_attribute(TXT("type")))
		{
			softDoorRendering = SoftDoorRendering::parse(attr->get_as_string());
		}
		softDoorRenderingEveryOddOffset = node->get_int_attribute(TXT("offset"), softDoorRenderingEveryOddOffset);
		softDoorRenderingTag = node->get_name_attribute(TXT("tag"), softDoorRenderingTag);
		if (!softDoorRenderingTag.is_valid() &&
			softDoorRendering != SoftDoorRendering::None)
		{
			error_loading_xml(node, TXT("soft door rendering without tag"));
			result = false;
		}
	}

	shouldHaveNavMesh = _node->get_bool_attribute_or_from_child_presence(TXT("hasNavMesh"), shouldHaveNavMesh);
	shouldHaveNavMesh = ! _node->get_bool_attribute_or_from_child_presence(TXT("noNavMesh"), ! shouldHaveNavMesh);

	shouldBeAvoidedToNavigateInto = _node->get_bool_attribute_or_from_child_presence(TXT("dontNavigateInto"), shouldBeAvoidedToNavigateInto);
	shouldBeAvoidedToNavigateInto = ! _node->get_bool_attribute_or_from_child_presence(TXT("allowNavigatingInto"), !shouldBeAvoidedToNavigateInto);

	result &= predefinedRoomOcclusionCulling.load_from_xml_child_node(_node);

	return result;
}

bool RoomType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		copySettings.apply_to(setting);
	}

	if (setting.is_set())
	{
		result &= setting->prepare_for_game(_library, _pfgContext);
	}

	if (doorTypeReplacer.is_set())
	{
		result &= doorTypeReplacer->prepare_for_game(_library, _pfgContext);
	}

	result &= useEnvironment.prepare_for_game(_library, _pfgContext);
	for_every(eo, environmentOverlays)
	{
		result &= eo->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	
	result &= useReverb.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	result &= libraryStoredReplacer.prepare_for_game(_library, _pfgContext);
		
	result &= doorVRCorridor.prepare_for_game(_library, _pfgContext);

	// everything below is just resolving
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		{	// region generator
			RegionGenerator::PreparingContext preparingContext(_library);

			result &= regionPiece->prepare(&preparingContext);
		}

		// setup "use environment"
		result &= useEnvironment.prepare_for_game(_library, _pfgContext);

		// room generators
		result &= roomGeneratorInfo.prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool RoomType::create_from_template(Library* _library, CreateFromTemplate const & _createFromTemplate) const
{
	LibraryName newName = get_name();
	_createFromTemplate.get_renamer().apply_to(newName);
	if (_library->get_room_types().find_may_fail(newName))
	{
		error(TXT("room type \"%S\" already exists"), newName.to_string().to_char());
		return false;
	}

	RoomType* newRoomType = _library->get_room_types().find_or_create(newName);

	bool result = base::fill_on_create_from_template(*newRoomType, _createFromTemplate);

	// copy everything
	newRoomType->softDoorRendering = softDoorRendering;
	newRoomType->softDoorRenderingEveryOddOffset = softDoorRenderingEveryOddOffset;
	newRoomType->softDoorRenderingTag = softDoorRenderingTag;

	newRoomType->useEnvironment = useEnvironment;
	newRoomType->useEnvironment.apply_renamer(_createFromTemplate.get_renamer());

	todo_important(TXT("no create copy yet! and no applying renamers"));
	newRoomType->setting = setting;
	newRoomType->doorTypeReplacer = doorTypeReplacer;

	newRoomType->libraryStoredReplacer = libraryStoredReplacer;
	_createFromTemplate.get_renamer().apply_to(newRoomType->libraryStoredReplacer);

	*newRoomType->regionPiece.get() = *regionPiece.get();
	RegionGenerator::apply_renamer_to(*newRoomType->regionPiece.get(), _createFromTemplate.get_renamer());

	newRoomType->useFallbackGeneration = useFallbackGeneration;
	newRoomType->roomGeneratorInfo = roomGeneratorInfo.create_copy();
	newRoomType->roomGeneratorInfo.apply_renamer(_createFromTemplate.get_renamer(), _library);
	newRoomType->doorVRCorridor = doorVRCorridor.create_copy();
	newRoomType->doorVRCorridor.apply_renamer(_createFromTemplate.get_renamer(), _library);

	newRoomType->collisionInfoProvider = collisionInfoProvider;

	return result;
}

RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> RoomType::get_region_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region* _region, REF_ Random::Generator & _randomGenerator) const
{
	RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> ptr;
	ptr = roomGeneratorInfo.generate_piece_for_region_generator(_generator, _region, REF_ _randomGenerator);
	if (ptr.is_set())
	{
		ptr->def.roomType = this; // otherwise we will never get to generate this room type
	}
	else
	{
		ptr = get_as_region_piece();
	}
	return ptr;
}

RoomGeneratorInfo const* RoomType::get_sole_room_generator_info() const
{
	if (roomGeneratorInfo.get_count() != 1)
	{
		error(TXT("there should be exactly one room generator info"));
		return nullptr;
	}
	return roomGeneratorInfo.get(0);
}
