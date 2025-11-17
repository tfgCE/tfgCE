#include "regionGenerator.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "door.h"
#include "doorInRoom.h"
#include "region.h"
#include "regionGeneratorInfo.h"
#include "regionType.h"
#include "roomType.h"
#include "room.h"
#include "world.h"
#include "..\game\game.h"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// door in room tags
DEFINE_STATIC_NAME(vrElevatorAltitude);

//

bool RegionGenerator::PieceDef::load_from_xml(IO::XML::Node const * _node, LoadingContext * _context)
{
	bool result = true;

	desc = _node->get_string_attribute_or_from_child(TXT("desc"), desc);
	roomType.load_from_xml(_node, TXT("roomType"), _context->libraryLoadingContext);
	regionType.load_from_xml(_node, TXT("regionType"), _context->libraryLoadingContext);

	return result;
}

bool RegionGenerator::PieceDef::prepare(PieceCombiner::Piece<RegionGenerator> * _piece, PreparingContext * _context)
{
	bool result = true;

	result &= roomType.find(_context->library);

	result &= regionType.find(_context->library);

	return result;
}

bool RegionGenerator::PieceDef::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _renamer.apply_to(roomType, _library);
	result &= _renamer.apply_to(regionType, _library);

	return result;
}

String RegionGenerator::PieceDef::get_desc() const
{
	String result;
	if (roomType.get())
	{
		result = String(TXT("[room type] ")) + roomType->get_name().to_string();
	}
	else if (roomType.get_name().is_valid())
	{
		result = String(TXT("[room type] ")) + roomType.get_name().to_string();
	}
	else if (regionType.get())
	{
		result = String(TXT("[region type] ")) + regionType->get_name().to_string();
	}
	else if (regionType.get_name().is_valid())
	{
		result = String(TXT("[region type] ")) + regionType.get_name().to_string();
	}
	if (! desc.is_empty() && ! result.is_empty())
	{
		return desc + TXT(" : ") + result;
	}
	else
	{
		return desc + result;
	}
}

//

bool RegionGenerator::ConnectorDef::load_from_xml(IO::XML::Node const * _node, LoadingContext * _context)
{
	bool result = true;

	result &= doorType.load_from_xml(_node, TXT("doorType"), _context->libraryLoadingContext);

	result &= doorInRoomTags.load_from_xml(_node, TXT("doorInRoomTags"));

	return result;
}

bool RegionGenerator::ConnectorDef::prepare(PieceCombiner::Piece<RegionGenerator> * _piece, PieceCombiner::Connector<RegionGenerator> * _connector, PreparingContext * _context)
{
	bool result = true;

	result &= doorType.find(_context->library);

	return result;
}

bool RegionGenerator::ConnectorDef::apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _renamer.apply_to(doorType, _library);

	return result;
}

String RegionGenerator::ConnectorDef::get_desc() const
{
	return doorType.get() ? doorType->get_name().to_string() : doorType.get_name().to_string();
}

//

