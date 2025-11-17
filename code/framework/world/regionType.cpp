#include "regionType.h"

#include "region.h"
#include "regionGeneratorInfo.h"

#include "..\..\core\types\names.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#include "..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

template <typename Class>
bool RegionType::Included<Class>::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _name)
{
	bool result = true;
	an_assert(_node->get_attribute(_name));
	if (included.load_from_xml(_node, _name, _lc))
	{
		if (_node->has_attribute(TXT("limit")))
		{
			useLimitCount = true;
			result &= limitCount.load_from_xml(_node, TXT("limit"));
		}
		probabilityCoef.load_from_xml(_node, TXT("probCoef"));
		probabilityCoef.load_from_xml(_node, TXT("probabilityCoef"));
		mulProbabilityCoef.load_from_xml(_node, TXT("mulProbCoef"));
		mulProbabilityCoef.load_from_xml(_node, TXT("mulProbabilityCoef"));
		canBeCreatedOnlyViaConnectorWithCreateNewPieceMark = _node->get_name_attribute(TXT("canBeCreatedOnlyViaConnectorWithCreateNewPieceMark"), canBeCreatedOnlyViaConnectorWithCreateNewPieceMark);
	}
	else
	{
		error_loading_xml(_node, TXT("problem loading \"%S\" for region type"), _name);
		result = false;
	}

	if (!result)
	{
		error_loading_xml(_node, TXT("can't load include %S"), _name);
	}

	return result;
}

template <typename Class>
bool RegionType::Included<Class>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= included.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

bool RegionType::IncludedWorldSettingCondition::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	probabilityCoef.load_from_xml(_node, TXT("probCoef"));
	probabilityCoef.load_from_xml(_node, TXT("probabilityCoef"));
	mulProbabilityCoef.load_from_xml(_node, TXT("mulProbCoef"));
	mulProbabilityCoef.load_from_xml(_node, TXT("mulProbabilityCoef"));
	canBeCreatedOnlyViaConnectorWithCreateNewPieceMark = _node->get_name_attribute(TXT("canBeCreatedOnlyViaConnectorWithCreateNewPieceMark"), canBeCreatedOnlyViaConnectorWithCreateNewPieceMark);
	WorldSettingCondition* includedSettingCondition = new WorldSettingCondition();
	if (includedSettingCondition->load_from_xml(_node))
	{
		if (!includedSettingCondition->is_empty())
		{
			worldSettingCondition = includedSettingCondition;
		}
		else
		{
			delete includedSettingCondition;
		}
	}
	else
	{
		// TODO error
		result = false;
		delete includedSettingCondition;
	}

	if (!result)
	{
		error_loading_xml(_node, TXT("can't load include world setting condition"));
	}

	return result;
}

//

template <typename Class>
RegionType::Include<Class>::Include(Name const & _typeName)
: typeName( _typeName )
{
}

template <typename Class>
RegionType::Include<Class>::~Include()
{
}

template <typename Class>
bool RegionType::Include<Class>::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _nameIncludePlainTypeNode, tchar const * _nameIncludePlainTypeAttr, tchar const * _nameIncludeSettingConditionNode)
{
	bool result = true;

	for_every(includedNode, _node->children_named(_nameIncludePlainTypeNode))
	{
		if (IO::XML::Attribute const * attr = includedNode->get_attribute(_nameIncludePlainTypeAttr))
		{
			RegionType::Included<Class> single;
			if (single.load_from_xml(includedNode, _lc, _nameIncludePlainTypeAttr))
			{
				included.push_back(single);
			}
			else
			{
				// TODO error
				result = false;
			}
		}
	}
	
	for_every(includeTaggedNode, _node->children_named(_nameIncludeSettingConditionNode))
	{
		includedSettingConditions.grow_size(1);
		if (includedSettingConditions.get_last().load_from_xml(includeTaggedNode, _lc))
		{
			// ok
		}
		else
		{
			includedSettingConditions.pop_back();
			result = false;
		}
	}

	return result;
}

