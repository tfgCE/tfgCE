#include "pilgrimage.h"

#include "pilgrimageInstanceLinear.h"
#include "pilgrimageInstanceOpenWorld.h"
#include "pilgrimageOpenWorldDefinition.h"
#include "pilgrimagePart.h"
#include "pilgrimageEnvironment.h"

#include "..\library\library.h"

#include "..\loaders\loaderStarField.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(Pilgrimage);
LIBRARY_STORED_DEFINE_TYPE(Pilgrimage, pilgrimage);

Pilgrimage::Pilgrimage(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

Pilgrimage::~Pilgrimage()
{
	output(TXT("Pilgrimage::~Pilgrimage"));
}

bool Pilgrimage::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	isTemplate = _node->get_bool_attribute_or_from_child_presence(TXT("template"), isTemplate);

	result &= defaultNextPilgrimage.load_from_xml(_node, TXT("defaultNextPilgrimage"), _lc);

	for_every(node, _node->children_named(TXT("pilgrimageName")))
	{
		result &= pilgrimageNamePreShort.load_from_xml_child(node, TXT("preShort"), _lc, TXT("pilNamePreShort"));
		result &= pilgrimageNamePreLong.load_from_xml_child(node, TXT("preLong"), _lc, TXT("pilNamePreLong"));
		result &= pilgrimageNamePreConnector.load_from_xml_child(node, TXT("preConnector"), _lc, TXT("pilNamePreCon"));
		result &= pilgrimageName.load_from_xml_child(node, TXT("main"), _lc, TXT("pilgrimageName"));
	}

	unlocks.autoUnlockOnProfileGeneralProgress.load_from_xml_attribute_or_child_node(_node, TXT("autoUnlockOnProfileGeneralProgress"));
	unlocks.autoUnlockWhenReached = _node->get_bool_attribute_or_from_child_presence(TXT("autoUnlockWhenReached"), unlocks.autoUnlockWhenReached);
	unlocks.canBeUnlockedForExperience = _node->get_bool_attribute_or_from_child_presence(TXT("canBeUnlockedForExperience"), unlocks.canBeUnlockedForExperience);
	unlocks.appearAsUnlockedForExperience = _node->get_bool_attribute_or_from_child_presence(TXT("appearAsUnlockedForExperience"), unlocks.appearAsUnlockedForExperience);
	unlocks.experienceForReachingForFirstTime.load_from_attribute_or_from_child(_node, TXT("experienceForReachingForFirstTime"));
	unlocks.allowedEXMLevels.load_from_xml_child_node(_node, TXT("allowedEXM"), TXT("levels"));
	
	{	// environments
		if (auto* attr = _node->get_attribute(TXT("copyEnvironmentFrom")))
		{
			bool copied = false;
			if (auto* lib = up_cast<Library>(_lc.get_library()))
			{
				Framework::LibraryName toFind;
				if (toFind.load_from_xml(attr, _lc))
				{
					if (auto* found = lib->get_pilgrimages().find(toFind))
					{
						// copy before loading!
						environmentCycleSpeed = found->environmentCycleSpeed;
						environmentCycles = found->environmentCycles;

						environments = found->environments;

						copied = true;
					}
				}
			}
			error_loading_xml_on_assert(copied, result, _node, TXT("couldn't find pilgrimage to copy from"));
		}

		for_every(environmentsNode, _node->children_named(TXT("environments")))
		{
			for_every(node, environmentsNode->children_named(TXT("create")))
			{
				warn_loading_xml(node, TXT("rename \"create\" to \"environment\" in pilgrimage definition"));
			}

			for_every(node, environmentsNode->children_named(TXT("environment")))
			{
				PilgrimageEnvironment pe;
				if (pe.load_from_xml(node, _lc))
				{
					bool addNew = true;
					for_every(e, environments)
					{
						if (e->name == pe.name)
						{
							*e = pe;
							addNew = false;
							break;
						}
					}
					if (addNew)
					{
						environments.push_back(pe);
					}
				}
				else
				{
					error_loading_xml(node, TXT("could not load environment info"));
				}
			}
		}

		environmentCycleSpeed = _node->get_float_attribute_or_from_child(TXT("environmentCycleSpeed"), environmentCycleSpeed);
		for_every(node, _node->children_named(TXT("lockEnvironmentCycleUntilSeen")))
		{
			lockEnvironmentCycleUntilSeen.load_from_xml(node, TXT("at"));
			lockEnvironmentCycleUntilSeenRoomTagged.load_from_xml_attribute_or_child_node(node, TXT("roomTagged"));
		}

		for_every(node, _node->children_named(TXT("forcedNoon")))
		{
			forcedNoonYaw.load_from_xml(node, TXT("yaw"));
			forcedNoonPitch.load_from_xml(node, TXT("pitch"));
		}

		for_every(node, _node->children_named(TXT("environmentCycle")))
		{
			RefCountObjectPtr<PilgrimageEnvironmentCycle> pec;
			pec = new PilgrimageEnvironmentCycle();

			if (pec->load_from_xml(node, _lc))
			{
				environmentCycles.push_back(pec);
				{
					bool found = false;
					for_every(e, environments)
					{
						if (e->name == pec->get_name())
						{
							found = true;
							break;
						}
					}
					error_loading_xml_on_assert(found, result, node, TXT("if you want to use an environmentCycle, add environment of corresponding name as well"));
				}
			}
			else
			{
				error_loading_xml(node, TXT("could not load environment cycle"));
			}
		}
	}

	{	// pilgrim
		result &= pilgrim.load_from_xml(_node, TXT("pilgrim"), _lc);

		error_loading_xml_on_assert(isTemplate || pilgrim.is_name_valid(), result, _node, TXT("no pilgrim defined"));
	}
	startHereWithEXMsFromLastSetup = _node->get_bool_attribute_or_from_child_presence(TXT("startHereWithEXMsFromLastSetup"), startHereWithEXMsFromLastSetup);
	startHereWithOnlyUnlockedEXMs = _node->get_bool_attribute_or_from_child_presence(TXT("startHereWithOnlyUnlockedEXMs"), startHereWithOnlyUnlockedEXMs);
	startHereWithoutEXMs = _node->get_bool_attribute_or_from_child_presence(TXT("startHereWithoutEXMs"), startHereWithoutEXMs);
	startHereWithoutWeapons = _node->get_bool_attribute_or_from_child_presence(TXT("startHereWithoutWeapons"), startHereWithoutWeapons);
	startHereWithWeaponsFromArmoury = _node->get_bool_attribute_or_from_child_presence(TXT("startHereWithWeaponsFromArmoury"), startHereWithWeaponsFromArmoury);
	startHereWithDamagedWeapons = _node->get_bool_attribute_or_from_child_presence(TXT("startHereWithDamagedWeapons"), startHereWithDamagedWeapons);

	for_every(linearNode, _node->children_named(TXT("linear")))
	{
		linear.active = true;
		linear.startingPart = linearNode->get_name_attribute_or_from_child(TXT("startingPart"), linear.startingPart);
		linear.looped = linearNode->get_bool_attribute_or_from_child_presence(TXT("looped"), linear.looped);
		linear.random = linearNode->get_bool_attribute_or_from_child_presence(TXT("random"), linear.random);

		for_every(node, linearNode->children_named(TXT("part")))
		{
			RefCountObjectPtr<PilgrimagePart> part;
			part = new PilgrimagePart();
			if (part->load_from_xml(node, _lc))
			{
				linear.parts.push_back(part);
			}
			else
			{
				error_loading_xml(node, TXT("couldn't load part"));
				result = false;
			}
		}

		for_every_ref(part, linear.parts)
		{
			if (!linear.looped && for_everys_index(part) == linear.parts.get_size() - 1)
			{
				error_loading_xml_on_assert(!part->innerRegionType.is_name_valid(), result, linearNode, TXT("for non looped, last part should not have innerRegionType"));
			}
			else
			{
				error_loading_xml_on_assert(part->innerRegionType.is_name_valid(), result, linearNode, TXT("no innerRegionType for one of the parts"));
			}
		}
	}

	for_every(openWorldNode, _node->children_named(TXT("openWorld")))
	{
		openWorld.active = true;

		openWorld.definition = new PilgrimageOpenWorldDefinition();
		if (! openWorld.definition->load_from_xml(openWorldNode, _lc))
		{
			error_loading_xml(openWorldNode, TXT("couldn't load open world definition"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("music")))
	{
		result &= musicDefinition.load_from_xml(node, _lc);
	}

	gameDirectorDefinition.useGameDirector = _node->get_bool_attribute_or_from_child_presence(TXT("useGameDirector"), gameDirectorDefinition.useGameDirector);
	for_every(node, _node->children_named(TXT("gameDirector")))
	{
		gameDirectorDefinition.useGameDirector = true;
		result &= gameDirectorDefinition.load_from_xml(node, _lc);
	}

	canCreateNextPilgrimage = _node->get_bool_attribute_or_from_child_presence(TXT("canCreateNextPilgrimage"), canCreateNextPilgrimage);
	canCreateNextPilgrimage = ! _node->get_bool_attribute_or_from_child_presence(TXT("cantCreateNextPilgrimage"), !canCreateNextPilgrimage);

	allowAutoStoreOfPersistence = _node->get_bool_attribute_or_from_child_presence(TXT("allowAutoStoreOfPersistence"), allowAutoStoreOfPersistence);
	allowAutoStoreOfPersistence = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowAutoStoreOfPersistence"), ! allowAutoStoreOfPersistence);
	allowStoreIntermediateGameState = _node->get_bool_attribute_or_from_child_presence(TXT("allowStoreIntermediateGameState"), allowStoreIntermediateGameState);
	allowStoreIntermediateGameState = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowStoreIntermediateGameState"), ! allowStoreIntermediateGameState);
	allowStoreChapterStartGameState = _node->get_bool_attribute_or_from_child_presence(TXT("allowStoreChapterStartGameState"), allowStoreChapterStartGameState);
	allowStoreChapterStartGameState = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowStoreChapterStartGameState"), !allowStoreChapterStartGameState);

	for_every(urNode, _node->children_named(TXT("utilityRoom")))
	{
		PilgrimageUtilityRoomDefinition purd;
		if (purd.load_from_xml(urNode, _lc))
		{
			utilityRooms.push_back(purd);
		}
		else
		{
			error_loading_xml(urNode, TXT("problem loading utilityRoom"));
			result = false;
		}
	}

	for_every(urNode, _node->children_named(TXT("lockerRoomSpawn")))
	{
		PilgrimageLockerRoomSpawn plrs;
		if (plrs.load_from_xml(urNode, _lc))
		{
			lockerRoomSpawns.push_back(plrs);
		}
		else
		{
			error_loading_xml(urNode, TXT("problem loading lockerRoomSpawn"));
			result = false;
		}
	}

	for_every(clNode, _node->children_named(TXT("customLoader")))
	{
		customLoader = clNode->get_name_attribute_or_from_child(TXT("type"), customLoader);
		customLoaderStartingPointRadius.load_from_xml(clNode, TXT("startingPointRadius"));
		customLoaderParams.load_from_xml_child_node(clNode, TXT("params"));
		for_every(wmpNode, clNode->children_named(TXT("initialBeAtRightPlaceWheresMyPoint")))
		{
			initialBeAtRightPlaceForCustomLoader.load_from_xml(wmpNode);
		}
	}

	//

	int activeCount = 0;
	activeCount += linear.active ? 1 : 0;
	activeCount += openWorld.active ? 1 : 0;
	if (activeCount > 1)
	{
		error_loading_xml(_node, TXT("more than one type of pilgrimage defined"));
		result = false;
	}
	else if (activeCount == 0 && ! isTemplate)
	{
		error_loading_xml(_node, TXT("no type of pilgrimage defined"));
		result = false;
	}

	//

	for_every(pNode, _node->children_named(TXT("preload")))
	{
		for_every(node, pNode->all_children())
		{
			Framework::UsedLibraryStoredAny p;
			if (p.load_from_xml(node, nullptr, TXT("name"), _lc))
			{
				preloads.push_back(p);
			}
		}
	}

	//

	for_every(pNode, _node->children_named(TXT("equipmentParameters")))
	{
		result &= equipmentParameters.load_from_xml(pNode);
		_lc.load_group_into(equipmentParameters);
	}

	return result;
}

bool Pilgrimage::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (isTemplate)
	{
		return result;
	}

	result &= defaultNextPilgrimage.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= pilgrim.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	
	for_every_ref(e, environmentCycles)
	{
		result &= e->prepare_for_game(_library, _pfgContext);
	}
	
	for_every(pe, environments)
	{
		result &= pe->prepare_for_game(_library, _pfgContext);
	}

	for_every_ref(part, linear.parts)
	{
		result &= part->prepare_for_game(_library, _pfgContext);
	}

	if (openWorld.definition.get())
	{
		result &= openWorld.definition->prepare_for_game(_library, _pfgContext);
	}

	result &= musicDefinition.prepare_for_game(_library, _pfgContext);
	result &= gameDirectorDefinition.prepare_for_game(_library, _pfgContext);

	for_every(ur, utilityRooms)
	{
		result &= ur->prepare_for_game(_library, _pfgContext);
	}

	for_every(lrs, lockerRoomSpawns)
	{
		result &= lrs->prepare_for_game(_library, _pfgContext);
	}

	for_every(p, preloads)
	{
		result &= p->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

PilgrimagePart* Pilgrimage::linear__get_starting_part(Random::Generator const & _rg) const
{
	an_assert(!linear.parts.is_empty());
	if (linear.startingPart.is_valid())
	{
		for_every_ref(part, linear.parts)
		{
			if (part->get_name() == linear.startingPart)
			{
				return part;
			}
		}
	}
	return linear.parts.get_first().get();
}

PilgrimagePart* Pilgrimage::linear__get_next_part(PilgrimagePart* _part, Random::Generator const& _rg, int _partIdx) const
{
	an_assert(!linear.parts.is_empty());
	if (linear.random)
	{
		/**
		 *	This is a temporary solution.
		 *	For the final game, I will need something more sophisticated than just putting random parts after each other.
		 */
		int partWithinLoop = mod(_partIdx, linear.parts.get_size());
		int loopNo = (_partIdx - partWithinLoop) / linear.parts.get_size();
		
		if (!linear.looped && loopNo != 0)
		{
			// not further parts
			return nullptr;
		}

		// advance to the proper loopNo
		Random::Generator rg = _rg;
		for_count(int, i, loopNo + 1)
		{
			rg.advance_seed(43, 802 + i);
		}

		int startingPartIdx = NONE;
		// for starting loop, if there is a starting part, always start with it
		if (loopNo == 0 &&
			linear.startingPart.is_valid())
		{
			for_every_ref(part, linear.parts)
			{
				if (part->get_name() == linear.startingPart)
				{
					startingPartIdx = for_everys_index(part);
					if (partWithinLoop == 0)
					{
						return linear.parts[startingPartIdx].get();
					}
					break;
				}
			}
		}

		// fill array with part indices, it will be just an array of consecutive numbers (0,1,2,3,4,5,6...)
		// for non looped, last one is ending one, without region type
		ARRAY_STACK(int, partsInLoop, linear.parts.get_size());
		for_count(int, i, linear.parts.get_size() + (!linear.looped? -1 : 0))
		{
			if (i != startingPartIdx)
			{
				partsInLoop.push_back(i);
			}
			else
			{
				--partWithinLoop; // we don't add it, so we have less to go through
			}
		}

		// remove parts to get the one we want, first remove former ones
		for_count(int, i, partWithinLoop)
		{
			int iAt = rg.get_int(partsInLoop.get_size());
			partsInLoop.remove_at(iAt);
		}

		// get the part we want
		int actuallyChosen = rg.get_int(partsInLoop.get_size());
		return linear.parts[actuallyChosen].get();
	}
	else
	{
		if (_part && _part->get_next_part_name().is_valid())
		{
			for_every_ref(part, linear.parts)
			{
				if (part->get_name() == _part->get_next_part_name())
				{
					return part;
				}
			}
		}
		if (_part)
		{
			// it should be one of the parts
			for_count(int, i, linear.parts.get_size() - 1)
			{
				if (linear.parts[i].get() == _part)
				{
					return linear.parts[i + 1].get();
				}
			}
		}
		if (!linear.looped && _part)
		{
			// if not looped but we wanted to find next part, it means we reached the end - this doubles a bit condiition above
			return nullptr;
		}
		return linear.parts.get_first().get();
	}
}

PilgrimageInstance* Pilgrimage::create_instance(Game* _game)
{
	if (is_linear())
	{
		return new PilgrimageInstanceLinear(_game, this);
	}
	else if (is_open_world())
	{
		return new PilgrimageInstanceOpenWorld(_game, this);
	}
	else
	{
		todo_implement;
		return nullptr;
	}
}

Loader::ILoader* Pilgrimage::create_custom_loader() const
{
	if (customLoader == TXT("star field"))
	{
		auto* l = new Loader::StarField(provide_initial_be_at_right_place_for_custom_loader());

		l->learn_from(customLoaderParams);

		return l;
	}
	error(TXT("loader not provided or unknown \"%S\""), customLoader.to_char());
	return nullptr;
}

Optional<Transform> Pilgrimage::provide_initial_be_at_right_place_for_custom_loader() const
{
	Optional<Transform> result;

	if (!initialBeAtRightPlaceForCustomLoader.is_empty())
	{
		WheresMyPoint::Context context(nullptr);
		WheresMyPoint::Value value;
		initialBeAtRightPlaceForCustomLoader.update(value, context);
		if (value.get_type() == type_id<Transform>())
		{
			result = value.get_as<Transform>();
		}
		else
		{
			error(TXT("expecting Transform value from initialBeAtRightPlaceForCustomLoader"));
		}
	}
	return result;
}

void Pilgrimage::load_on_demand_all_required()
{
	output(TXT("[Pilgrimage] load on demand for \"%S\""), get_name().to_string().to_char());

	for_every(p, preloads)
	{
		if (auto* ls = p->get())
		{
			ls->load_on_demand_if_required();
		}
	}

	get_music_definition().load_on_demand_if_required();

	if (auto* sc = get_game_director_definition().storyChapter.get())
	{
		sc->load_data_on_demand_if_required();
	}
}

void Pilgrimage::unload_for_load_on_demand_all_required()
{
	output(TXT("[Pilgrimage] unload on demand for \"%S\""), get_name().to_string().to_char());

	//for time being only music can be unloaded
	/*
	for_every(p, preloads)
	{
		if (auto* ls = p->get())
		{
			ls->unload_for_load_on_demand();
		}
	}
	*/

	get_music_definition().unload_for_load_on_demand();

	/*
	if (auto* sc = get_game_director_definition().storyChapter.get())
	{
		sc->unload_data_for_load_on_demand();
	}
	*/
}

//

bool PilgrimageUtilityRoomDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	tag = _node->get_name_attribute(TXT("tag"));
	error_loading_xml_on_assert(tag.is_valid(), result, _node, TXT("no \"tag\" provided"));

	roomType.load_from_xml(_node, TXT("roomType"), _lc);
	error_loading_xml_on_assert(roomType.is_name_valid(), result, _node, TXT("no \"roomType\" provided"));

	return result;
}

bool PilgrimageUtilityRoomDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= roomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool PilgrimageLockerRoomSpawn::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	name = _node->get_string_attribute_or_from_child(TXT("name"), name);

	tags.load_from_xml_attribute_or_child_node(_node, TXT("tags")); 

	if (name.is_empty() && !tags.is_empty())
	{
		name = tags.to_string();
	}
	
	actorType.load_from_xml(_node, TXT("actorType"), _lc);
	itemType.load_from_xml(_node, TXT("itemType"), _lc);
	sceneryType.load_from_xml(_node, TXT("sceneryType"), _lc);
	error_loading_xml_on_assert(actorType.is_name_valid() || itemType.is_name_valid() || sceneryType.is_name_valid(), result, _node, TXT("no \"actorType\", \"itemType\" nor \"sceneryType\" provided"));
		
	parameters.load_from_xml_child_node(_node, TXT("parameters"));
	_lc.load_group_into(parameters);

	return result;
}

bool PilgrimageLockerRoomSpawn::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= actorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= itemType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= sceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