void RegionGenerator::setup_generator(PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region * _region, Library* _library, Random::Generator & _useRG)
{
	an_assert(_region);
	an_assert(_region->get_region_type());

	auto const * regionType = _region->get_region_type();

	_generator.use_generation_rules(regionType->get_generation_rules());
	_generator.use_update_and_validate(Utils::update_and_validate);

	for_every_ref(insideRegionPiece, regionType->get_inside_region_pieces())
	{
		_generator.add_piece_type(insideRegionPiece);
	}

	auto* game = Game::get();

	for_every(includedRegionType, regionType->get_included_region_types())
	{
		if (auto* i = includedRegionType->get_included())
		{
			cast_to_nonconst(i)->load_on_demand_if_required();
		}
		if (! game || game->is_ok_to_use_for_region_generation(includedRegionType->get_included()))
		{
			PieceCombiner::AddPieceTypeParams params;
			params.with_limit_count(includedRegionType->get_limit_count());
			params.can_be_created_only_via_connector_with_create_new_piece_mark(includedRegionType->get_can_be_created_only_via_connector_with_new_piece_mark());
			params.with_probability_coef(includedRegionType->get_probability_coef());
			params.with_mul_probability_coef(includedRegionType->get_mul_probability_coef());
			_generator.add_piece_type(includedRegionType->get_included()->get_as_region_piece(), params);
		}
	}
	for_every(includeRoomType, regionType->get_included_room_types())
	{
		if (auto* i = includeRoomType->get_included())
		{
			cast_to_nonconst(i)->load_on_demand_if_required();
		}
		if (!game || game->is_ok_to_use_for_region_generation(includeRoomType->get_included()))
		{
			PieceCombiner::AddPieceTypeParams params;
			params.with_limit_count(includeRoomType->get_limit_count());
			params.can_be_created_only_via_connector_with_create_new_piece_mark(includeRoomType->get_can_be_created_only_via_connector_with_new_piece_mark());
			params.with_probability_coef(includeRoomType->get_probability_coef());
			params.with_mul_probability_coef(includeRoomType->get_mul_probability_coef());
			_generator.add_piece_type(includeRoomType->get_included()->get_as_region_piece(), params);
		}
	}
	
	if (_library)
	{
		WorldSettingConditionContext settingConditionContext(_region);
		for_every(includedSettingCondition, regionType->get_included_region_type_setting_conditions())
		{
			for_every_ptr(regionType, _library->get_region_types().get_using(*includedSettingCondition->get_setting_condition(), settingConditionContext))
			{
				regionType->load_on_demand_if_required();
				if (!game || game->is_ok_to_use_for_region_generation(regionType))
				{
					PieceCombiner::AddPieceTypeParams params;
					params.can_be_created_only_via_connector_with_create_new_piece_mark(includedSettingCondition->get_can_be_created_only_via_connector_with_new_piece_mark());
					params.with_probability_coef(includedSettingCondition->get_probability_coef());
					params.with_mul_probability_coef(includedSettingCondition->get_mul_probability_coef());
					_generator.add_piece_type(regionType->get_as_region_piece(), params);
				}
			}
		}
		for_every(includedSettingCondition, regionType->get_included_room_type_setting_conditions())
		{
			for_every_ptr(roomType, _library->get_room_types().get_using(*includedSettingCondition->get_setting_condition(), settingConditionContext))
			{
				roomType->load_on_demand_if_required();
				if (!game || game->is_ok_to_use_for_region_generation(roomType))
				{
					PieceCombiner::AddPieceTypeParams params;
					params.can_be_created_only_via_connector_with_create_new_piece_mark(includedSettingCondition->get_can_be_created_only_via_connector_with_new_piece_mark());
					params.with_probability_coef(includedSettingCondition->get_probability_coef());
					params.with_mul_probability_coef(includedSettingCondition->get_mul_probability_coef());
					_generator.add_piece_type(roomType->get_as_region_piece(), params);
				}
			}
		}
	}

	regionType->setup_region_generator_add_pieces(_generator, _region, _useRG);
}