template <typename Class>
bool RegionType::Include<Class>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(include, included)
	{
		result &= include->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

//

bool RegionType::Set::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc, RegionGenerator::LoadingContext * _regionLoadingContext)
{
	bool result = true;

	chance = _node->get_float_attribute(TXT("chance"), chance);
	count.load_from_xml(_node, TXT("count"));

	generationTagsCondition.load_from_xml_attribute(_node, TXT("generationTagsCondition"));
	
	setVariables.load_from_xml_child_node(_node, TXT("setVariables"));

	for_every(includeInstanceNode, _node->children_named(TXT("includeInstance")))
	{
		if (includeInstanceNode->has_attribute(TXT("regionType")))
		{
			UsedLibraryStored<RegionType> includeInstanceRegionType;
			if (includeInstanceRegionType.load_from_xml(includeInstanceNode, TXT("regionType"), _lc))
			{
				IncludeInstance<RegionType> include;
				include.include = includeInstanceRegionType;
				include.chance = includeInstanceNode->get_float_attribute(TXT("chance"), include.chance);
				include.generationTagsCondition.load_from_xml_attribute_or_child_node(includeInstanceNode, TXT("generationTagsCondition"));
				include.mayFail = includeInstanceNode->get_bool_attribute_or_from_child_presence(TXT("mayFail"), false);
				includeInstanceRegionTypes.push_back(include);
			}
			else
			{
				error_loading_xml(includeInstanceNode, TXT("invalid region type @ %S"), includeInstanceNode->get_location_info().to_char());
				result = false;
			}
		}
		if (includeInstanceNode->has_attribute(TXT("roomType")))
		{
			UsedLibraryStored<RoomType> includeInstanceRoomType;
			if (includeInstanceRoomType.load_from_xml(includeInstanceNode, TXT("roomType"), _lc))
			{
				IncludeInstance<RoomType> include;
				include.include = includeInstanceRoomType;
				include.chance = includeInstanceNode->get_float_attribute(TXT("chance"), include.chance);
				include.generationTagsCondition.load_from_xml_attribute_or_child_node(includeInstanceNode, TXT("generationTagsCondition"));
				include.mayFail = includeInstanceNode->get_bool_attribute_or_from_child_presence(TXT("mayFail"), false);
				includeInstanceRoomTypes.push_back(include);
			}
			else
			{
				error_loading_xml(includeInstanceNode, TXT("invalid room type @ %S"), includeInstanceNode->get_location_info().to_char());
				result = false;
			}
		}
	}

	for_every(includeInstanceNode, _node->children_named(TXT("regionPieceInstance")))
	{
		RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> piece( new PieceCombiner::Piece<RegionGenerator>() );
		piece->def.desc = TXT("regionPieceInstance of set");
		bool loaded = piece->load_from_xml(includeInstanceNode, _regionLoadingContext);
		result &= loaded;
		if (loaded)
		{
			insideRegionPieceInstances.push_back(piece);
		}
	}

	return result;
}