bool RegionGenerator::generate_and_add_to_sub_world(SubWorld* _inSubWorld, Region* _inRegion, RegionType const * _regionType, Optional<Random::Generator> const & _rgForRegion, Random::Generator* _randomGenerator, std::function<void(PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Framework::Region* _forRegion)> _setup_generator, std::function<void(Region* _region)> _setup_region, OUT_ Region** _regionPtr, bool _generateRooms)
{
	Random::Generator defaultRandomGenerator;
	if (!_randomGenerator)
	{
		_randomGenerator = &defaultRandomGenerator;
	}
	Random::Generator const generatorUsed = _rgForRegion.get(*_randomGenerator);

#ifdef LOG_WORLD_GENERATION
	output(TXT("generating region \"%S\" using rg:%S"), _regionType->get_name().to_string().to_char(), generatorUsed.get_seed_string().to_char());
#endif

	cast_to_nonconst(_regionType)->load_on_demand_if_required();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	bool debugNow = false;
	goto START_GENERATION;
	DEBUG_GENERATION:
	debugNow = true;
	START_GENERATION:
#endif
	// create region and add to sub world
	Region* region;
	Game::get()->perform_sync_world_job(TXT("create region"), [&region, _inSubWorld, _inRegion, _regionType, generatorUsed]()
	{
		region = new Region(_inSubWorld, _inRegion, _regionType, generatorUsed);
	});

	Random::Generator useRG = region->get_individual_random_generator();
	an_assert(useRG == generatorUsed);

	PieceCombiner::Generator<Framework::RegionGenerator> generator;

	// use created region as parameters provider, there might be already some variables set that will be required when generating pieces
	generator.set_parameters_provider(region);

	{
		WheresMyPoint::Context context(region);
		context.set_random_generator(useRG);
		region->get_region_type()->get_wheres_my_point_processor_setup_parameters().update(context);
	}
	{
		WheresMyPoint::Context context(region);
		context.set_random_generator(useRG);
		region->get_region_type()->get_wheres_my_point_processor_on_setup_generator().update(context);
	}
	// get_wheres_my_point_processor_on_generate is below, where we know generator
	an_assert(_setup_generator);
	_setup_generator(generator, region);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	bool wasDebugVisualiserActive = false;
	if (auto * gdv = generator.access_debug_visualiser())
	{
		wasDebugVisualiserActive = gdv->is_active();
		if (!wasDebugVisualiserActive && debugNow)
		{
			wasDebugVisualiserActive = true;
			gdv->activate();
		}
	}
#endif

	if (_setup_region)
	{
		_setup_region(region);
	}

	{
		RegionGeneratorContextOwner rgc(region, &generator);
		WheresMyPoint::Context context(&rgc);
		context.set_random_generator(useRG);
		region->get_region_type()->get_wheres_my_point_processor_on_generate().update(context);
	}

	// generate generator
#ifdef LOG_WORLD_GENERATION
	::System::TimeStamp startedGenerationTimeStamp;
	startedGenerationTimeStamp.reset();
	{
		ScopedOutputLock outputLock;
		{
			LogInfoContext log;
			generator.output_setup(log);
			log.output_to_output();
		}
	}
#endif
	auto useRGForPieceCombiner = useRG;
	PieceCombiner::GenerationResult::Type generationResult = generator.generate(&useRGForPieceCombiner);

#ifdef LOG_WORLD_GENERATION
	float generationTime = startedGenerationTimeStamp.get_time_since();
	output(TXT("generated in %.3fs"), generationTime);
#endif

	if (generationResult != PieceCombiner::GenerationResult::Success)
	{
		generator.set_parameters_provider(nullptr);
		error(TXT("failed region generation \"%S\""), _regionType->get_name().to_string().to_char());
#ifdef AN_DEVELOPMENT_OR_PROFILER
		goto DEBUG_GENERATION;
#endif
		an_assert(false, TXT("failed region generation \"%S\""), _regionType->get_name().to_string().to_char());
		Game::get()->perform_sync_world_job(TXT("create region"), [&region]()
		{
			delete region;
		});
		return false;
	}

	region->generate_environments();

	bool result = true;

	// go through all connectors, create doors and put info about those doors in custom data 
	// generate and add things - when connectors are added, add doors in subworld
	for_every_ptr(connector, generator.access_all_connector_instances())
	{
		ConnectorInstanceData & conData = connector->access_data();
		if (!connector->is_internal_external() && ! connector->is_blocked())
		{
			an_assert(connector->is_connected(), TXT("not required, but at this point of development, it should be connected"));
			todo_note(TXT("if it is not connected, check if it is \"leaveOpen\", if so, leave it as it is, just return open door in array"));
			todo_note(TXT("for time being it is better to use slotConnectors to create hanging connectors in generation"));
			PieceCombiner::ConnectorInstance<RegionGenerator>* otherConnector = connector->get_connector_on_other_end();
			ConnectorInstanceData * otherConData = otherConnector? &otherConnector->access_data() : nullptr;
			if (!conData.door)
			{
				if (otherConData && otherConData->door)
				{
					conData.door = otherConData->door;
				}
				else
				{
					if (connector->get_connector()->def.doorType.get())
					{
						Game::get()->perform_sync_world_job(TXT("create door"), [&conData, _inSubWorld, &connector]()
						{
							conData.door = new Door(_inSubWorld, connector->get_connector()->def.doorType.get());
						});
						// will connect other one eventually
					}
					else
					{
						an_assert(!connector->get_connector()->def.doorType.get_name().is_valid());
					}
				}
			}
		}
	}

	// then go deeper down to rooms - at room level it's obvious
	for_every_ptr(piece, generator.access_all_piece_instances())
	{
		useRG.advance_seed(0, 1);
		PieceDef const & def = piece->get_piece()->def;
		if (def.regionType.get())
		{
			if (!generate_and_add_to_sub_world(_inSubWorld, region, def.regionType.get(), piece->get_data().useRGForCreatedPiece, &useRG,
				[&generator, piece, def, &useRG
#ifdef AN_DEVELOPMENT_OR_PROFILER
				, wasDebugVisualiserActive
#endif
				](PieceCombiner::Generator<Framework::RegionGenerator> & _newGenerator, Framework::Region* _forRegion)
				{
					// generate region
					_newGenerator.access_generation_context().generationTags = generator.get_generation_context().generationTags;
#ifdef AN_DEVELOPMENT_OR_PROFILER
					// switch to new one
					if (auto * dv = _newGenerator.access_debug_visualiser())
					{
						if (auto * gdv = generator.access_debug_visualiser())
						{
							if (wasDebugVisualiserActive)
							{
								dv->activate();
								if (auto * font = gdv->get_font())
								{
									dv->use_font(font);
								}
							}
						}
					}
#endif

					// add outer connectors
					for_every_ptr(ownedConnector, piece->get_owned_connectors())
					{
						PieceCombiner::Connector<RegionGenerator> const* connectorType = ownedConnector->get_connector();
						if (connectorType->name.is_valid())
						{
							// check if for this connector there is "outer" inside this region type
							if (PieceCombiner::Connector<RegionGenerator> const* insideOuterConnectorType = def.regionType->find_inside_outer_region_connector(connectorType->name))
							{
								// there is, setup connector data to have it connected to same door
								// opposite side as it will be connected to something inside and with that thing having opposite side it will be just as internal connector
								// (inside this region type) would be connected to something outside
								_newGenerator.add_outer_connector(insideOuterConnectorType, ownedConnector, ownedConnector->get_data(), PieceCombiner::ConnectorSide::opposite(ownedConnector->get_side()));
							}
						}
					}

					// may require outer connectors
					Framework::RegionGenerator::setup_generator(_newGenerator, _forRegion, Framework::Library::get_current(), useRG);
				}, _setup_region, nullptr, _generateRooms))
			{
				result = false;
			}
		}
		else if (def.roomType.get())
		{
			// generate room
			// 1. create room
			Room* room;
			Game::get()->perform_sync_world_job(TXT("create room"), [&room, _inSubWorld, &region, &def, piece, &useRG]()
			{
				room = new Room(_inSubWorld, region, def.roomType.get(), piece->get_data().useRGForCreatedPiece.is_set()? piece->get_data().useRGForCreatedPiece.get() : useRG.spawn());
				ROOM_HISTORY(room, TXT("via region generator"));
			});
			// 2. (above) setup random generator and door type replacer - BEFORE WE CONNECT DOORS! (we could do it later but why should we change it numorous times?)
			// 3. create doors for that room
			for_every_ptr(ownedConnector, piece->get_owned_connectors())
			{
				if (ownedConnector->get_data().door)
				{
					DoorInRoom* dir = nullptr;
					Game::get()->perform_sync_world_job(TXT("create door in room"), [&room, &ownedConnector, &dir]()
					{
						scoped_call_stack_info(TXT("create door in room"));
						if (Game::get()->does_want_to_cancel_creation())
						{
							return;
						}
						dir = new DoorInRoom(room);
						{
							scoped_call_stack_info(TXT("setup"));
							DOOR_IN_ROOM_HISTORY(dir, TXT("created via region generator"));
							dir->set_outer_name(ownedConnector->get_connector()->name); // to allow overriding this door in room with door override_ for room type
							dir->access_tags().set_tags_from(ownedConnector->get_connector()->def.doorInRoomTags);
							dir->set_connector_tag(ownedConnector->get_connector()->connectorTag);
						}
						if (ownedConnector->get_side() == PieceCombiner::ConnectorSide::A)
						{
							scoped_call_stack_info(TXT("link a"));
							DOOR_IN_ROOM_HISTORY(dir, TXT("region generator: linking as A to d%p"), ownedConnector->get_data().door);
							ownedConnector->get_data().door->link_a(dir);
						}
						else if (ownedConnector->get_side() == PieceCombiner::ConnectorSide::B)
						{
							scoped_call_stack_info(TXT("link b"));
							DOOR_IN_ROOM_HISTORY(dir, TXT("region generator: linking as B to d%p"), ownedConnector->get_data().door);
							ownedConnector->get_data().door->link_b(dir);
						}
						else
						{
							scoped_call_stack_info(TXT("link c"));
							DOOR_IN_ROOM_HISTORY(dir, TXT("region generator: linking to d%p"), ownedConnector->get_data().door);
							ownedConnector->get_data().door->link(dir);
						}
						if (ownedConnector->get_connector()->def.doorInRoomTags.has_tag(NAME(vrElevatorAltitude)))
						{
							dir->set_vr_elevator_altitude(ownedConnector->get_connector()->def.doorInRoomTags.get_tag(NAME(vrElevatorAltitude)));
						}
					});
					if (dir)
					{
						dir->init_meshes();
					}
				}
			}
			// 4. generate room? or leave so it can be generated on demand or by other means
			if (_generateRooms)
			{
				room->generate();
			}
		}
	}

	if (result)
	{
		if (auto* rgi = region->get_region_type()->get_region_generator_info())
		{
			result &= rgi->post_generate(region);
		}
	}

	if (result)
	{
		assign_optional_out_param(_regionPtr, region);
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (wasDebugVisualiserActive)
	{
		if (auto * gdv = generator.access_debug_visualiser())
		{
			gdv->deactivate();
		}
	}
#endif

	return result;
}

bool RegionGenerator::apply_renamer_to(PieceCombiner::Piece<RegionGenerator> & _piece, LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _piece.def.apply_renamer(_renamer, _library);
	for_every(connector, _piece.connectors)
	{
		result &= apply_renamer_to(*connector, _renamer, _library);
	}

	return result;
}

bool RegionGenerator::apply_renamer_to(PieceCombiner::Connector<RegionGenerator> & _connector, LibraryStoredRenamer const & _renamer, Library* _library)
{
	bool result = true;

	result &= _connector.def.apply_renamer(_renamer, _library);

	return result;
}

//

REGISTER_FOR_FAST_CAST(RegionGeneratorContextOwner);

bool RegionGeneratorContextOwner::store_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value)
{
	return region->store_value_for_wheres_my_point(_byName, _value);
}

bool RegionGeneratorContextOwner::restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const
{
	return region->restore_value_for_wheres_my_point(_byName, _value);
}

bool RegionGeneratorContextOwner::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return region->store_convert_value_for_wheres_my_point(_byName, _to);
}

bool RegionGeneratorContextOwner::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return region->rename_value_forwheres_my_point(_from, _to);
}

bool RegionGeneratorContextOwner::store_global_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value)
{
	return region->store_global_value_for_wheres_my_point(_byName, _value);
}

bool RegionGeneratorContextOwner::restore_global_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const
{
	return region->restore_global_value_for_wheres_my_point(_byName, _value);
}

WheresMyPoint::IOwner* RegionGeneratorContextOwner::get_wmp_owner()
{
	return region;
}