bool RegionType::Set::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext, RegionGenerator::PreparingContext & _preparingContext)
{
	bool result = true;

	for_every(includeInstanceRegionType, includeInstanceRegionTypes)
	{
		if (includeInstanceRegionType->mayFail)
		{
			includeInstanceRegionType->include.prepare_for_game_find_may_fail(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
		else
		{
			result &= includeInstanceRegionType->include.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
	}

	for_every(includeInstanceRoomType, includeInstanceRoomTypes)
	{
		if (includeInstanceRoomType->mayFail)
		{
			includeInstanceRoomType->include.prepare_for_game_find_may_fail(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
		else
		{
			result &= includeInstanceRoomType->include.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		}
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		for_every_ref(insideRegionPieceInstance, insideRegionPieceInstances)
		{
			if (!insideRegionPieceInstance->prepare(&_preparingContext))
			{
				error(TXT("couldn't prepare inside region piece instance"));
				result = false;
			}
		}

	}

	return result;
}

//

bool RegionType::Environments::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	defaultEnvironmentName = _node->get_name_attribute(TXT("default"), defaultEnvironmentName);
	defaultEnvironmentName = _node->get_name_attribute(TXT("defaultEnvironmentName"), defaultEnvironmentName);
	for_every(createEnvironmentNode, _node->children_named(TXT("create")))
	{
		Environments::Create createEnvironment;
		createEnvironment.name = createEnvironmentNode->get_name_attribute(TXT("name"));
		createEnvironment.parentName = createEnvironmentNode->get_name_attribute(TXT("parent"));
		createEnvironment.environmentType.load_from_xml(createEnvironmentNode, TXT("type"), _lc);
		if (createEnvironment.name.is_valid() &&
			createEnvironment.environmentType.is_name_valid())
		{
			bool found = false;
			for_every(c, create)
			{
				if (c->name == createEnvironment.name)
				{
					*c = createEnvironment;
					found = true;
					break;
				}
			}
			if (!found)
			{
				create.push_back(createEnvironment);
			}
		}
		else
		{
			error_loading_xml(createEnvironmentNode, TXT("create environment is not properly defined"));
			result = false;
		}
	}

	return result;
}

bool RegionType::Environments::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(c, create)
	{
		result &= c->environmentType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

//
		
REGISTER_FOR_FAST_CAST(RegionType);
LIBRARY_STORED_DEFINE_TYPE(RegionType, regionType);

RegionType::RegionType(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, includedRoomTypes(Names::roomType)
, includedRegionTypes(Names::regionType)
, regionPiece( new PieceCombiner::Piece<RegionGenerator>() )
{
	regionPiece->def.regionType.lock(this);
}

void RegionType::prepare_to_unload()
{
	base::prepare_to_unload();
	regionPiece->def.regionType.unlock_and_clear();
}

RegionType::~RegionType()
{
}

bool RegionType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("requiresVariables")))
	{
		result &= requiresVariables.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("setting")))
	{
		if (!setting.is_set())
		{
			setting = new WorldSetting();
		}
		result &= setting->load_from_xml(node, _lc);
	}

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

	result &= copySettings.load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("wheresMyPointSetupParameters")))
	{
		result &= wheresMyPointProcessorSetupParameters.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("wheresMyPointOnSetupGenerator")))
	{
		result &= wheresMyPointProcessorOnSetupGenerator.load_from_xml(node);
	}
	
	for_every(node, _node->children_named(TXT("wheresMyPointOnGenerate")))
	{
		result &= wheresMyPointProcessorOnGenerate.load_from_xml(node);
	}

	result &= libraryStoredReplacer.load_from_xml_child_node(_node, _lc);

	{	// region generator
		RegionGenerator::LoadingContext loadingContext(_lc);

		for_every(node, _node->children_named(TXT("asRegionPiece")))
		{
			result &= regionPiece->load_from_xml(node, &loadingContext);
		}
	}

	for_every(regionGeneratorNode, _node->children_named(TXT("asRegionGenerator")))
	{	// region generator
		{
			String rgiType = regionGeneratorNode->get_string_attribute(TXT("type"));
			if (!rgiType.is_empty() || !regionGeneratorInfo.get())
			{
				if (regionGeneratorInfo.get() && !rgiType.is_empty())
				{
					warn_loading_xml(regionGeneratorNode, TXT("replacing already existing region generation info, provide without type to add data"));
				}
				regionGeneratorInfo = RegionGeneratorInfo::create(rgiType);
			}
		}
		if (regionGeneratorInfo.is_set())
		{
			result &= regionGeneratorInfo->load_from_xml(regionGeneratorNode, _lc);
		}

		for_every(generationRulesNode, regionGeneratorNode->children_named(TXT("generationRules")))
		{
			result &= generationRules.load_from_xml(generationRulesNode);
		}
		result &= includedRoomTypes.load_from_xml(regionGeneratorNode, _lc, TXT("include"), TXT("roomType"), TXT("includeRoomTypes"));
		result &= includedRegionTypes.load_from_xml(regionGeneratorNode, _lc, TXT("include"), TXT("regionType"), TXT("includeRegionTypes"));

		RegionGenerator::LoadingContext loadingContext(_lc);

		result &= load_region_generation_from_xml(regionGeneratorNode, _lc, &loadingContext);

		for_every(pieceNode, regionGeneratorNode->children_named(TXT("regionPiece")))
		{
			RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> piece(new PieceCombiner::Piece<RegionGenerator>());
			bool loaded = piece->load_from_xml(pieceNode, &loadingContext);
			result &= loaded;
			if (loaded)
			{
				insideRegionPieces.push_back(piece);
			}
		}

		for_every(insideOuterConnectorNode, regionGeneratorNode->children_named(TXT("outerRegionConnector")))
		{
			RefCountObjectPtr<PieceCombiner::Connector<RegionGenerator>> insideOuterConnector( new PieceCombiner::Connector<RegionGenerator>() );
			bool loaded = insideOuterConnector->load_from_xml(insideOuterConnectorNode, &loadingContext);
			if (loaded && !insideOuterConnector->outerName.is_valid())
			{
				error_loading_xml(insideOuterConnectorNode, TXT("no outer defined for outer connector"));
				loaded = false;
			}
			result &= loaded;
			if (loaded)
			{
				insideOuterRegionConnectors.push_back(insideOuterConnector);
			}
		}

		for_every(slotConnectorNode, regionGeneratorNode->children_named(TXT("slotConnector")))
		{
			RefCountObjectPtr<PieceCombiner::Connector<RegionGenerator>> slotConnector( new PieceCombiner::Connector<RegionGenerator>() );
			bool loaded = slotConnector->load_from_xml(slotConnectorNode, &loadingContext);
			slotConnector->type = PieceCombiner::ConnectorType::Normal;
			if (loaded && !slotConnector->name.is_valid())
			{
				error_loading_xml(slotConnectorNode, TXT("no name defined for slot connector"));
				loaded = false;
			}
			result &= loaded;
			if (loaded)
			{
				insideSlotConnectors.push_back(slotConnector);
			}
		}
	}
	
	for_every(node, _node->children_named(TXT("environments")))
	{
		result &= environments.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("useReverb")))
	{
		warn_loading_xml(node, TXT("deprecated, use sound reverb pair"));
		result &= useReverb.load_from_xml(_node->first_child_named(TXT("useReverb")), TXT("type"), _lc);
	}
	for_every(node, _node->children_named(TXT("sound")))
	{
		result &= useReverb.load_from_xml(node, TXT("reverb"), _lc);
	}

	result &= doorVRCorridor.load_from_xml(_node, _lc);

	return result;
}

bool RegionType::load_region_generation_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc, RegionGenerator::LoadingContext * _regionLoadingContext)
{
	bool result = true;

	for_every(setNode, _node->children_named(TXT("obligatorySet")))
	{
		obligatorySets.push_back(Set());
		if (!obligatorySets.get_last().load_from_xml(setNode, _lc, _regionLoadingContext))
		{
			obligatorySets.pop_back();
			result = false;
		}
	}

	for_every(setNode, _node->children_named(TXT("set")))
	{
		sets.push_back(Set());
		if (!sets.get_last().load_from_xml(setNode, _lc, _regionLoadingContext))
		{
			sets.pop_back();
			result = false;
		}
	}

	return result;
}

bool RegionType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
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

	result &= libraryStoredReplacer.prepare_for_game(_library, _pfgContext);

	if (regionGeneratorInfo.is_set())
	{
		result &= regionGeneratorInfo->prepare_for_game(_library, _pfgContext);
	}

	result &= environments.prepare_for_game(_library, _pfgContext);
	result &= useReverb.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	result &= includedRoomTypes.prepare_for_game(_library, _pfgContext);
	result &= includedRegionTypes.prepare_for_game(_library, _pfgContext);

	{	// region generator
		RegionGenerator::PreparingContext preparingContext(_library);
		for_every(set, obligatorySets)
		{
			result &= set->prepare_for_game(_library, _pfgContext, preparingContext);
		}
		for_every(set, sets)
		{
			result &= set->prepare_for_game(_library, _pfgContext, preparingContext);
		}
	}

	result &= doorVRCorridor.prepare_for_game(_library, _pfgContext);

	// everything below is just resolving
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		{	// region generator
			RegionGenerator::PreparingContext preparingContext(_library);

			result &= regionPiece->prepare(&preparingContext);
		}

		{	// region generator
			RegionGenerator::PreparingContext preparingContext(_library);

			for_every_ref(piece, insideRegionPieces)
			{
				result &= piece->prepare(&preparingContext);
			}

			for_every_ref(insideOuterConnector, insideOuterRegionConnectors)
			{
				result &= insideOuterConnector->prepare(nullptr, &preparingContext);
			}

			for_every_ref(insideSlotConnector, insideSlotConnectors)
			{
				result &= insideSlotConnector->prepare(nullptr, &preparingContext);
			}
		}
	}

	return result;
}

RegionType::Set const * RegionType::choose_set(Random::Generator & _generator, Tags const & _generationTags) const
{
	if (sets.is_empty())
	{
		return nullptr;
	}
	int idx = RandomUtils::ChooseFromContainer<List<Set>, Set>::choose(
		_generator, sets,
		[](Set const & _set)
		{
			return _set.chance;
		},
		[&_generationTags](Set const& _set)
		{
			return _set.generationTagsCondition.check(_generationTags);
		}, true);

	if (idx != NONE && idx < sets.get_size())
	{
		return &sets[idx];
	}
	else
	{
		return nullptr;
	}
}

PieceCombiner::Connector<RegionGenerator> const * RegionType::find_inside_outer_region_connector(Name const & _outerName) const
{
	for_every_ref(insideOuterRegionConnector, insideOuterRegionConnectors)
	{
		if (insideOuterRegionConnector->outerName == _outerName)
		{
			return insideOuterRegionConnector;
		}
	}

	return nullptr;
}

PieceCombiner::Connector<RegionGenerator> const * RegionType::find_inside_slot_connector(Name const & _slotName) const
{
	for_every_ref(insideSlotConnector, insideSlotConnectors)
	{
		if (insideSlotConnector->name == _slotName)
		{
			return insideSlotConnector;
		}
	}

	return nullptr;
}

void RegionType::add_set(RegionType::Set const * regionTypeSet, REF_ PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Region * _region, REF_ Random::Generator & _randomGenerator)
{
	struct IncludeOne
	{
		enum Type
		{
			RegionType,
			RoomType,
		};
		Type type;
		int idx;
		float chance;
		IncludeOne() {}
		IncludeOne(Type _type, int _idx, float _chance) : type(_type), idx(_idx), chance(_chance) {}
	};

	_region->access_variables().set_from(regionTypeSet->setVariables);

	if (! regionTypeSet->count.is_empty() &&
		regionTypeSet->count.min > 0)
	{
		ARRAY_STACK(IncludeOne, include, regionTypeSet->includeInstanceRegionTypes.get_size() + regionTypeSet->includeInstanceRoomTypes.get_size());
		for_every(includeInstanceRegionType, regionTypeSet->includeInstanceRegionTypes)
		{
			if (includeInstanceRegionType->generationTagsCondition.check(_generator.get_generation_context().generationTags))
			{
				include.push_back(IncludeOne(IncludeOne::RegionType, for_everys_index(includeInstanceRegionType), includeInstanceRegionType->chance));
			}
		}
		for_every(includeInstanceRoomType, regionTypeSet->includeInstanceRoomTypes)
		{
			if (includeInstanceRoomType->generationTagsCondition.check(_generator.get_generation_context().generationTags))
			{
				include.push_back(IncludeOne(IncludeOne::RoomType, for_everys_index(includeInstanceRoomType), includeInstanceRoomType->chance));
			}
		}
		int countLeft = _randomGenerator.get(regionTypeSet->count);
		while (countLeft && !include.is_empty())
		{
			int idx = RandomUtils::ChooseFromContainer<ArrayStack<IncludeOne>, IncludeOne>::choose(
				_randomGenerator, include, [](IncludeOne const & _ci) { return _ci.chance; });
			IncludeOne & inc = include[idx];
			if (inc.type == IncludeOne::RegionType)
			{
				auto* includeInstanceRegionType = &regionTypeSet->includeInstanceRegionTypes[inc.idx];
				_generator.add_piece(includeInstanceRegionType->include.get()->get_as_region_piece(), Framework::RegionGenerator::PieceInstanceData());
			}
			else
			{
				auto* includeInstanceRoomType = &regionTypeSet->includeInstanceRoomTypes[inc.idx];
				an_assert(inc.type == IncludeOne::RoomType);
				_generator.add_piece(includeInstanceRoomType->include.get()->get_region_piece_for_region_generator(_generator, _region, REF_ _randomGenerator).get(), Framework::RegionGenerator::PieceInstanceData());
			}
			include.remove_fast_at(idx);
			--countLeft;
		}
	}
	else
	{
		// all
		for_every(includeInstanceRegionType, regionTypeSet->includeInstanceRegionTypes)
		{
			_generator.add_piece(includeInstanceRegionType->include.get()->get_as_region_piece(), Framework::RegionGenerator::PieceInstanceData());
		}
		for_every(includeInstanceRoomType, regionTypeSet->includeInstanceRoomTypes)
		{
			_generator.add_piece(includeInstanceRoomType->include.get()->get_region_piece_for_region_generator(_generator, _region, REF_ _randomGenerator).get(), Framework::RegionGenerator::PieceInstanceData());
		}
	}
	for_every_ref(insideRegionPieceInstance, regionTypeSet->insideRegionPieceInstances)
	{
		_generator.add_piece(insideRegionPieceInstance, Framework::RegionGenerator::PieceInstanceData());
	}
}

void RegionType::setup_region_generator_add_pieces(REF_ PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Region * _region, REF_ Random::Generator & _randomGenerator) const
{
	for_every(set, obligatorySets)
	{
		if (set->generationTagsCondition.check(_generator.get_generation_context().generationTags))
		{
			add_set(set, _generator, _region, _randomGenerator);
		}
	}
	if (RegionType::Set const * regionTypeSet = choose_set(_randomGenerator, _generator.get_generation_context().generationTags))
	{
		add_set(regionTypeSet, _generator, _region, _randomGenerator);
	}
}
