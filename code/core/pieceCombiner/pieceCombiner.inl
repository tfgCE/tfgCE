#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define LOG_PIECE_COMBINER
//#define LOG_PIECE_COMBINER_FAILS
//#define LOG_PIECE_COMBINER_DETAILS
//#define LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
//#define LOG_PIECE_COMBINER_SUCCESS
#endif

namespace ConnectorType
{
	Type parse(String const & _string)
	{
		if (_string == TXT("noConnector") ||
			_string == TXT("nullConnector"))
		{
			return No;
		}
		if (_string == TXT("external") ||
			_string == TXT("externalConnector") ||
			_string == TXT("connector"))
		{
			return Normal;
		}
		if (_string == TXT("child") ||
			_string == TXT("childConnector"))
		{
			return Child;
		}
		if (_string == TXT("parent") ||
			_string == TXT("parentConnector"))
		{
			return Parent;
		}
		if (_string == TXT("internal") ||
			_string == TXT("internalConnector"))
		{
			return Internal;
		}
		return Unknown;
	}
};

///////////////////////////////////////////////////////////////////////////////////
// GENERATOR

template <typename CustomType>
Generator<CustomType>::Generator(GenerationRules<CustomType> const * _generationRulesSource)
: checkpointId(0)
#ifdef PIECE_COMBINER_DEBUG
, workingWithSteps(false)
, insideStep(0)
, debugIdx(0)
#endif
, generationSpace(GenerationSpace<CustomType>::get_one())
{
	if (_generationRulesSource)
	{
		generationRulesSource = *_generationRulesSource;
	}

#ifdef PIECE_COMBINER_VISUALISE
	debugVisualiser = new DebugVisualiser<CustomType>(this);
#endif
}

template <typename CustomType>
Generator<CustomType>::~Generator()
{
#ifdef PIECE_COMBINER_VISUALISE
	debugVisualiser = nullptr;
#endif
	clear();
	generationSpace->release();
}

template <typename CustomType>
bool Generator<CustomType>::is_valid_step_call() const
{
#ifdef PIECE_COMBINER_DEBUG
	return ! workingWithSteps || insideStep;
#endif
	return true;
}

template <typename CustomType>
void Generator<CustomType>::mark_failed()
{
	++failCount;
}

template <typename CustomType>
bool Generator<CustomType>::check_fail_count()
{
#ifdef LOG_PIECE_COMBINER_FAILS
	if (failCount >= 10000)
	{
		log_to_file(TXT("fail: exceeded fail count!"));
	}
#endif
	return failCount < 10000;
}

template <typename CustomType>
bool Generator<CustomType>::has_piece_type(Piece<CustomType> const * _piece) const
{
	for_every(piece, pieces)
	{
		if (piece->piece == _piece)
		{
			return true;
		}
	}
	return false;
}

template <typename CustomType>
void Generator<CustomType>::output_setup(LogInfoContext & _log) const
{
	_log.log(TXT("generator setup info"));
	LOG_INDENT(_log);
	_log.log(TXT("piece types"));
	{
		LOG_INDENT(_log);
		for_every(piece, pieces)
		{
			_log.log(TXT("[%05.3f] %S"), piece->piece->probabilityCoef, piece->piece->def.get_desc().to_char());
		}
	}
	_log.log(TXT("provided pieces"));
	{
		LOG_INDENT(_log);
		for_every(piece, providedPieces)
		{
			_log.log(TXT("[%05.3f] %S"), piece->piece->probabilityCoef, piece->piece->def.get_desc().to_char());
		}
	}
	_log.log(TXT("outer connectors"));
	{
		LOG_INDENT(_log);
		for_every(oc, outerConnectors)
		{
			_log.log(TXT("/%S/ %S"), oc->connector->name.to_char(), oc->connector->def.get_desc().to_char());
		}
	}
}

template <typename CustomType>
void Generator<CustomType>::add_piece_type(Piece<CustomType> const * _piece, AddPieceTypeParams const & _params)
{
	memory_leak_suspect;
	UsePiece<CustomType> usePiece;
	usePiece.piece = _piece;
	usePiece.canBeCreatedOnlyViaConnectorWithCreateNewPieceMark = _params.canBeCreatedOnlyViaConnectorWithCreateNewPieceMark;
	usePiece.probabilityCoef = _params.probabilityCoef;
	usePiece.mulProbabilityCoef = _params.mulProbabilityCoef;
	if (_params.limitCount)
	{
		usePiece.limitCount = _params.limitCount;
	}
	pieces.push_back_unique(usePiece);
	forget_memory_leak_suspect;
}

template <typename CustomType>
int Generator<CustomType>::add_piece(Piece<CustomType> const * _piece, typename CustomType::PieceInstanceData const & _data)
{
	an_assert(steps.is_empty(), TXT("you should not use this method when generation is in progress"));
	memory_leak_suspect;
	providedPieces.push_back(ProvidedPiece(_piece, _data));
	forget_memory_leak_suspect;
	return providedPieces.get_size() - 1;
}

template <typename CustomType>
int Generator<CustomType>::add_outer_connector(Connector<CustomType> const * _connector, ConnectorInstance<CustomType> const * _outerConnectorInstance, typename CustomType::ConnectorInstanceData const & _data, ConnectorSide::Type _side)
{
	an_assert(steps.is_empty(), TXT("you should not use this method when generation is in progress"));
	memory_leak_suspect;
	outerConnectors.push_back(OuterConnector(_connector, _outerConnectorInstance, _data, _side));
	forget_memory_leak_suspect;
	return outerConnectors.get_size() - 1;
}

template <typename CustomType>
typename Generator<CustomType>::OuterConnector * Generator<CustomType>::get_outer_connector(int _outerConnectorIndex)
{
	return outerConnectors.is_index_valid(_outerConnectorIndex)? &outerConnectors[_outerConnectorIndex] : nullptr;
}

template <typename CustomType>
ConnectorInstance<CustomType> * Generator<CustomType>::get_outer_connector_instance(int _outerConnectorIndex)
{
	return outerConnectors.is_index_valid(_outerConnectorIndex)? allConnectorInstances[outerConnectors[_outerConnectorIndex].index] : nullptr;
}

template <typename CustomType>
void Generator<CustomType>::step__add_piece_instance_to_all(PieceInstance<CustomType>* _pieceInstance)
{
	an_assert(is_valid_step_call());
	an_assert(!allPieceInstances.does_contain(_pieceInstance));
	_pieceInstance->add_ref();
	memory_leak_suspect;
	allPieceInstances.push_back(_pieceInstance);
	forget_memory_leak_suspect;
}

template <typename CustomType>
void Generator<CustomType>::step__add_connector_instance_to_all(ConnectorInstance<CustomType>* _connectorInstance)
{
	an_assert(is_valid_step_call());
	an_assert(!allConnectorInstances.does_contain(_connectorInstance));
	_connectorInstance->add_ref();
	memory_leak_suspect;
	allConnectorInstances.push_back(_connectorInstance);
	forget_memory_leak_suspect;
}

template <typename CustomType>
void Generator<CustomType>::step__remove_piece_instance_from_all(PieceInstance<CustomType>* _pieceInstance)
{
	an_assert(is_valid_step_call());
	an_assert(allPieceInstances.does_contain(_pieceInstance));
	allPieceInstances.remove(_pieceInstance);
	_pieceInstance->release_ref();
}

template <typename CustomType>
void Generator<CustomType>::step__remove_connector_instance_from_all(ConnectorInstance<CustomType>* _connectorInstance)
{
	an_assert(is_valid_step_call());
	an_assert(allConnectorInstances.does_contain(_connectorInstance));
	allConnectorInstances.remove(_connectorInstance);
	_connectorInstance->release_ref();
}

template <typename CustomType>
void Generator<CustomType>::prepare_for_generation()
{
	generationRules.instantiate(generationRulesSource, *randomGenerator);
	loopsTaken = 0; // reset loops taken count
	failCount = 0; // clear fail count
	for_every(usePiece, pieces)
	{
		usePiece->instantiate(*randomGenerator);
	}
}

template <typename CustomType>
void Generator<CustomType>::use_generation_rules(GenerationRules<CustomType> const * _generationRules)
{
	if (_generationRules)
	{
		generationRulesSource = *_generationRules;
	}
	else
	{
		generationRulesSource.clear();
	}
}

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate(Random::Generator * _randomGenerator, int _tryCount)
{
	randomGenerator = _randomGenerator ? _randomGenerator : &ownRandomGenerator;
#ifdef PIECE_COMBINER_DEBUG
	workingWithSteps = true;
#endif

#ifdef LOG_PIECE_COMBINER
	log_to_file(TXT("start generation with rg:%S"), randomGenerator->get_seed_string().to_char());
#endif

	GenerationResult::Type result = GenerationResult::Failed;
	int tryCount = _tryCount;
	while (tryCount)
	{
#ifdef LOG_PIECE_COMBINER
		log_to_file(TXT("prepare generation"));
#endif
		prepare_for_generation();
		// plan
		// 1. for outer connectors inside - turn them into connector instances
		// 2. for provided pieces - turn them into piece instances
		// 3. go through all not connected connectors and try to connect them to existing or create new piece
		// 4. if there are no available connectors (except for optionals) check if whole thing is complete - ie check if all pieces are connected
		// 5. if not connected, try to find one connector that is not connected but is part of piece that is complete and try to connect with piece that is not part of complete
		//    or with internal/external that will try to connect in same way
		// 6. after reaching not-complete check completness again, if failed, repeat previous point
		// rules:
		// i. outer connector instances (initial) can't be connected to each other
		// ii. always first try to do pieces for parents
		// iii. then choose internal/external if second connector in pair is connected - this way with chain of connectors - if we connect at one end, we should do everything to connect second end)
		// iv. when connecting to existing, prefer inaccessible (from point of view of this one), then accessible
		// v. if completness fails and we have closed, inaccessible part, fail and go back
		// vi. when changing parent, disconnect all normal (if they are ok, they will reconnect)
		// vii. parent can be changed when placed inside potential parent's space or in main space (will find any parent that matches)
		Random::Generator storedGenerator = *randomGenerator; // store in case of fail - we then advance original by 1 (offset), as failed one may depend on time
#ifdef LOG_PIECE_COMBINER_SUCCESS
		::System::TimeStamp startedGenerationTimeStamp;
		startedGenerationTimeStamp.reset();
#endif
#ifdef LOG_PIECE_COMBINER
		log_to_file(TXT("use rg:%S"), randomGenerator->get_seed_string().to_char());
#endif
		result = generate__turn_outer_connector_into_connector_instance(0);
		if (result == GenerationResult::Success)
		{
			apply_post_generation();
#ifdef LOG_PIECE_COMBINER_SUCCESS
			log_to_file(TXT("successful generation %i %i in %.3fs (in %i loops, with %i fails)"), allPieceInstances.get_size(), allConnectorInstances.get_size(), startedGenerationTimeStamp.get_time_since(), loopsTaken, failCount);
			int idx = 0;
			for_every_ptr(piece, allPieceInstances)
			{
				log_to_file(TXT("  %i. %S (%S)"), idx, piece->get_piece()->def.get_desc().to_char(), piece->get_data().get_desc().to_char());
				for_every_ptr(con, piece->ownedConnectors)
				{
					log_to_file(TXT("    %S/%S (%S) %c (%S:%S/%S %S)"),
						con->get_connector()->linkName.to_char(),
						con->get_connector()->linkNameAlt.to_char(),
						con->get_connector()->name.to_char(),
						con->is_available() ? 'A' : (con->is_blocked() ? 'b' : '-'),
						con->is_connected() && con->get_owner_on_other_end() && con->get_owner_on_other_end()->get_piece() ? con->get_owner_on_other_end()->get_piece()->def.get_desc().to_char() : TXT(""),
						con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkName.to_char() : TXT(""),
						con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkNameAlt.to_char() : TXT(""),
						con->is_connected() && con->get_owner_on_other_end() ? con->get_owner_on_other_end()->get_data().get_desc().to_char() : TXT(""));
				}
				++idx;
			}
#endif
			break;
		}
		else
		{
#ifdef LOG_PIECE_COMBINER_SUCCESS
			log_to_file(TXT("failed generation %i %i (%i %i) in %.3fs (in %i loops, with %i fails)"),
				allPieceInstances.get_size(), allConnectorInstances.get_size(),
				providedPieces.get_size(), outerConnectors.get_size(),
				startedGenerationTimeStamp.get_time_since(), loopsTaken, failCount);
#endif
		}
#ifdef LOG_PIECE_COMBINER_SUCCESS
#endif
		*randomGenerator = storedGenerator;
		randomGenerator->advance_seed(0, 1);

#ifdef LOG_PIECE_COMBINER
		log_to_file(TXT("another try"));
#endif
		--tryCount;
	}

#ifdef PIECE_COMBINER_DEBUG
	an_assert(insideStep == 0, TXT("should leave executing/reverting step"));
	workingWithSteps = false;
#endif

	if (result == GenerationResult::Success)
	{
#ifdef PIECE_COMBINER_VISUALISE
#ifndef AN_CLANG
		bool sep = check_if_there_are_separate_closed_cycles();
#endif
		debugVisualiser->mark_finished();
		debugVisualiser->show(Colour::green, String::printf(TXT("done!")));
#endif

#ifdef LOG_PIECE_COMBINER
		log_to_file(TXT("generation successful"));
#endif
		// remove all steps - not needed anymore
		for_every_ptr(step, steps)
		{
			delete step;
		}
		steps.clear();
		// remove all optional connectors 
		remove_all_not_needed_connectors();
		// for all connectors define side (it could be any)
		define_side_for_all_connectors();
	}
	else
	{
#ifdef LOG_PIECE_COMBINER
		log_to_file(TXT("generation failed"));
#endif
#ifdef PIECE_COMBINER_VISUALISE
		debugVisualiser->mark_finished();
		debugVisualiser->show(Colour::white, String::printf(TXT("generation failed")));
#endif
	}

	return result;
}

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::apply_post_generation()
{
	for_every_ptr(piece, allPieceInstances)
	{
		if (piece->get_piece() && piece->get_piece()->disconnectIrrelevantConnectorsPostGeneration)
		{
			for_every_ptr(connector, piece->get_owned_connectors())
			{
				if (!connector->is_internal_external() &&
					connector->is_connected())
				{
					if (auto* oPiece = connector->get_owner_on_other_end())
					{
						if (oPiece->get_piece() && oPiece->get_piece()->disconnectIrrelevantConnectorsPostGeneration)
						{
							bool disconnect = (oPiece == piece);
							if (!disconnect)
							{
								for_every_ptr(oConnector, piece->get_owned_connectors())
								{
									if (oConnector != connector &&
										oConnector->get_owner_on_other_end() == oPiece)
									{
										disconnect = true;
										break;
									}
								}
							}
							if (disconnect)
							{
								connector->post_generation__disconnect();
							}
						}
					}
				}
			}
		}
	}

	return GenerationResult::Success;
}

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate__turn_outer_connector_into_connector_instance(int _idx)
{
	// go through outerConnectors list (with recurrence) and add connectors instance for each one 
	if (_idx < outerConnectors.get_size())
	{
		checkpoint();
		// add connector as instance
		OuterConnector& outerConnector = outerConnectors[_idx];
		outerConnector.index = allConnectorInstances.get_size();
		GenerationSteps::CreateConnectorInstance<CustomType>* step = new GenerationSteps::CreateConnectorInstance<CustomType>(outerConnector.connector, generationSpace, outerConnector.inheritedConnectorTag, outerConnector.inheritedProvideConnectorTag);
		do_step(step);
		step->connectorInstance->data = outerConnector.data;
		step->connectorInstance->side = outerConnector.side; // have side as provided by outer connector
		step->connectorInstance->createdFromProvidedData = true;
		// try next
		GenerationResult::Type result = generate__turn_outer_connector_into_connector_instance(_idx + 1);
		if (result == GenerationResult::Success)
		{
			return result;
		}
		// revert and loop
		revert();
		mark_failed();
		return GenerationResult::Failed;
	}
	else
	{
		return generate__turn_provided_piece_into_piece_instance(0);
	}
}

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate__turn_provided_piece_into_piece_instance(int _idx)
{
	// go through providedPieces list (with recurrence) and add connectors instance for each one 
	if (_idx < providedPieces.get_size())
	{
		checkpoint();
		// add piece as instance
		ProvidedPiece& providedPiece = providedPieces[_idx];
		providedPiece.index = allPieceInstances.get_size();
		GenerationSteps::CreatePieceInstance<CustomType>* step = new GenerationSteps::CreatePieceInstance<CustomType>(providedPiece.piece.get(), generationSpace, nullptr);
		do_step(step);
		step->pieceInstance->data = providedPiece.data;
		step->pieceInstance->createdFromProvidedData = true;
		// try next
		GenerationResult::Type result = generate__turn_provided_piece_into_piece_instance(_idx + 1);
		if (result == GenerationResult::Success)
		{
			return result;
		}
		// revert and loop
		revert();
		mark_failed();
		return GenerationResult::Failed;
	}
	else
	{
		return generate__generation_loop();
	}
}

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate__generation_loop(GenerateLoopReason::Type _reason)
{
	++loopsTaken;
	if (loopsTaken > 20000)
	{
		// we've run of out time
#ifdef LOG_PIECE_COMBINER_FAILS
		log_to_file(TXT("fail: too many loops taken"));
#endif
		mark_failed();
		return GenerationResult::Failed;
	}

#ifdef LOG_PIECE_COMBINER
	static int pieceCombinerLoopIdx = 0;
	log_to_file(TXT("{pcli:%i} {di:%i} loop %i %i (l:%i, f:%i)"), pieceCombinerLoopIdx, debugIdx, allPieceInstances.get_size(), allConnectorInstances.get_size(), loopsTaken, failCount);
	++pieceCombinerLoopIdx;
	++debugIdx;
#endif

#ifdef PIECE_COMBINER_DEBUG
	validate_process__generation_space_aspect();
#endif

	bool isValid = ext_update_and_validate ? ext_update_and_validate(*this) : true;
	if (!isValid ||
		check_if_limits_are_broken() ||
		check_if_one_outer_connector_per_piece_for_piece_connections_is_compromised() ||
		check_if_uniqueness_for_piece_connections_is_compromised() ||
		check_if_connector_groups_are_compromised() ||
		check_if_there_are_impossible_generation_spaces() ||
		check_if_there_are_separate_closed_cycles() ||
		check_if_there_is_fail_connection())
	{
#ifdef LOG_PIECE_COMBINER_FAILS
		log_to_file(TXT("fail: invalid!"));
		if (!isValid)
		{
			log_to_file(TXT("fail: +- ! Validator not valid!"));
		}
		if (check_if_limits_are_broken())
		{
			log_to_file(TXT("fail: +- check_if_limits_are_broken!"));
		}
		if (check_if_one_outer_connector_per_piece_for_piece_connections_is_compromised())
		{
			log_to_file(TXT("fail: +- check_if_one_outer_connector_per_piece_for_piece_connections_is_compromised!"));
		}
		if (check_if_uniqueness_for_piece_connections_is_compromised())
		{
			log_to_file(TXT("fail: +- check_if_uniqueness_for_piece_connections_is_compromised!"));
		}
		if (check_if_connector_groups_are_compromised())
		{
			log_to_file(TXT("fail: +- check_if_connector_groups_are_compromised!"));
		}
		if (check_if_there_are_impossible_generation_spaces())
		{
			log_to_file(TXT("fail: +- check_if_there_are_impossible_generation_spaces!"));
		}
		if (check_if_there_are_separate_closed_cycles())
		{
			log_to_file(TXT("fail: +- check_if_there_are_separate_closed_cycles!"));
		}
		if (check_if_there_is_fail_connection())
		{
			log_to_file(TXT("fail: +- check_if_there_is_fail_connection!"));
		}
		log_to_file(TXT("FAILED:"));
		int idx = 0;
		for_every_ptr(piece, allPieceInstances)
		{
			log_to_file(TXT("  %i. %S (%S)"), idx, piece->get_piece()->def.get_desc().to_char(), piece->get_data().get_desc().to_char());
			for_every_ptr(con, piece->ownedConnectors)
			{
				log_to_file(TXT("    %S/%S (%S:%i) %c (blockCount:%i) (%S:%S/%S %S)"),
					con->get_connector()->linkName.to_char(),
					con->get_connector()->linkNameAlt.to_char(),
					con->get_connector()->name.to_char(),
					con->get_connector()->autoId,
					con->is_connected() ? 'v' : (con->is_available() ? 'A' : (con->is_blocked() ? '=' : '-')), con->blockCount,
					con->is_connected() && con->get_owner_on_other_end() && con->get_owner_on_other_end()->get_piece() ? con->get_owner_on_other_end()->get_piece()->def.get_desc().to_char() : TXT(""),
					con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkName.to_char() : TXT(""),
					con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkNameAlt.to_char() : TXT(""),
					con->is_connected() && con->get_owner_on_other_end() ? con->get_owner_on_other_end()->get_data().get_desc().to_char() : TXT(""));
				for_every(conBlocks, con->get_connector()->blocks)
				{
					log_to_file(TXT("       blocks %S"), conBlocks->to_char());
				}
				for_every(conBlocks, con->get_connector()->autoBlocks)
				{
					log_to_file(TXT("       auto blocks %i"), *conBlocks);
				}
			}
			++idx;
		}
#endif
#ifdef PIECE_COMBINER_VISUALISE
		clear_visualise_issues();
		String problem(TXT("??"));
		if (check_if_limits_are_broken()) problem = TXT("limits are broken"); else
		if (check_if_one_outer_connector_per_piece_for_piece_connections_is_compromised()) problem = TXT("piece one outer connector rule compromised"); else
		if (check_if_uniqueness_for_piece_connections_is_compromised()) problem = TXT("piece uniqueness compromised"); else
		if (check_if_connector_groups_are_compromised()) problem = TXT("connection groups are compromised"); else
		if (check_if_there_are_impossible_generation_spaces()) problem = TXT("impossible generation space"); else
		if (check_if_there_are_separate_closed_cycles()) problem = TXT("separate closed cycles"); else
		if (check_if_there_is_fail_connection()) problem = TXT("fail connection");
		debugVisualiser->show(Colour::red, problem);
		clear_visualise_issues();
#endif
		mark_failed();
		return GenerationResult::Failed;
	}

#ifdef LOG_PIECE_COMBINER
	int idx = 0;
	log_to_file(TXT("connector instances (outer)"));
	for_every_ptr(con, allConnectorInstances)
	{
		if (!con->is_outer())
		{
			continue;
		}
		log_to_file(TXT("  %i. %S/%S (%S:%i) [%S>%S] %c (blockCount:%i) (%S:%S/%S %S)%S%S"),
			idx,
			con->get_connector()->linkName.to_char(),
			con->get_connector()->linkNameAlt.to_char(),
			con->get_connector()->name.to_char(),
			con->get_connector()->autoId,
			con->connectorTag.to_char(),
			con->provideConnectorTag.is_set() ? con->provideConnectorTag.get().to_char() : TXT("-"),
			con->is_connected() ? 'v' : (con->is_available() ? 'A' : (con->is_blocked() ? '=' : '-')),
			con->blockCount,
			con->is_connected() && con->get_owner_on_other_end() && con->get_owner_on_other_end()->get_piece() ? con->get_owner_on_other_end()->get_piece()->def.get_desc().to_char() : TXT(""),
			con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkName.to_char() : TXT(""),
			con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkNameAlt.to_char() : TXT(""),
			con->is_connected() && con->get_owner_on_other_end() ? con->get_owner_on_other_end()->get_data().get_desc().to_char() : TXT(""),
			con->get_connector()->check_required_parameters(this)? TXT("") : TXT(" [required parameters not met]"),
			con->internalExternalConnector? TXT(" INT-EXT-CON") : TXT(" not-int-ext")
			);
		for_every(conBlocks, con->get_connector()->blocks)
		{
			log_to_file(TXT("       blocks %S"), conBlocks->to_char());
		}
		for_every(conBlocks, con->get_connector()->autoBlocks)
		{
			log_to_file(TXT("       auto blocks %i"), *conBlocks);
		}
		++idx;
	}
	idx = 0;
	log_to_file(TXT("piece instances"));
	for_every_ptr(piece, allPieceInstances)
	{
		log_to_file(TXT("  %i. %S (%S)"), idx, piece->get_piece()->def.get_desc().to_char(), piece->get_data().get_desc().to_char());
		for_every_ptr(con, piece->ownedConnectors)
		{
			log_to_file(TXT("    %S/%S (%S:%i) [%S>%S] %c (blockCount:%i) (%S:%S/%S %S)%S%S"),
				con->get_connector()->linkName.to_char(),
				con->get_connector()->linkNameAlt.to_char(),
				con->get_connector()->name.to_char(),
				con->get_connector()->autoId,
				con->connectorTag.to_char(),
				con->provideConnectorTag.is_set() ? con->provideConnectorTag.get().to_char() : TXT("-"),
				con->is_connected() ? 'v' : (con->is_available() ? 'A' : (con->is_blocked() ? '=' : '-')),
				con->blockCount,
				con->is_connected() && con->get_owner_on_other_end() && con->get_owner_on_other_end()->get_piece() ? con->get_owner_on_other_end()->get_piece()->def.get_desc().to_char() : TXT(""),
				con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkName.to_char() : TXT(""),
				con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkNameAlt.to_char() : TXT(""),
				con->is_connected() && con->get_owner_on_other_end() ? con->get_owner_on_other_end()->get_data().get_desc().to_char() : TXT(""),
				con->get_connector()->check_required_parameters(this)? TXT("") : TXT(" [required parameters not met]"),
				con->internalExternalConnector? TXT(" INT-EXT-CON") : TXT(" not-int-ext")
				);
			for_every(conBlocks, con->get_connector()->blocks)
			{
				log_to_file(TXT("       blocks %S"), conBlocks->to_char());
			}
			for_every(conBlocks, con->get_connector()->autoBlocks)
			{
				log_to_file(TXT("       auto blocks %i"), *conBlocks);
			}
		}
		++idx;
	}
#endif

#ifdef PIECE_COMBINER_VISUALISE
	debugVisualiser->show(Colour::white, String::printf(TXT("going on...")));
#endif

	// get usual connectors, try to avoid optionals
	ARRAY_STATIC(ConnectorInstance<CustomType>*, bestConnectorInstances, 16);//min(16 , allConnectorInstances.get_size()));
	find_free_connectors(bestConnectorInstances, _reason == GenerateLoopReason::TryOptionals ? FindFreeConnectorsReason::ForLoopWithOptionals : FindFreeConnectorsReason::ForLoop);

#ifdef LOG_PIECE_COMBINER
	log_to_file(TXT("  bestConnectorInstances: %i"), bestConnectorInstances.get_size());
#endif

	if (bestConnectorInstances.is_empty())
	{
		return generate__no_free_connectors();
	}

	// try for each found connector
	while (!bestConnectorInstances.is_empty())
	{
		int tryIdx = RandomUtils::ChooseFromContainer<ArrayStatic<ConnectorInstance<CustomType>*,16>, ConnectorInstance<CustomType>*>::choose(
			*randomGenerator, bestConnectorInstances, [](ConnectorInstance<CustomType>* _ci) { return _ci->get_connector()? _ci->get_connector()->chooseChance : 1.0f; });
		GenerationResult::Type result = generate__try_connecting(bestConnectorInstances[tryIdx]);
		if (result == GenerationResult::Success)
		{
			return result;
		}
		bestConnectorInstances.remove_fast_at(tryIdx);
		if (!check_fail_count())
		{
			break;
		}
	}

	// tried everything and failed
#ifdef LOG_PIECE_COMBINER_FAILS
	log_to_file(TXT("fail: run out of options (%i %i)"), allPieceInstances.get_size(), allConnectorInstances.get_size());
#endif
	mark_failed();
	return GenerationResult::Failed;
}

#ifdef PIECE_COMBINER_VISUALISE
template <typename CustomType>
void Generator<CustomType>::clear_visualise_issues()
{
	for_every_ptr(p, allPieceInstances)
	{
		p->visualiseIssue = false;
	}
	for_every_ptr(c, allConnectorInstances)
	{
		c->visualiseIssue = false;
	}
}
#endif

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate__try_to_complete()
{
#ifdef PIECE_COMBINER_DEBUG
	validate_process__generation_space_aspect();
#endif
	// get optionals (non-optionals should be ok)
	ARRAY_STATIC(ConnectorInstance<CustomType>*, bestConnectorInstances, 16);// min(16, allConnectorInstances.get_size()));
	find_free_connectors(bestConnectorInstances, FindFreeConnectorsReason::ToComplete);

	if (bestConnectorInstances.is_empty())
	{
		// there are no optionals?
#ifdef LOG_PIECE_COMBINER_FAILS
		log_to_file(TXT("fail: no connectors left"));
#endif
		mark_failed();
		return GenerationResult::Failed;
	}

	// but we're actually interested in non internal/external and those that part of main group
	while (!bestConnectorInstances.is_empty())
	{
		int tryIdx = randomGenerator->get_int(bestConnectorInstances.get_size());
		GenerationResult::Type result = generate__try_to_complete_with(bestConnectorInstances[tryIdx]);
		if (result == GenerationResult::Success)
		{
			return result;
		}
		bestConnectorInstances.remove_fast_at(tryIdx);
		if (!check_fail_count())
		{
			break;
		}
	}

	// try loop but with optionals, maybe we will add something that will help us?
	GenerationResult::Type result = generate__generation_loop(GenerateLoopReason::TryOptionals);
	if (result == GenerationResult::Success)
	{
		return result;
	}

	// tried everything and failed
#ifdef LOG_PIECE_COMBINER_FAILS
	log_to_file(TXT("fail: can't complete"));
#endif
	mark_failed();
	return GenerationResult::Failed;
}

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate__try_to_complete_with(ConnectorInstance<CustomType>* _tryConnector)
{
#ifdef PIECE_COMBINER_DEBUG
	validate_process__generation_space_aspect();
#endif
	an_assert(_tryConnector->get_in_generation_space() != nullptr);
	// try connecting with existing connectors
	// avoid connecting outer to outer
	// have to be in same space
	auto & connectorsInSameSpace = _tryConnector->get_in_generation_space()->connectors;
	ARRAY_STATIC(ConnectorInstance<CustomType>*, connectors, 16);// min(16, connectorsInSameSpace.get_size()));
	for_every_ptr(connector, connectorsInSameSpace)
	{
		if (connector->is_available() // is available
		 && (!_tryConnector->is_outer() || !connector->is_outer()) // not outer-outer
		 && (connector->internalExternalConnector || (connector->get_owner() && ! connector->get_owner()->was_accessed())) // internal/external or one of not main group
		 && connector->can_connect_with(_tryConnector)) // can connect with one provided
		{
			memory_leak_suspect;
			connectors.push_back_or_replace(connector, *randomGenerator);
			forget_memory_leak_suspect;
		}
	}
	if (!connectors.is_empty())
	{
		while (!connectors.is_empty())
		{
			int idx = randomGenerator->get_int(connectors.get_size());
			if (ConnectorInstance<CustomType>* connector = connectors[idx])
			{
				checkpoint();
				_tryConnector->connect_with(this, connector);
				GenerationResult::Type result = GenerationResult::Failed;
				if (connector->internalExternalConnector)
				{
					// try to finish with internal/external pair
					result = generate__try_to_complete_with(connector->internalExternalConnector);
				}
				else
				{
					// try to finish - try loop again as we could add something already
					result = generate__generation_loop();
				}
				if (result == GenerationResult::Success)
				{
					return result;
				}
				revert();
			}
			connectors.remove_fast_at(idx);
			if (!check_fail_count())
			{
				break;
			}
		}
	}

#ifdef LOG_PIECE_COMBINER_FAILS
	log_to_file(TXT("fail: can't complete with"));
#endif
	mark_failed();
	return GenerationResult::Failed;
}

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate__no_free_connectors()
{
	// just make sure that it is valid
	bool isValid = ext_update_and_validate ? ext_update_and_validate(*this) : true;
	if (!isValid ||
		check_if_limits_are_broken() ||
		check_if_one_outer_connector_per_piece_for_piece_connections_is_compromised() ||
		check_if_uniqueness_for_piece_connections_is_compromised() ||
		check_if_connector_groups_are_compromised() ||
		check_if_there_are_impossible_generation_spaces() ||
		check_if_there_are_separate_closed_cycles(true) ||
		check_if_there_is_fail_connection())
	{
#ifdef LOG_PIECE_COMBINER_FAILS
		log_to_file(TXT("fail: invalid (no free connectors)"));
#endif
		mark_failed();
		return GenerationResult::Failed;
	}
	// check completness of graph - all pieces should be accessible, although only are checked through normals to get if they are properly used,
	// parent/children are checked just to be sure everything is used but they are not required for completness
	if (!check_if_complete())
	{
		return generate__try_to_complete();
	}
	// nothing more? all connected? yay! - this is actual only happy end that there is
	return GenerationResult::Success;
}

template <typename CustomType>
void Generator<CustomType>::find_free_connectors(ArrayStatic< ConnectorInstance<CustomType>*, 16 > & _bestConnectorInstances, FindFreeConnectorsReason::Type _reason)
{
	// get unconnected connectors
	// highest priority
	// parents first, internal/external with one connected in pair second, rest 
	float bestPriority = -INF_FLOAT;
	// when changing
	FindFreeConnectorsBest::Type bestWas = FindFreeConnectorsBest::Normal;
	for_every_ptr(connectorInstance, allConnectorInstances)
	{
		if (connectorInstance->is_available())
		{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
			{
				auto* con = connectorInstance;
				log_to_file(TXT(" free? %S/%S (%S:%i) [%S>%S] %c (blockCount:%i) (%S:%S/%S %S)%S%S"),
					con->get_connector()->linkName.to_char(),
					con->get_connector()->linkNameAlt.to_char(),
					con->get_connector()->name.to_char(),
					con->get_connector()->autoId,
					con->connectorTag.to_char(),
					con->provideConnectorTag.is_set() ? con->provideConnectorTag.get().to_char() : TXT("-"),
					con->is_connected() ? 'v' : (con->is_available() ? 'A' : (con->is_blocked() ? '=' : '-')),
					con->blockCount,
					con->is_connected() && con->get_owner_on_other_end() && con->get_owner_on_other_end()->get_piece() ? con->get_owner_on_other_end()->get_piece()->def.get_desc().to_char() : TXT(""),
					con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkName.to_char() : TXT(""),
					con->is_connected() && con->get_connector_on_other_end() ? con->get_connector_on_other_end()->get_connector()->linkNameAlt.to_char() : TXT(""),
					con->is_connected() && con->get_owner_on_other_end() ? con->get_owner_on_other_end()->get_data().get_desc().to_char() : TXT(""),
					con->get_connector()->check_required_parameters(this) ? TXT("") : TXT(" [required parameters not met]"),
					con->internalExternalConnector ? TXT(" INT-EXT-CON") : TXT(" not-int-ext")
				);
			}
#endif
			bool isOk = false;
			if (_reason == FindFreeConnectorsReason::ForLoop ||
				_reason == FindFreeConnectorsReason::ForLoopWithOptionals)
			{

#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
				if (connectorInstance->internalExternalConnector)
				{
					log_to_file(TXT("      connectorInstance->internalExternalConnector [%S]"), connectorInstance->internalExternalConnector->is_connected()? TXT("CONNECTED") : TXT("not connected"));
				}
#endif
				isOk = (connectorInstance->internalExternalConnector && connectorInstance->internalExternalConnector->is_connected()); // we're in pair internal/external and one of them is connected
				if (_reason == FindFreeConnectorsReason::ForLoop)
				{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
					log_to_file(TXT("      FOR LOOP"));
					if (! connectorInstance->is_optional())
					{
						log_to_file(TXT("      ok : not optional"));
					}
#endif
					isOk |= (!connectorInstance->is_optional() || randomGenerator->get_chance(generationRules.get_chance_to_use_optional_connector_as_normal())); // not optional or we're lucky
				}
				if (_reason == FindFreeConnectorsReason::ForLoopWithOptionals)
				{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
					log_to_file(TXT("      FOR LOOP WITH OPTIONALS"));
					if (connectorInstance->is_optional())
					{
						log_to_file(TXT("      ok : optional"));
					}
#endif
					isOk |= connectorInstance->is_optional(); // add optionals
				}
			}
			else if (_reason == FindFreeConnectorsReason::ToComplete)
			{
				isOk = ! connectorInstance->internalExternalConnector // is not internal/external
					&& (connectorInstance->get_owner() && connectorInstance->get_owner()->was_accessed()); // is part of main group
			}
			if (isOk)
			{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
				log_to_file(TXT("   ok"));
#endif
				if (Connector<CustomType> const * connector = connectorInstance->get_connector())
				{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
					log_to_file(TXT("   priority %.3f > %.3f bestPriority?"), connector->priority, bestPriority);
#endif
					if (connector->priority > bestPriority || _bestConnectorInstances.is_empty())
					{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
						log_to_file(TXT("   better one"));
#endif
						_bestConnectorInstances.clear();
						memory_leak_suspect;
						_bestConnectorInstances.push_back_or_replace(connectorInstance, *randomGenerator);
						forget_memory_leak_suspect;
						bestPriority = connector->priority;
						// reset best was
						if (connectorInstance->is_outer())
						{
							bestWas = FindFreeConnectorsBest::Outer;
						}
						else if (connector->type == ConnectorType::Parent)
						{
							bestWas = FindFreeConnectorsBest::Parent;
						}
						else if (connectorInstance->internalExternalConnector)
						{
							bestWas = FindFreeConnectorsBest::InternalExternal;
						}
						else
						{
							bestWas = FindFreeConnectorsBest::Normal;
						}
					}
					else if (connector->priority == bestPriority)
					{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
						log_to_file(TXT("   same as best"));
#endif
						if (bestWas != FindFreeConnectorsBest::Outer && connectorInstance->is_outer())
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   replace [outer]"));
#endif
							_bestConnectorInstances.clear();
							memory_leak_suspect;
							_bestConnectorInstances.push_back_or_replace(connectorInstance, *randomGenerator);
							forget_memory_leak_suspect;
							bestPriority = connector->priority;
							an_assert(bestWas < FindFreeConnectorsBest::Outer);
							bestWas = FindFreeConnectorsBest::Outer;
						}
						else if (bestWas == FindFreeConnectorsBest::Outer && !connectorInstance->is_outer())
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   ignore [outer]"));
#endif
							continue;
						}
						else if (bestWas != FindFreeConnectorsBest::Parent && connector->type == ConnectorType::Parent)
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   replace [Parent]"));
#endif
							_bestConnectorInstances.clear();
							memory_leak_suspect;
							_bestConnectorInstances.push_back_or_replace(connectorInstance, *randomGenerator);
							forget_memory_leak_suspect;
							bestPriority = connector->priority;
							an_assert(bestWas < FindFreeConnectorsBest::Parent);
							bestWas = FindFreeConnectorsBest::Parent;
						}
						else if (bestWas == FindFreeConnectorsBest::Parent && connector->type != ConnectorType::Parent)
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   ignore [parent]"));
#endif
							continue;
						}
						// then choose internal/external to pass connection (when we have pair internal/external and one of them is connected, connect second one)
						// with this, when one end of such chain is connected, we should do everything possible to connect other end
						else if (connectorInstance->internalExternalConnector && connectorInstance->internalExternalConnector->is_available())
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   ignore [internalExternalConnector]"));
#endif
							continue;
						}
						else if (bestWas != FindFreeConnectorsBest::InternalExternal && connectorInstance->internalExternalConnector)
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   replace [InternalExternal]"));
#endif
							_bestConnectorInstances.clear();
							memory_leak_suspect;
							_bestConnectorInstances.push_back_or_replace(connectorInstance, *randomGenerator);
							forget_memory_leak_suspect;
							bestPriority = connector->priority;
							an_assert(bestWas < FindFreeConnectorsBest::InternalExternal);
							bestWas = FindFreeConnectorsBest::InternalExternal;
						}
						else if (bestWas == FindFreeConnectorsBest::InternalExternal && !connectorInstance->internalExternalConnector)
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   ignore [InternalExternal]"));
#endif
							continue;
						}
						else
						{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
							log_to_file(TXT("   add"));
#endif

							memory_leak_suspect;
							_bestConnectorInstances.push_back_or_replace(connectorInstance, *randomGenerator);
							forget_memory_leak_suspect;
						}
					}
				}
			}
			else
			{
#ifdef LOG_PIECE_COMBINER_FIND_FREE_CONNECTORS
				log_to_file(TXT("   not good"));
#endif
			}
		}
	}
}

namespace TryConnectingUtils
{
	namespace Method
	{
		enum Type
		{
			ToInaccessible,
			ToAccessible,
			ToSelf,
			Num
		};
	};

	template <typename CustomType>
	bool should_try_connecting(Generator<CustomType> * _generator, /*Method::Type*/int _method, TryConnectingConnectorMethod::Type _tryConnectingConnectorMethod, ConnectorInstance<CustomType>* _toConnector, ConnectorInstance<CustomType>* _connector)
	{
		if (_toConnector == _connector)
		{
			// shouldn't try to connect to same connector
			return false;
		}
		PieceInstance<CustomType>* conOwner = _connector->get_owner();
		PieceInstance<CustomType>* toConOwner = _toConnector->get_owner();
		PieceInstance<CustomType>* closestCon = conOwner? conOwner : _connector->get_owner_on_other_end();
		PieceInstance<CustomType>* closestToCon = toConOwner ? toConOwner : _toConnector->get_owner_on_other_end();
		if (conOwner == toConOwner &&
			(!_toConnector->get_connector()->allowConnectingToSelf ||
			 !_connector->get_connector()->allowConnectingToSelf))
		{
			// can't connect to self
			return false;
		}
		if (_connector->is_available() // is available
			&& (!_toConnector->is_outer() || !_connector->is_outer()) // isn't outer to outer
			&& _connector->can_connect_with(_toConnector)) // can connect with one provided
		{
			if (_method == Method::ToInaccessible)
			{
				if (conOwner)
				{
					if (conOwner->was_accessed() || // we want inaccessible
						conOwner == toConOwner) // not to self
					{
						return false;
					}
				}
			}
			else if (_method == Method::ToAccessible)
			{
				if (conOwner)
				{
					if (!conOwner->was_accessed() || // we want accessible
						conOwner == toConOwner) // not to self
					{
						return false;
					}
				}
			}
			// check cycles (if rules request that)
			if (!_generator->get_generation_rules().should_allow_connecting_provided_pieces())
			{
				if (((conOwner && conOwner->was_created_from_provided_data()) || _connector->was_created_from_provided_data()) &&
					((toConOwner && toConOwner->was_created_from_provided_data()) || _toConnector->was_created_from_provided_data()))
				{
					// one or the other was created as provided - disallow connecting them
#ifdef LOG_PIECE_COMBINER_DETAILS
					log_to_file(TXT("disallow connecting provided pieces %S/%S and %S/%S"),
						closestCon && closestCon->get_piece() ? closestCon->get_piece()->def.get_desc().to_char() : _connector->get_connector()->linkName.to_char(),
						closestCon && closestCon->get_piece() ? closestCon->get_piece()->def.get_desc().to_char() : _connector->get_connector()->linkNameAlt.to_char(),
						closestToCon && closestToCon->get_piece() ? closestToCon->get_piece()->def.get_desc().to_char() : _toConnector->get_connector()->linkName.to_char(),
						closestToCon && closestToCon->get_piece() ? closestToCon->get_piece()->def.get_desc().to_char() : _toConnector->get_connector()->linkNameAlt.to_char());
#endif
					return false;
				}
			}
			if (_generator->get_generation_rules().get_number_of_pieces_required_in_cycle_to_allow_connection_to_other_cycle() > 0)
			{
				// parent and child connectors are not part of cycles check! (unless it is stated that they are)
				if ((_toConnector->get_connector()->type != ConnectorType::Parent && _toConnector->get_connector()->type != ConnectorType::Child) ||
					_generator->get_generation_rules().should_treat_parents_and_children_as_parts_of_cycle())
				{
					// if they are in same cycle, just follow normal rules, otherwise...
					if (!_generator->are_both_in_same_cycle(closestCon, closestToCon))
					{
						// cycle sizes are fine
						int requiredCycleSize = _generator->get_generation_rules().get_number_of_pieces_required_in_cycle_to_allow_connection_to_other_cycle();
						int conOwnerCycleSize = 0;
						int toConOwnerCycleSize = 0;
						int conOwnerCycleOuterConnected = 0;
						int toConOwnerCycleOuterConnected = 0;
						bool conCanCreateAnything = false;
						bool toConCanCreateAnything = false;
						_generator->calculate_cycle_info(closestCon, conOwnerCycleSize, conOwnerCycleOuterConnected, conCanCreateAnything);
						_generator->calculate_cycle_info(closestToCon, toConOwnerCycleSize, toConOwnerCycleOuterConnected, toConCanCreateAnything);
						if ((conOwnerCycleSize == 0 && (toConOwnerCycleSize >= requiredCycleSize || toConOwnerCycleOuterConnected == 0)) || // initial connection of outer to cycle without outer or cycle big enough
							(toConOwnerCycleSize == 0 && (conOwnerCycleSize >= requiredCycleSize || conOwnerCycleOuterConnected == 0)) || // as above but connectors reversed
							((conOwnerCycleSize >= requiredCycleSize || ! conCanCreateAnything) &&
							 (toConOwnerCycleSize >= requiredCycleSize || ! toConCanCreateAnything))) // both cycle have proper size (or cannot grow)
						{
							// that's fine
							// note: I could "! ()" above, but now it's clearer
						}
						else
						{
#ifdef LOG_PIECE_COMBINER_DETAILS
							log_to_file(TXT("cycles too small %S:%i %S:%i"),
								closestCon->get_piece()->def.get_desc().to_char(), conOwnerCycleSize,
								closestToCon->get_piece()->def.get_desc().to_char(), toConOwnerCycleSize);
#endif
							// cycles too small
							return false;
						}
					}
				}
			}
			if (_tryConnectingConnectorMethod < TryConnectingConnectorMethod::IgnorePreferEachConnectorHasUniquePiece)
			{
				if (_toConnector->should_prefer_each_connector_has_unique_piece() &&
					toConOwner && ! toConOwner->check_if_would_be_connected_as_unique_piece(conOwner))
				{
					// wouldn't be unique
					return false;
				}
				if (_connector->should_prefer_each_connector_has_unique_piece() &&
					conOwner && ! conOwner->check_if_would_be_connected_as_unique_piece(toConOwner))
				{
					// wouldn't be unique
					return false;
				}
			}

			return true;
		}
		return false;
	}
};

template <typename CustomType>
GenerationResult::Type Generator<CustomType>::generate__try_connecting(ConnectorInstance<CustomType>* _tryConnector)
{
#ifdef LOG_PIECE_COMBINER_DETAILS
	log_to_file(TXT("trying to connect \"%S/%S\" \"%S\""),
		_tryConnector->get_connector()->linkName.to_char(),
		_tryConnector->get_connector()->linkNameAlt.to_char(),
		_tryConnector->get_connector()->name.to_char());
#endif

	bool forceCreateNewPiece = false;
	bool allowFail = false;
	if (Connector<CustomType> const * connector = _tryConnector->get_connector())
	{
		allowFail = connector->allowFail;
		if (connector->forceCreateNewPieceLimit == 0 ||
			allPieceInstances.get_size() < connector->forceCreateNewPieceLimit)
		{
			if (randomGenerator->get_chance(connector->forceCreateNewPieceProbability))
			{
				forceCreateNewPiece = true;
#ifdef LOG_PIECE_COMBINER_DETAILS
				log_to_file(TXT("  force create new piece [1]"));
#endif
			}
		}
	}

	for_count(int, _tryConnectingConnectorMethod, TryConnectingConnectorMethod::NUM)
	{
		TryConnectingConnectorMethod::Type tryConnectingConnectorMethod = (TryConnectingConnectorMethod::Type)_tryConnectingConnectorMethod;
		bool doMethod = tryConnectingConnectorMethod == TryConnectingConnectorMethod::Default;

		// filter method
		{
			if (tryConnectingConnectorMethod == TryConnectingConnectorMethod::IgnorePreferCreatingNewPieces)
			{
				doMethod = _tryConnector->should_prefer_creating_new_pieces();
			}
			if (tryConnectingConnectorMethod == TryConnectingConnectorMethod::IgnorePreferEachConnectorHasUniquePiece)
			{
				doMethod = false;
				if (_tryConnector->should_prefer_each_connector_has_unique_piece())
				{
					doMethod = true;
				}
				else
				{
					// check if any other connector in this space is eager to give up on uniqueness
					auto& connectorsInSameSpace = _tryConnector->get_in_generation_space()->connectors;
					for_every_ptr(connector, connectorsInSameSpace)
					{
						if (connector->should_prefer_each_connector_has_unique_piece())
						{
							doMethod = true;
							break;
						}
					}
				}
			}
		}

		// try connecting
		if (doMethod)
		{
			if (_tryConnector->should_prefer_creating_new_pieces() &&
				tryConnectingConnectorMethod < TryConnectingConnectorMethod::IgnorePreferCreatingNewPieces)
			{
				forceCreateNewPiece = true;
#ifdef LOG_PIECE_COMBINER_DETAILS
				log_to_file(TXT("  force create new piece [2]"));
#endif
			}

			if (!forceCreateNewPiece)
			{
				an_assert(_tryConnector->get_in_generation_space() != nullptr);
				// try connecting with existing connectors from same space
				// avoid connecting outer to outer
				// have to be in same space
				auto& connectorsInSameSpace = _tryConnector->get_in_generation_space()->connectors;
				for (int method = 0; method < TryConnectingUtils::Method::Num; ++method)
				{
#ifdef LOG_PIECE_COMBINER_DETAILS
					log_to_file(TXT("  using method %i"), method);
#endif

					check_if_complete(_tryConnector);
					ARRAY_STATIC(ConnectorInstance<CustomType>*, connectors, 64);
					// add all connectors from same space
					for_every_ptr(connector, connectorsInSameSpace)
					{
						if (TryConnectingUtils::should_try_connecting(this, method, tryConnectingConnectorMethod, _tryConnector, connector))
						{
							memory_leak_suspect;
							connectors.push_back_or_replace(connector, *randomGenerator);
							forget_memory_leak_suspect;
						}
					}
					// add all outers
					for_every_ptr(connector, allConnectorInstances)
					{
						if (connector->is_outer())
						{
							if (TryConnectingUtils::should_try_connecting(this, method, tryConnectingConnectorMethod, _tryConnector, connector))
							{
								memory_leak_suspect;
								connectors.push_back_or_replace(connector, *randomGenerator);
								forget_memory_leak_suspect;
							}
						}
					}
					if (_tryConnector->get_connector()->type == ConnectorType::Parent &&
						_tryConnector->is_in_main_generation_space())
					{
						// add all child connectors from all pieces as long as this piece is in main generation space
						for_every_ptr(piece, allPieceInstances)
						{
							for_every_ptr(connector, piece->ownedConnectors)
							{
								if (connector->get_connector()->type == ConnectorType::Child)
								{
									if (TryConnectingUtils::should_try_connecting(this, method, tryConnectingConnectorMethod, _tryConnector, connector))
									{
										memory_leak_suspect;
										connectors.push_back_or_replace(connector, *randomGenerator);
										forget_memory_leak_suspect;
									}
								}
							}
						}
					}
					if (!connectors.is_empty())
					{
						{	// remove "only if nothing else"
							int onlyIfNothingElseCount = 0;
							for (int i = 0; i < connectors.get_size(); ++i)
							{
								if (connectors[i]->get_connector()->connectToOnlyIfNothingElse)
								{
									++onlyIfNothingElseCount;
								}
							}
							if (onlyIfNothingElseCount > 0 && onlyIfNothingElseCount < connectors.get_size())
							{
								for (int i = 0; i < connectors.get_size(); ++i)
								{
									if (connectors[i]->get_connector()->connectToOnlyIfNothingElse)
									{
										connectors.remove_fast_at(i);
										--i;
									}
								}
							}
						}
						while (!connectors.is_empty())
						{
							int idx = randomGenerator->get_int(connectors.get_size());
							if (ConnectorInstance<CustomType>* connector = connectors[idx])
							{
								checkpoint();
								_tryConnector->connect_with(this, connector);
								GenerationResult::Type result = generate__generation_loop();
								if (result == GenerationResult::Success)
								{
									return result;
								}
								revert();
							}
							connectors.remove_fast_at(idx);
							if (!check_fail_count())
							{
								break;
							}
						}
					}
					if (!check_fail_count())
					{
						break;
					}
				}

				if (_tryConnector->get_connector()->type == ConnectorType::Parent &&
					!_tryConnector->is_in_main_generation_space())
				{
					//error(TXT("tried to add parent when not in main generation space and no parent available!"));
#ifdef LOG_PIECE_COMBINER_FAILS
					log_to_file(TXT("fail: tried to add parent when not in main generation space and no parent available!"));
#endif
					mark_failed();
					return GenerationResult::Failed;
				}
			}

#ifdef LOG_PIECE_COMBINER_DETAILS
			log_to_file(TXT("trying to connect %S/%S %S (maybe create?)"),
				_tryConnector->get_connector()->linkName.to_char(),
				_tryConnector->get_connector()->linkNameAlt.to_char(),
				_tryConnector->get_connector()->name.to_char());
#endif

			if (_tryConnector->get_connector()->canCreateNewPieces ||
				forceCreateNewPiece)
			{
#ifdef LOG_PIECE_COMBINER_DETAILS
				log_to_file(TXT("trying to connect %S/%S %S (create!)"),
					_tryConnector->get_connector()->linkName.to_char(),
					_tryConnector->get_connector()->linkNameAlt.to_char(),
					_tryConnector->get_connector()->name.to_char());
#endif
				struct TryPiece
				{
					Piece<CustomType> const* piece;
					float probabilityCoef = 1.0f;
					TryPiece() {}
					TryPiece(Piece<CustomType> const* _piece, float _probabilityCoef) : piece(_piece), probabilityCoef(_probabilityCoef) {}
				};
				// try creating piece
				ARRAY_PREALLOC_SIZE(TryPiece, tryPieces, pieces.get_size());
				// filter pieces using limit count
				float pieceProbabilitySum = 0.0f;
				for_every(usePiece, pieces)
				{
					if (usePiece->actualLimitCount > 0)
					{
						int alreadyExisting = 0;
						for_every_ptr(pieceInstance, allPieceInstances)
						{
							if (pieceInstance->get_piece() == usePiece->piece.get())
							{
								++alreadyExisting;
							}
						}
						if (alreadyExisting >= usePiece->actualLimitCount)
						{
							continue;
						}
					}
					if (usePiece->canBeCreatedOnlyViaConnectorWithCreateNewPieceMark.is_valid())
					{
						if (_tryConnector->get_connector()->createNewPieceMark != usePiece->canBeCreatedOnlyViaConnectorWithCreateNewPieceMark)
						{
							continue;
						}
					}
#ifdef LOG_PIECE_COMBINER_DETAILS
					log_to_file(TXT("piece %S promising"), usePiece->piece->def.get_desc().to_char());
#endif
					// add only those that can be added
					if (usePiece->piece->can_connect_with(_tryConnector->get_connector()))
					{
#ifdef LOG_PIECE_COMBINER_DETAILS
						log_to_file(TXT("piece %S added!"), usePiece->piece->def.get_desc().to_char());
#endif
						float probCoef = usePiece->probabilityCoef.get(usePiece->piece->probabilityCoef) * usePiece->mulProbabilityCoef.get(1.0f);
						tryPieces.push_back(TryPiece(usePiece->piece.get(), probCoef));
						pieceProbabilitySum += probCoef;
					}
				}
				// for all valid, try to create piece
				while (!tryPieces.is_empty())
				{
					float chosenProb = randomGenerator->get_float(0.0f, pieceProbabilitySum);
					int idx = 0;
					for_every(tryPiece, tryPieces)
					{
						chosenProb -= tryPiece->probabilityCoef;
						if (chosenProb <= 0.0f)
						{
							// lower probability sum as we won't be able to choose this piece again
							pieceProbabilitySum -= tryPiece->probabilityCoef;
							break;
						}
						++idx;
					}
					idx = min(idx, tryPieces.get_size() - 1);
					if (Piece<CustomType> const* tryPiece = tryPieces[idx].piece)
					{
#ifdef LOG_PIECE_COMBINER_DETAILS
						log_to_file(TXT("try creating %S"), tryPiece->def.get_desc().to_char());
#endif
						if (tryPiece->can_connect_with(_tryConnector->get_connector()))
						{
							int triesLeft = clamp(tryPiece->get_amount_of_randomisation_tries(), 1, 2);
							while (triesLeft > 0)
							{
								checkpoint();
#ifdef LOG_PIECE_COMBINER_DETAILS
								log_to_file(TXT("try creating %S [tries left %i]"), tryPiece->def.get_desc().to_char(), triesLeft);
#endif
								GenerationSteps::CreatePieceInstance<CustomType>* step = new GenerationSteps::CreatePieceInstance<CustomType>(tryPiece, _tryConnector->get_in_generation_space(), _tryConnector);
								do_step(step);
								if (_tryConnector->get_connector()->type == ConnectorType::Parent)
								{
#ifdef LOG_PIECE_COMBINER_DETAILS
									log_to_file(TXT("move to inside space"));
#endif
									// if we created piece for parent connector (parent piece), move immediately to that parent's generation space
									_tryConnector->get_owner()->move_to_space(this, step->pieceInstance->get_generation_space());
								}
								GenerationResult::Type result = step->pieceInstance->generate__try_connecting(this, _tryConnector);
								if (result == GenerationResult::Success)
								{
									return result;
								}
								revert();
								--triesLeft;
							}
						}
					}
					tryPieces.remove_fast_at(idx);
					if (!check_fail_count())
					{
						break;
					}
				}
			}
			else
			{
#ifdef LOG_PIECE_COMBINER_DETAILS
				log_to_file(TXT("not trying to create"));
#endif
			}
		}
	}

	// can't connect this one, but maybe if this one might be blocked by something else, we could already block it? - check other connectors in same piece
	bool mightBeBlockedBySomethingElse = false;
	if (_tryConnector->get_owner())
	{
		for_every_ptr(connector, _tryConnector->get_owner()->get_owned_connectors())
		{
			if (connector != _tryConnector &&
				connector->is_available())
			{
				if (connector->get_connector()->blocks.does_contain(_tryConnector->get_connector()->name))
				{
					// iterated one might block our "tryConnector" and is also available!
					mightBeBlockedBySomethingElse = true;
					break;
				}
				if (connector->get_connector()->autoBlocks.does_contain(_tryConnector->get_connector()->autoId))
				{
					// iterated one might block our "tryConnector" and is also available!
					mightBeBlockedBySomethingElse = true;
					break;
				}
			}
		}
	}
	if (mightBeBlockedBySomethingElse)
	{
#ifdef LOG_PIECE_COMBINER_DETAILS
		log_to_file(TXT("might be blocked by something else, continue %S/%S %S"),
			_tryConnector->get_connector()->linkName.to_char(),
			_tryConnector->get_connector()->linkNameAlt.to_char(),
			_tryConnector->get_connector()->name.to_char());
#endif
	}
	if (allowFail)
	{
#ifdef LOG_PIECE_COMBINER_DETAILS
		log_to_file(TXT("fail, but we do allow failing here, block and continue %S/%S %S"),
			_tryConnector->get_connector()->linkName.to_char(),
			_tryConnector->get_connector()->linkNameAlt.to_char(),
			_tryConnector->get_connector()->name.to_char());
#endif
	}
	if (mightBeBlockedBySomethingElse || allowFail)
	{
		// let's try blocking this one and see what happens
		checkpoint();
		do_step(new GenerationSteps::BlockSingle<CustomType>(_tryConnector));
		GenerationResult::Type result = generate__generation_loop();
		if (result == GenerationResult::Success)
		{
			return result;
		}
		revert();
	}

#ifdef LOG_PIECE_COMBINER_FAILS
	log_to_file(TXT("fail: can't connect this one %S/%S %S"),
		_tryConnector->get_connector()->linkName.to_char(),
		_tryConnector->get_connector()->linkNameAlt.to_char(),
		_tryConnector->get_connector()->name.to_char());
#endif
	mark_failed();
	return GenerationResult::Failed;
}

template <typename CustomType>
void Generator<CustomType>::clear()
{
	// revert everything
	while (revert()) {}

	an_assert(steps.is_empty());
	for_every_ptr(step, steps)
	{
		delete step;
	}
	generationSpace->clear();
	for_every_ptr(connector, allConnectorInstances)
	{
		// disconnect all connectors. this is required for successful generator as for failed we reverted everything anyway
		if (connector->connectedTo)
		{
			connector->step__disconnect(this);
		}
		if (connector->internalExternalConnector)
		{
			connector->step__disconnect_internal_external(this);
		}
		connector->release_ref();
	}
	for_every_ptr(piece, allPieceInstances)
	{
		piece->release_ref();
	}
	allConnectorInstances.clear();
	allPieceInstances.clear();
}

template <typename CustomType>
void Generator<CustomType>::do_step(GenerationStep<CustomType>* _step)
{
	memory_leak_suspect;
	steps.push_back(_step);
	forget_memory_leak_suspect;
#ifdef PIECE_COMBINER_DEBUG
	++ insideStep;
#endif
	_step->execute(this);
#ifdef PIECE_COMBINER_DEBUG
	-- insideStep;
#endif
}

template <typename CustomType>
int Generator<CustomType>::checkpoint()
{
	GenerationSteps::Checkpoint<CustomType>* checkpoint = new GenerationSteps::Checkpoint<CustomType>(checkpointId++);
	do_step(checkpoint);
	return checkpoint->get_id();
}

template <typename CustomType>
bool Generator<CustomType>::revert()
{
	if (steps.get_size() == 0)
	{
		return false;
	}
	revert_checkpoint();
	return true;
}

template <typename CustomType>
bool Generator<CustomType>::revert_to(int _id)
{
	if (steps.get_size() == 0)
	{
		return false;
	}
	while (steps.get_size() > 0)
	{
		int id = revert_checkpoint();
		if (id == _id)
		{
			break;
		}
	}
	return true;
}

template <typename CustomType>
int Generator<CustomType>::revert_checkpoint()
{
	while (steps.get_size() > 0)
	{
		GenerationStep<CustomType>* lastStep = steps.get_last();
		bool checkpoint = false;
		int lastCheckpointId = 0;
		if (GenerationSteps::Checkpoint<CustomType>* cp = fast_cast<GenerationSteps::Checkpoint<CustomType> >(lastStep))
		{
			checkpoint = true;
			lastCheckpointId = cp->get_id();
		}
#ifdef PIECE_COMBINER_DEBUG
		++ insideStep;
#endif
		lastStep->revert(this);
#ifdef PIECE_COMBINER_DEBUG
		-- insideStep;
#endif
		delete lastStep;
		steps.remove_at(steps.get_size() - 1);
		if (checkpoint)
		{
			return lastCheckpointId;
		}
	}
	an_assert(false, TXT("expected to revert to checkpoint, but no checkpoint found"));
	return 0;
}

#ifdef PIECE_COMBINER_DEBUG
template <typename CustomType>
void Generator<CustomType>::validate_process__generation_space_aspect()
{
	for_every_ptr(connector, allConnectorInstances)
	{
		if (! connector->is_outer())
		{
			an_assert(connector->get_owner());
			if (connector->get_connector()->type == ConnectorType::Parent ||
				connector->get_connector()->type == ConnectorType::Normal)
			{
				an_assert(connector->get_in_generation_space() == connector->get_owner()->get_in_generation_space());
			}
			if (connector->get_connector()->type == ConnectorType::Child ||
				connector->get_connector()->type == ConnectorType::Internal)
			{
				an_assert(connector->get_in_generation_space() == connector->get_owner()->get_generation_space());
			}
		}
	}
}
#endif

template <typename CustomType>
bool Generator<CustomType>::check_if_complete(ConnectorInstance<CustomType>* _forConnector)
{
	if (allPieceInstances.get_size() == 0)
	{
		return false;
	}

	// clear accessibility info
	for_every_ptr(piece, allPieceInstances)
	{
		piece->clear_accessibility();
	}

	if (_forConnector && !_forConnector->get_owner())
	{
		return false;
	}
	// go through all pieces through connectors
	if (_forConnector)
	{
		_forConnector->get_owner()->check_if_complete();
	}
	else
	{
		allPieceInstances[0]->check_if_complete();
	}

	// check if all pieces were accessed
	for_every_ptr(piece, allPieceInstances)
	{
		if (!piece->was_accessed())
		{
			return false;
		}
	}

	for_every_ptr(connector, allConnectorInstances)
	{
		// if it has owner, it had to be accessed (see above)
		// so we only have to check case when it doesn't have owner, if there is owner on the other end
		if (!connector->get_owner() && !connector->get_connector_on_other_end() && !connector->is_optional() && !connector->is_no_connector() && !connector->is_blocked())
		{
			return false;
		}
	}

	// complete!
	return true;
}

template <typename CustomType>
bool Generator<CustomType>::are_both_in_same_cycle(PieceInstance<CustomType>* _pieceInstanceA, PieceInstance<CustomType>* _pieceInstanceB)
{
	// clear cycle info
	for_every_ptr(piece, allPieceInstances)
	{
		piece->clear_cycle();
	}

	if (_pieceInstanceA)
	{
		BuildCycleContext<CustomType> context;
		context.generator = this;
		context.cycleId = 1;
		_pieceInstanceA->build_cycle(context);
	}
	if (_pieceInstanceB)
	{
		return _pieceInstanceB->get_cycle_id() == 1;
	}
	else
	{
		return false;
	}
}

template <typename CustomType>
bool Generator<CustomType>::are_both_in_same_cycle(ConnectorInstance<CustomType>* _connectorInstanceA, ConnectorInstance<CustomType>* _connectorInstanceB)
{
	// find closest piece - it can be owner or (mostly for provided ones) first connected to this one
	auto * pieceInstanceA = _connectorInstanceA->get_owner();
	if (!pieceInstanceA)
	{
		pieceInstanceA = _connectorInstanceA->get_owner_on_other_end();
	}
	auto* pieceInstanceB = _connectorInstanceA->get_owner();
	if (!pieceInstanceB)
	{
		pieceInstanceB = _connectorInstanceA->get_owner_on_other_end();
	}

	if (!pieceInstanceA || !pieceInstanceB)
	{
		// connector doesn't have any piece, it has to be free!
		return false;
	}

	return are_both_in_same_cycle(pieceInstanceA, pieceInstanceB);
}

template <typename CustomType>
void Generator<CustomType>::calculate_cycle_info(PieceInstance<CustomType>* _pieceInstance, OUT_ int & _cycleSize, OUT_ int & _outerConnectorCountInCycle, OUT_ bool & _canCreateAnything)
{
	// clear cycle info
	for_every_ptr(piece, allPieceInstances)
	{
		piece->clear_cycle();
	}
	
	int cycleSize = 0;
	int outerConnectorCountInCycle = 0;
	
	if (_pieceInstance)
	{
		BuildCycleContext<CustomType> context;
		context.generator = this;
		context.cycleId = 1;
		_pieceInstance->build_cycle(context, &cycleSize, &outerConnectorCountInCycle, &_canCreateAnything);
	}
	
	_cycleSize = cycleSize;
	_outerConnectorCountInCycle = outerConnectorCountInCycle;
}

template <typename CustomType>
bool Generator<CustomType>::check_if_limits_are_broken() const
{
	int pieceLimit = get_generation_rules().get_piece_limit();
	if (pieceLimit > 0 && allPieceInstances.get_size() > pieceLimit)
	{
		return true;
	}

	// everything is fine
	return false;
}

template <typename CustomType>
bool Generator<CustomType>::check_if_one_outer_connector_per_piece_for_piece_connections_is_compromised() const
{
	// go through all pieces
	// for each piece (that has onlyOneConnectorToOuter) check all connectors
	// if there are two connectors that lead to outer, uniqueness is compromised
	for_every_ptr(piece, allPieceInstances)
	{
		if (piece->get_piece()->onlyOneConnectorToOuter.get(get_generation_rules().get_number_of_pieces_required_in_cycle_to_allow_connection_to_other_cycle() > 0 || get_generation_rules().should_allow_only_one_outer_connector_per_piece()))
		{
			int connectedToOuter = 0;
			for_every_ptr(connector, piece->get_owned_connectors())
			{
				if (connector->is_connected() &&
					!connector->is_internal_external() &&
					connector->get_connector()->type == ConnectorType::Normal)
				{
					if (auto* otherConnector = connector->get_connector_on_other_end())
					{
						if (otherConnector->is_outer() && // for outers we do not need to check type
							!otherConnector->is_internal_external())
						{
							++connectedToOuter;
							if (connectedToOuter > 1)
							{
#ifdef PIECE_COMBINER_VISUALISE
								piece->visualiseIssue = true;
								for_every_ptr(connector, piece->get_owned_connectors())
								{
									if (connector->is_connected() &&
										connector->get_connector()->type == ConnectorType::Normal)
									{
										if (auto* otherConnector = connector->get_connector_on_other_end())
										{
											if (otherConnector->is_outer())
											{
												connector->visualiseIssue = true;
											}
										}
									}
								}
#endif
								return true;
							}
						}
					}
				}
			}
		}
	}
	// everything ok
	return false;
}

template <typename CustomType>
bool Generator<CustomType>::check_if_uniqueness_for_piece_connections_is_compromised() const
{
	// go through all pieces
	// for each piece (that has eachConnectorHasUniquePiece or eachConnectorHasSamePiece set) check all connectors
	// if there are two connectors that lead to same piece, uniqueness is compromised
	for_every_ptr(piece, allPieceInstances)
	{
		for_every_ptr(connectorA, piece->get_owned_connectors())
		{
			if (connectorA->is_connected() &&
				connectorA->get_connector()->type == ConnectorType::Normal)
			{
				bool piece_eachConnectorHasUniquePiece = piece->get_piece()->eachConnectorHasUniquePiece;
				bool connector_eachConnectorHasUniquePiece = connectorA->get_connector()->eachConnectorHasUniquePiece;
				if (get_outer_connector_count() == 1)
				{
					if (piece->get_piece()->eachConnectorMayHaveNonUniquePieceForSingleOuterConnector)
					{
						piece_eachConnectorHasUniquePiece = false;
					}
					if (connectorA->get_connector()->eachConnectorMayHaveNonUniquePieceForSingleOuterConnector)
					{
						connector_eachConnectorHasUniquePiece = false;
					}
				}					
				if (piece_eachConnectorHasUniquePiece ||
					connector_eachConnectorHasUniquePiece)
				{
					if (PieceInstance<CustomType>* pieceOnAsEnd = connectorA->get_owner_on_other_end())
					{
						for_every_ptr_continue_after(connectorB, connectorA)
						{
							if (connectorB->is_connected() &&
								connectorB->get_connector()->type == ConnectorType::Normal)
							{
								if (pieceOnAsEnd == connectorB->get_owner_on_other_end())
								{
#ifdef PIECE_COMBINER_VISUALISE
									connectorB->visualiseIssue = true;
									connectorA->visualiseIssue = true;
									piece->visualiseIssue = true;
									pieceOnAsEnd->visualiseIssue = true;
#endif
									return true;
								}
							}
						}
					}
				}
				if (piece->get_piece()->eachConnectorHasSamePiece)
				{
					if (PieceInstance<CustomType>* pieceOnAsEnd = connectorA->get_owner_on_other_end())
					{
						for_every_ptr_continue_after(connectorB, connectorA)
						{
							if (connectorB->is_connected() &&
								connectorB->get_connector()->type == ConnectorType::Normal)
							{
								if (pieceOnAsEnd != connectorB->get_owner_on_other_end())
								{
#ifdef PIECE_COMBINER_VISUALISE
									connectorB->visualiseIssue = true;
									connectorA->visualiseIssue = true;
									piece->visualiseIssue = true;
									pieceOnAsEnd->visualiseIssue = true;
#endif
									return true;
								}
							}
						}
					}
				}
			}
		}
	}
	// everything ok
	return false;
}

template <typename CustomType>
bool Generator<CustomType>::check_if_connector_groups_are_compromised() const
{
	// go through all pieces
	// for each piece check connectors
	// if there are two connectors that have the same samePieceConnectorGroup (non empty)
	// check if they both connect to same piece - if they do, it's ok
	for_every_ptr(piece, allPieceInstances)
	{
		for_every_ptr(connectorA, piece->get_owned_connectors())
		{
			if (connectorA->is_connected() &&
				connectorA->get_connector()->type == ConnectorType::Normal)
			{
				if (connectorA->get_connector()->samePieceConnectorGroup.is_valid())
				{
					if (PieceInstance<CustomType>* pieceOnAsEnd = connectorA->get_owner_on_other_end())
					{
						for_every_ptr_continue_after(connectorB, connectorA)
						{
							if (connectorA->get_connector()->samePieceConnectorGroup == connectorB->get_connector()->samePieceConnectorGroup &&
								connectorB->is_connected() &&
								connectorB->get_connector()->type == ConnectorType::Normal)
							{
								if (pieceOnAsEnd != connectorB->get_owner_on_other_end())
								{
									return true;
								}
							}
						}
					}
				}
			}
		}
	}
	// everything ok
	return false;
}

template <typename CustomType>
bool Generator<CustomType>::check_if_there_are_impossible_generation_spaces() const
{
	if (generationSpace->check_if_impossible())
	{
		return true;
	}
	for_every_ptr(piece, allPieceInstances)
	{
		if (piece->get_generation_space()->check_if_impossible())
		{
#ifdef PIECE_COMBINER_VISUALISE
			piece->visualiseIssue = true;
#endif
			return true;
		}
	}
	return false;
}

template <typename CustomType>
bool Generator<CustomType>::check_if_there_are_separate_closed_cycles(bool _ignoreOptionals)
{
	if (allPieceInstances.get_size() == 0)
	{
		return false;
	}

	// clear cycle info
	for_every_ptr(piece, allPieceInstances)
	{
		piece->clear_cycle();
	}

	int cycleId = 0;
	int closedCycleCount = 0;
	while (true)
	{
		PieceInstance<CustomType>* startingPiece = nullptr;
		for_every_ptr(piece, allPieceInstances)
		{
			if (!piece->is_part_of_cycle())
			{
				bool allowAsStartingPiece = get_generation_rules().should_treat_parents_and_children_as_parts_of_cycle();
				if (!allowAsStartingPiece)
				{
					// check if this is piece that has internal or child connectors, if so, skip it as we may have better one to start with (that may actually build a cycle)
					allowAsStartingPiece = true;
					for_every_ptr(connector, piece->ownedConnectors)
					{
						if ((connector->get_connector()->type == ConnectorType::Internal && connector->get_connector()->internalExternalName.is_valid()) ||
							connector->get_connector()->type == ConnectorType::Child)
						{
							allowAsStartingPiece = false;
							break;
						}
					}
				}
				if (allowAsStartingPiece)
				{
					startingPiece = piece;
					break;
				}
			}
		}

		if (!startingPiece)
		{
			// all pieces are in cycles
			break;
		}

		++cycleId;

		// build cycle
		BuildCycleContext<CustomType> context;
		context.generator = this;
		context.cycleId = cycleId;
		context.ignoreOptionals = _ignoreOptionals;
		if (!startingPiece->build_cycle(context))
		{
			// we have closed cycle
			++closedCycleCount;
		}
	}

	return closedCycleCount > 0 && cycleId > 1; // if there is just one cycle and it is closed - that's fine, otherwise just quit
}

template <typename CustomType>
bool Generator<CustomType>::check_if_there_is_fail_connection()
{
	for_every_ptr(connector, allConnectorInstances)
	{
		if (connector->is_connected() &&
			connector->get_connector()->failIfConnected)
		{
			// well, we should be not connected
			connector->shouldNotExist = true;
			return true;
		}
		if (connector->shouldNotExist)
		{
			// should not exist at all - will be responsible for removal of piece containing this connector instance
			return true;
		}
	}
	// everything alright
	return false;
}

template <typename CustomType>
void Generator<CustomType>::remove_all_not_needed_connectors()
{
	ARRAY_PREALLOC_SIZE(ConnectorInstance<CustomType>*, connectorsToCheck, allConnectorInstances.get_size());
	connectorsToCheck.copy_from(allConnectorInstances);
	for_every_ptr(connector, connectorsToCheck)
	{
		if (connector->get_owner() &&
			! connector->is_connected() &&
			(connector->is_optional() || connector->is_blocked()))
		{
			// expectation: we're done with generation at this point
			if (connector->internalExternalConnector)
			{
				connector->step__disconnect_internal_external(this);
			}
			connector->get_owner()->step__remove_connector(this, connector);
			connector->get_in_generation_space()->step__remove_connector(this, connector);
			step__remove_connector_instance_from_all(connector);
		}
	}
}

template <typename CustomType>
void Generator<CustomType>::define_side_for_all_connectors()
{
	for_every_ptr(connector, allConnectorInstances)
	{
		if (!connector->is_internal_external())
		{
			connector->define_side();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
// GENERATOR :: OUTER CONNECTOR
template <typename CustomType>
Generator<CustomType>::OuterConnector::OuterConnector(Connector<CustomType> const* _connector, ConnectorInstance<CustomType> const* _outerConnectorInstance, typename CustomType::ConnectorInstanceData const& _data, ConnectorSide::Type _side)
: connector(_connector)
, side(_side)
, data(_data)
{
	if (_connector->inheritConnectorTag)
	{
		if (_outerConnectorInstance)
		{
			inheritedConnectorTag = _outerConnectorInstance->connectorTag;
			inheritedProvideConnectorTag = _outerConnectorInstance->provideConnectorTag;
		}
		else
		{
			inheritedConnectorTag = Name::invalid();
			inheritedProvideConnectorTag.clear();
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////
// GENERATION RULES

template <typename CustomType>
GenerationRules<CustomType>::GenerationRules()
: chanceToUseOptionalConnectorAsNormal(0.0f)
, numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle(0)
, onlyOneConnectorToOuter(false)
, allowConnectingProvidedPieces(true)
, pieceLimit(0)
, treatParentsAndChildrenAsPartsOfCycle(false)
{
}

template <typename CustomType>
void GenerationRules<CustomType>::instantiate(Optional<GenerationRules> _source, Random::Generator& _generator)
{
	if (!_source.is_set())
	{
		_source = GenerationRules();
	}

	chanceToUseOptionalConnectorAsNormal = Random::Number<float>(_source.get().chanceToUseOptionalConnectorAsNormal.get(_generator));
	numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle = Random::Number<int>(_source.get().numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle.get(_generator));
	onlyOneConnectorToOuter = _source.get().onlyOneConnectorToOuter;
	allowConnectingProvidedPieces = _source.get().allowConnectingProvidedPieces;
	pieceLimit = Random::Number<int>(_source.get().pieceLimit.get(_generator));
	treatParentsAndChildrenAsPartsOfCycle = _source.get().treatParentsAndChildrenAsPartsOfCycle;
}

template <typename CustomType>
bool GenerationRules<CustomType>::load_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context)
{
	bool result = true;
	result &= chanceToUseOptionalConnectorAsNormal.load_from_xml(_node, TXT("chanceToUseOptionalConnectorAsNormal"));
	result &= numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle.load_from_xml(_node, TXT("numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle"));
	onlyOneConnectorToOuter = _node->get_bool_attribute(TXT("onlyOneConnectorToOuter"), onlyOneConnectorToOuter);
	allowConnectingProvidedPieces = _node->get_bool_attribute(TXT("allowConnectingProvidedPieces"), allowConnectingProvidedPieces);
	result &= pieceLimit.load_from_xml(_node, TXT("pieceLimit"));
	treatParentsAndChildrenAsPartsOfCycle = _node->get_bool_attribute(TXT("treatParentsAndChildrenAsPartsOfCycle"), treatParentsAndChildrenAsPartsOfCycle);
	return result;
}

///////////////////////////////////////////////////////////////////////////////////
// GENERATION SPACE

template <typename CustomType>
GenerationSpace<CustomType>::GenerationSpace()
: owner(nullptr)
{
}

template <typename CustomType>
void GenerationSpace<CustomType>::on_get()
{
	owner = nullptr;
	clear();
}

template <typename CustomType>
void GenerationSpace<CustomType>::on_release()
{
	owner = nullptr;
	clear();
}

template <typename CustomType>
void GenerationSpace<CustomType>::clear()
{
	for_every_ptr(connector, connectors)
	{
		connector->on_generation_space_clear(this);
		connector->release_ref();
	}
	for_every_ptr(piece, pieces)
	{
		piece->on_generation_space_clear(this);
		piece->release_ref();
	}
	connectors.clear();
	pieces.clear();
}

template <typename CustomType>
void GenerationSpace<CustomType>::set_owner(PieceInstance<CustomType>* _owner)
{
	owner = _owner;
}

template <typename CustomType>
void GenerationSpace<CustomType>::step__add_piece(Generator<CustomType>* _generator, PieceInstance<CustomType>* _pieceInstance)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(!pieces.does_contain(_pieceInstance));
	an_assert(_pieceInstance->inGenerationSpace == nullptr);
	_pieceInstance->add_ref();
	_pieceInstance->inGenerationSpace = this;
	memory_leak_suspect;
	pieces.push_back(_pieceInstance);
	forget_memory_leak_suspect;
}

template <typename CustomType>
void GenerationSpace<CustomType>::step__remove_piece(Generator<CustomType>* _generator, PieceInstance<CustomType>* _pieceInstance)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(_pieceInstance && _pieceInstance->inGenerationSpace == this);
	an_assert(pieces.does_contain(_pieceInstance));
	_pieceInstance->inGenerationSpace = nullptr;
	pieces.remove(_pieceInstance);
	_pieceInstance->release_ref();
}

template <typename CustomType>
void GenerationSpace<CustomType>::step__add_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(!connectors.does_contain(_connectorInstance));
	an_assert(_connectorInstance->inGenerationSpace == nullptr);
	_connectorInstance->add_ref();
	_connectorInstance->inGenerationSpace = this;
	memory_leak_suspect;
	connectors.push_back(_connectorInstance);
	forget_memory_leak_suspect;
}

template <typename CustomType>
void GenerationSpace<CustomType>::step__remove_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(_connectorInstance && _connectorInstance->inGenerationSpace == this);
	an_assert(connectors.does_contain(_connectorInstance));
	_connectorInstance->inGenerationSpace = nullptr;
	connectors.remove(_connectorInstance);
	_connectorInstance->release_ref();
}


template <typename CustomType>
void GenerationSpace<CustomType>::step__block_non_owned_with(Generator<CustomType>* _generator, Connector<CustomType> const* _connector)
{
	an_assert(_generator->is_valid_step_call());
	for_every(blocks, _connector->blocks)
	{
		for_every_ptr(connector, connectors)
		{
			if (! connector->owner &&
				connector->get_connector()->name == *blocks)
			{
				connector->step__mark_blocked(_generator);
			}
		}
	}
	if (! _connector->autoBlocks.is_empty())
	{
		warn(TXT("no support for auto blocks"));
	}
}

template <typename CustomType>
void GenerationSpace<CustomType>::step__unblock_non_owned_with(Generator<CustomType>* _generator, Connector<CustomType> const* _connector)
{
	an_assert(_generator->is_valid_step_call());
	for_every(blocks, _connector->blocks)
	{
		for_every_ptr(connector, connectors)
		{
			if (!connector->owner &&
				connector->get_connector()->name == *blocks)
			{
				connector->step__mark_unblocked(_generator);
			}
		}
	}
	if (!_connector->autoBlocks.is_empty())
	{
		warn(TXT("no support for auto blocks"));
	}
}

template <typename CustomType>
bool GenerationSpace<CustomType>::check_if_impossible() const
{
	if (connectors.get_size() == 0)
	{
		// no connectors are fine - this happens for most of empty pieces
		return false; // possible
	}
	
	// look for connectors, check if there are only connectors that can't add piece and they can't connect to each other
	// it won't get all impossibilities but still may help a little
	// if at least one of connectors can create new pieces, anything is possible
	ARRAY_PREALLOC_SIZE(ConnectorInstance<CustomType>*, availableConnectorsToCheck, connectors.get_size());
	ARRAY_PREALLOC_SIZE(ConnectorInstance<CustomType>*, availableConnectorsAll, connectors.get_size());
	for_every_ptr(connector, connectors)
	{
		if (connector->is_available())
		{
			memory_leak_suspect;
			availableConnectorsAll.push_back(connector);
			forget_memory_leak_suspect;
			if (!connector->is_optional())
			{
				if (connector->get_connector()->canCreateNewPieces)
				{
					// it can create anything and anything is possible because of that
					return false; // possible
				}
				memory_leak_suspect;
				availableConnectorsToCheck.push_back(connector);
				forget_memory_leak_suspect;
			}
			else // is optional
			{
				// can create new pieces and is internal/external
				// if so, check if other one can create new pieces or if it can be connected somewhere else
				// it's either "both can create stuff" or can be connected
				if (connector->get_connector()->canCreateNewPieces)
				{
					if (auto* ieConnector = connector->get_internal_external())
					{
						if (ieConnector->is_connected())
						{
							// we may create stuff!
							return false; // possible
						}
						if (ieConnector->get_connector()->canCreateNewPieces)
						{
							// one of them will create something
							return false; // possible
						}
						else if (auto* gs = ieConnector->get_in_generation_space())
						{
							ARRAY_PREALLOC_SIZE(ConnectorInstance<CustomType>*, availableConnectorsAllOtherGS, gs->connectors.get_size());
							for_every_ptr(oGSConnector, gs->connectors)
							{
								if (oGSConnector->is_available())
								{
									availableConnectorsAllOtherGS.push_back(oGSConnector);
								}
							}
							bool canConnect = false;
							for_every_const_ptr(checkConnector, availableConnectorsAllOtherGS)
							{
								if (checkConnector != ieConnector)
								{
									if (ieConnector->can_connect_with(checkConnector))
									{
										canConnect = true;
										break;
									}
								}
							}
							if (canConnect)
							{
								// other connector may still connect, which means that we should be able to create new pieces
								return false; // possible
							}
						}
					}
				}
			}
		}
	}
	
	// now check if at least one of them doesn't match any other - that one will render everything here impossible
	for(int ic = 0; ic < availableConnectorsToCheck.get_size(); ++ ic)
	{
		auto connector = availableConnectorsToCheck[ic];
		bool canConnect = false;
		for_every_const_ptr(checkConnector, availableConnectorsAll)
		{
			if (checkConnector != connector)
			{
				if (connector->can_connect_with(checkConnector))
				{
					canConnect = true;
					break;
				}
			}
		}	
		if ( ! canConnect)
		{
			// check if can be blocked by something else, then we do not have to worry
			bool canBeBlockedBySomethingElse = false;
			for_every_ptr(other, availableConnectorsToCheck)
			{
				if (ic != for_everys_index(other) &&
					(other->get_connector()->blocks.does_contain(connector->get_connector()->name) ||
					 other->get_connector()->autoBlocks.does_contain(connector->get_connector()->autoId)))
				{
					canBeBlockedBySomethingElse = true;
					break;
				}
			}
			if (canBeBlockedBySomethingElse)
			{
				availableConnectorsToCheck.remove_at(ic);
				--ic;
				continue;
			}
			bool stillImpossible = true;
			// check its piece and find out if there is parent connector that is not connected
			// if there is, maybe we first need to find a parent for that guy that will bring him to proper space?
			if (PieceInstance<CustomType> const * piece = connector->get_owner())
			{
				for_every_const_ptr(pieceConnector, piece->get_owned_connectors())
				{
					if (pieceConnector->is_available() && pieceConnector->get_connector()->type == ConnectorType::Parent)
					{
						// owner of this connector has parent connector that is not connected, we might move it from this generation space
						stillImpossible = false;
						break;
					}
				}
			}
			if (stillImpossible)
			{
#ifdef LOG_PIECE_COMBINER_DETAILS
				log_to_file(TXT("impossible piece %S connector %S/%S -> %S/%S"),
					connector->get_owner() ? connector->get_owner()->get_piece()->def.get_desc().to_char() : TXT(""),
					connector->get_connector()->linkName.to_char(),
					connector->get_connector()->linkNameAlt.to_char(),
					connector->get_connector()->linkWith.to_char(),
					connector->get_connector()->linkWithAlt.to_char()
					);
#endif
				// can't connect to any other available and since we can't create anything new
				// and we won't be moving owner piece to other generation space, drop this idea
#ifdef PIECE_COMBINER_VISUALISE
				connector->visualiseIssue = true;
#endif
				return true; // impossible
			}
		}
	}
	
	// it is still possible
	return false; // possible
}

///////////////////////////////////////////////////////////////////////////////////
// GENERATION STEP

template <typename CustomType>
REGISTER_FOR_FAST_CAST(GenerationStep<CustomType>);

template <typename CustomType>
GenerationStep<CustomType>::GenerationStep()
{
}

///////////////////////////////////////////////////////////////////////////////////
// USE PIECE

template <typename CustomType>
UsePiece<CustomType>::UsePiece()
: piece(nullptr)
{
}

template <typename CustomType>
UsePiece<CustomType>::UsePiece(Piece<CustomType> const * _piece)
: piece(_piece)
{
}

template <typename CustomType>
void UsePiece<CustomType>::instantiate(Random::Generator& _generator)
{
	if (limitCount)
	{
		if (limitCount->is_empty())
		{
			actualLimitCount = -1;
		}
		else
		{
			actualLimitCount = limitCount->get(_generator);
		}
	}
	else
	{
		if (piece->limitCount.is_empty())
		{
			actualLimitCount = -1;
		}
		else
		{
			actualLimitCount = piece->limitCount.get(_generator);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
// PIECE

template <typename CustomType>
Piece<CustomType>::Piece()
: probabilityCoef(1.0f)
, eachConnectorHasUniquePiece(false)
, eachConnectorMayHaveNonUniquePieceForSingleOuterConnector(false)
, eachConnectorHasSamePiece(false)
, preferCreatingNewPieces(false)
, preferEachConnectorHasUniquePiece(false)
, disconnectIrrelevantConnectorsPostGeneration(false)
{
}

template <typename CustomType>
bool Piece<CustomType>::can_connect_with(Connector<CustomType> const* _with) const
{
	for_every_const(connector, connectors)
	{
		if (connector->can_connect_with(_with))
		{
			return true;
		}
	}
	return false;
}

template <typename CustomType>
bool Piece<CustomType>::load_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context)
{
	bool result = true;

	if (!_node)
	{
		return false;
	}

	result &= def.load_from_xml(_node, _context);

	probabilityCoef = _node->get_float_attribute(TXT("probCoef"), probabilityCoef);
	probabilityCoef = _node->get_float_attribute(TXT("probabilityCoef"), probabilityCoef);

	result &= limitCount.load_from_xml(_node, TXT("limit"));

	onlyOneConnectorToOuter.load_from_xml(_node, TXT("onlyOneConnectorToOuter"));
	eachConnectorHasUniquePiece = _node->get_bool_attribute(TXT("eachConnectorHasUniquePiece"), eachConnectorHasUniquePiece);
	eachConnectorMayHaveNonUniquePieceForSingleOuterConnector = _node->get_bool_attribute(TXT("eachConnectorMayHaveNonUniquePieceForSingleOuterConnector"), eachConnectorMayHaveNonUniquePieceForSingleOuterConnector);
	eachConnectorHasSamePiece = _node->get_bool_attribute(TXT("eachConnectorHasSamePiece"), eachConnectorHasSamePiece);
	preferCreatingNewPieces = _node->get_bool_attribute(TXT("preferCreatingNewPieces"), preferCreatingNewPieces);
	preferEachConnectorHasUniquePiece = _node->get_bool_attribute(TXT("preferEachConnectorHasUniquePiece"), preferEachConnectorHasUniquePiece);
	disconnectIrrelevantConnectorsPostGeneration = _node->get_bool_attribute(TXT("disconnectIrrelevantConnectorsPostGeneration"), disconnectIrrelevantConnectorsPostGeneration);
	
	for_every(childNode, _node->all_children())
	{
		result &= load_something_from_xml(childNode, _context);
	}
	
	for_every(mapNode, _node->children_named(TXT("connect")))
	{
		Name internalName = mapNode->get_name_attribute(TXT("internal"));
		Name externalName = mapNode->get_name_attribute(TXT("external"));
		Connector<CustomType>* internalConnector = nullptr;
		Connector<CustomType>* externalConnector = nullptr;
		for_every(connector, connectors)
		{
			if (connector->name == internalName)
			{
				internalConnector = connector;
			}
			if (connector->name == externalName)
			{
				externalConnector = connector;
			}
			if (internalConnector && externalConnector)
			{
				break;
			}
		}
		if (internalConnector && externalConnector)
		{
			internalConnector->internalExternalName = externalName;
			externalConnector->internalExternalName = internalName;
		}
		else
		{
			error(TXT("could not find %S %S pair in %S"), internalName.to_char(), externalName.to_char());
			result = false;
		}
	}

	return result;
}

template <typename CustomType>
bool Piece<CustomType>::load_plug_connectors_from_xml(IO::XML::Node const * _node, tchar const * _plugAttrName, tchar const * _plugNodeName, OUT_ bool * _loadedAnything, std::function<bool(IO::XML::Node const * _node, Connector<CustomType> & _connector)> _load_custom)
{
	if (!_node)
	{
		return true;
	}

	assign_optional_out_param(_loadedAnything, false);
	
	// there can be only one plug active, collect all of them, add them and have make_connectors_block_each_other called on all indices
	bool result = true;
	struct LoadedConnector
	{
		Name connector;
		float chance = 1.0f;
		IO::XML::Node const * node = nullptr;
	};
	Array<LoadedConnector> paramConnectors;
	if (_plugAttrName && _node->has_attribute(_plugAttrName))
	{
		String connectorAttr = _node->get_string_attribute(_plugAttrName);
		List<String> connectors;
		connectorAttr.split(String::comma(), connectors);
		for_every(connector, connectors)
		{
			Name paramConnector = Name(connector->trim());
			if (paramConnector.is_valid())
			{
				LoadedConnector lc;
				lc.connector = paramConnector;
				lc.node = _node;
				paramConnectors.push_back(lc);
			}
			else
			{
				error_loading_xml(_node, TXT("invalid connector"));
				result = false;
			}
		}
	}
	if (_plugNodeName)
	{
		for_every(node, _node->children_named(_plugNodeName))
		{
			Name paramConnector = node->get_name_attribute(TXT("connector"));
			if (paramConnector.is_valid())
			{
				LoadedConnector lc;
				lc.connector = paramConnector;
				lc.chance = node->get_float_attribute(TXT("chance"), lc.chance);
				lc.node = node;
				paramConnectors.push_back(lc);
			}
			else
			{
				error_loading_xml(node, TXT("missing connector for param"));
				result = false;
			}
		}
	}
	if (!paramConnectors.is_empty())
	{
		Array<int> connectorIndices;
		for_every(paramConnector, paramConnectors)
		{	// we will connect to parent through this
			PieceCombiner::Connector<CustomType> connector;
			connector.be_normal_connector();
			connector.make_required_single_only();
			connector.make_can_create_new_pieces(false);
			connector.set_link_ps_as_plug(paramConnector->connector.to_char());
			connector.set_choose_chance(paramConnector->chance);
			if (_load_custom)
			{
				result &= _load_custom(paramConnector->node, connector);
			}
			connectorIndices.push_back(this->add_connector(connector));
		}
		this->make_connectors_block_each_other(connectorIndices);
		assign_optional_out_param(_loadedAnything, true);
	}

	return result;
}

template <typename CustomType>
bool Piece<CustomType>::load_socket_connectors_from_xml(IO::XML::Node const * _node, tchar const * _socketNodeName, OUT_ bool * _loadedAnything, std::function<bool(IO::XML::Node const * _node, Connector<CustomType> & _connector)> _load_custom)
{
	if (!_node)
	{
		return true;
	}

	// loaded from socketAttrName
	assign_optional_out_param(_loadedAnything, false);

	bool result = true;
	struct LoadedConnector
	{
		Name connector;
		float chance = 1.0f;
		IO::XML::Node const * node = nullptr;
	};

	if (_socketNodeName)
	{
		// make them block each other per socket node
		for_every(node, _node->children_named(_socketNodeName))
		{
			Array<LoadedConnector> paramConnectors;
			String connectorAttr = node->get_string_attribute(TXT("connector"));
			List<String> connectors;
			connectorAttr.split(String::comma(), connectors);
			for_every(connector, connectors)
			{
				if (!connector->is_empty())
				{
					Name paramConnector = Name(connector->trim());
					if (paramConnector.is_valid())
					{
						LoadedConnector lc;
						lc.connector = paramConnector;
						lc.chance = node->get_float_attribute(TXT("chance"), lc.chance);
						lc.node = node;
						paramConnectors.push_back(lc);
					}
					else
					{
						error_loading_xml(node, TXT("missing connector for param"));
						result = false;
					}
				}
			}
			for_every(useNode, node->children_named(TXT("use")))
			{
				Name paramConnector = useNode->get_name_attribute(TXT("connector"));
				if (paramConnector.is_valid())
				{
					LoadedConnector lc;
					lc.connector = paramConnector;
					lc.chance = useNode->get_float_attribute(TXT("chance"), lc.chance);
					lc.node = node; //not useNode!
					paramConnectors.push_back(lc);
				}
				else
				{
					error_loading_xml(useNode, TXT("missing connector for param"));
					result = false;
				}
			}
			if (!paramConnectors.is_empty())
			{
				Array<int> connectorIndices;
				for_every(paramConnector, paramConnectors)
				{	// we will create children through this
					PieceCombiner::Connector<CustomType> connector;
					connector.be_normal_connector();
					connector.make_required_single_only();
					connector.make_can_create_new_pieces(true);
					connector.set_link_ps_as_socket(paramConnector->connector.to_char());
					connector.set_probability_of_existence(paramConnector->chance);
					connector.allow_fail(paramConnector->chance < 1.0f);
					if (_load_custom)
					{
						result &= _load_custom(paramConnector->node, connector);
					}
					connectorIndices.push_back(this->add_connector(connector));
				}
				this->make_connectors_block_each_other(connectorIndices);
				assign_optional_out_param(_loadedAnything, true);
			}
		}
	}

	return result;
}

template <typename CustomType>
bool Piece<CustomType>::load_something_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context)
{
	if (_node->get_name() == TXT("oneOf"))
	{
		return load_one_of_from_xml(_node, _context);
	}
	else if (_node->get_name() == TXT("grouped"))
	{
		return load_grouped_connectors_from_xml(_node, _context);
	}
	else
	{
		return load_connector_from_xml(_node, _context);
	}
}

template <typename CustomType>
int Piece<CustomType>::get_amount_of_randomisation_tries() const
{
	int options = 0;
	for_every(c, connectors)
	{
		options += max(0, c->requiredCount.estimate_options_count() - 1);
		options += max(0, c->optionalCount.estimate_options_count() - 1);
		options += max(0, c->totalCount.estimate_options_count() - 1);
	}

	return clamp(1 + (options > 1? max(2, (options * 3) / 2) : 0), 1, 6);
}

template <typename CustomType>
void Piece<CustomType>::make_connectors_block_each_other(Array<int> const & _indices)
{
	for_every(a, _indices)
	{
		Array<int> & autoBlocks = connectors[*a].autoBlocks;
		for_every(b, _indices)
		{
			if (*a != *b)
			{
				validate_auto_id_for(connectors[*b]);
				memory_leak_suspect;
				autoBlocks.push_back_unique(connectors[*b].autoId);
				forget_memory_leak_suspect;
			}
		}
	}
}

template <typename CustomType>
bool Piece<CustomType>::load_one_of_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context)
{
	// we work with starting indices as we may have nested connectors but we have flat array of connectors
	bool result = true;
	Array<int> startingIndices;
	memory_leak_suspect;
	startingIndices.push_back(connectors.get_size());
	forget_memory_leak_suspect;
	for_every(childNode, _node->all_children())
	{
		result &= load_something_from_xml(childNode, _context);
		memory_leak_suspect;
		startingIndices.push_back_unique(connectors.get_size());
		forget_memory_leak_suspect;
	}
	if (startingIndices.get_size() > 1)
	{
		// for all groups (group starts at startingIndices[i] and ends before startingIndices[i+1])
		// mark that each blocks all from other groups
		for (int i = 0; i < startingIndices.get_size() - 1; ++ i)
		{
			for (int j = 0; j < startingIndices.get_size() - 1; ++j)
			{
				if (i != j)
				{
					for (int in = startingIndices[i], iEnd = startingIndices[i + 1]; in < iEnd; ++in)
					{
						Array<int> & autoBlocks = connectors[in].autoBlocks;
						for (int jn = startingIndices[j], jEnd = startingIndices[j + 1]; jn < jEnd; ++jn)
						{
							validate_auto_id_for(connectors[jn]);
							memory_leak_suspect;
							autoBlocks.push_back_unique(connectors[jn].autoId);
							forget_memory_leak_suspect;
						}
					}
				}
			}
		}
	}
	else
	{
		error_loading_xml(_node, TXT("just one \"oneOf\" node? in %S"), _node->get_location_info().to_char());
	}
	return result;
}

template <typename CustomType>
bool Piece<CustomType>::load_grouped_connectors_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context)
{
	bool result = true;
	for_every(childNode, _node->all_children())
	{
		result &= load_something_from_xml(childNode, _context);
	}
	return result;
}

template <typename CustomType>
void Piece<CustomType>::validate_auto_id_for(Connector<CustomType> & _connector)
{
	if (_connector.autoId == NONE)
	{
		int tryId = 0;
		bool ok = false;
		while (!ok)
		{
			ok = true;
			for_every(connector, connectors)
			{
				if (connector->autoId == tryId)
				{
					++tryId;
					ok = false;
				}
			}
		}
		_connector.autoId = tryId;
	}
}

template <typename CustomType>
bool Piece<CustomType>::load_connector_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context)
{
	ConnectorType::Type type = ConnectorType::parse(_node->get_name());
	if (type != ConnectorType::Unknown)
	{
		memory_leak_suspect;
		connectors.push_back(Connector<CustomType>(type));
		forget_memory_leak_suspect;
		bool connectorLoaded = connectors.get_last().load_from_xml(_node, _context);
		if (!connectorLoaded)
		{
			error_loading_xml(_node, TXT("connector %S/%S invalid!"), connectors.get_last().linkName.to_char(), connectors.get_last().linkNameAlt.to_char());
			connectors.pop_back();
			return false;
		}
		return true;
	}
	return true; // something else?
}

template <typename CustomType>
bool Piece<CustomType>::prepare(typename CustomType::PreparingContext * _context)
{
	bool result = true;
	result &= def.prepare(this, _context);
	for_every(connector, connectors)
	{
		result &= connector->prepare(this, _context);
		// check if internal has name and has internal/external info set
		if (connector->type == ConnectorType::Internal)
		{
			if (!connector->name.is_valid())
			{
				error(TXT("internal connector of %S requires name"), def.get_desc().to_char());
				result = false;
			}
			if (!connector->internalExternalName.is_valid())
			{
				error(TXT("internal connector %S of %S doesn't have external connector mapped"), connector->name.to_char(), def.get_desc().to_char());
				result = false;
			}
		}
		// check if internal-external pair matches
		if (connector->internalExternalName.is_valid())
		{
			for_every(otherConnector, connectors)
			{
				if (otherConnector != connector)
				{
					if (connector->internalExternalName == otherConnector->name)
					{
						if (otherConnector->internalExternalName != connector->name)
						{
							error(TXT("check %S and %S if both have internal/external setup to match each other"), connector->name.to_char(), connector->internalExternalName.to_char());
							result = false;
						}
					}
				}
			}
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////////
// CONNECTOR

template <typename CustomType>
Connector<CustomType>::Connector(ConnectorType::Type _type)
: type(_type)
{
}

template <typename CustomType>
bool Connector<CustomType>::can_connect_with(Connector<CustomType> const* _with) const
{
	// check if links match
	if ((linkWith.is_valid() || _with->linkWith.is_valid()) // at least one link-with is defined
	 &&	(! linkWith.is_valid() || (_with->linkName.is_valid() && (linkWith == _with->linkName || (linkWithAlt.is_valid() && linkWithAlt == _with->linkName)))
							   || (_with->linkNameAlt.is_valid() && (linkWith == _with->linkNameAlt || (linkWithAlt.is_valid() && linkWithAlt == _with->linkNameAlt)))) // "this" links with anything or matches "with"'s link
	 && (! _with->linkWith.is_valid() || (linkName.is_valid() && (_with->linkWith == linkName || (_with->linkWithAlt.is_valid() && _with->linkWithAlt == linkName)))
									  || (linkNameAlt.is_valid() && (_with->linkWith == linkNameAlt || (_with->linkWithAlt.is_valid() && _with->linkWithAlt == linkNameAlt))))) // "with" links with anything or matches "this"'s link
	{
		an_assert(linkName.is_valid() && _with->linkName.is_valid(), TXT("both links should be defined (\"%S\" v \"%S\", wasn't there error related to loading connectors?"), linkName.to_char(), _with->linkName.to_char());
		if ((type == ConnectorType::Parent && _with->type == ConnectorType::Child) ||
			(type == ConnectorType::Child && _with->type == ConnectorType::Parent))
		{
			// parent <> child doesn't require extra checks
			return true;
		}
		// check if definitions allow
		if (_with->def.can_join_to(def))
		{
			// check sides
			if ((_with->def.are_sides_symmetrical() && def.are_sides_symmetrical()) ||
				(ConnectorSide::can_connect(_with->side, side)))
			{
				return true;
			}
		}
	}
	return false;
}

template <typename CustomType>
bool Connector<CustomType>::check_required_parameters(Generator<CustomType>* _generator) const
{
	for_every(param, requiresParametersToBeTrue)
	{
		if (auto* paramProvider = _generator->get_parameters_provider())
		{
			bool paramValue;
			if (paramProvider->AN_CLANG_TEMPLATE restore_value<bool>(*param, paramValue))
			{
				if (!paramValue)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	for_every(param, requiresParametersToBeFalse)
	{
		if (auto* paramProvider = _generator->get_parameters_provider())
		{
			bool paramValue;
			if (paramProvider->AN_CLANG_TEMPLATE restore_value<bool>(*param, paramValue))
			{
				if (paramValue)
				{
					return false;
				}
			}
		}
	}
	return true;
}

template <typename CustomType>
bool Connector<CustomType>::load_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context)
{
	bool result = true;

	if (!_node)
	{
		return false;
	}

	result &= def.load_from_xml(_node, _context);

	result &= addRequired.load_from_xml(_node, TXT("addRequired"));
	result &= requiredCount.load_from_xml(_node, TXT("required"));
	result &= optionalCount.load_from_xml(_node, TXT("optional"));
	result &= totalCount.load_from_xml(_node, TXT("limit"));

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("socket")))
	{
		name = attr->get_as_name();
		set_link_ps_as_socket(attr->get_as_string().to_char());
		if (!addRequired.is_set() &&
			requiredCount.get_max() == 0 && optionalCount.get_max() == 0 && totalCount.get_max() == 0) // check if anything is defined
		{
			requiredCount = Random::Number<int>(1);
		}
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("socketDuplex")))
	{
		name = attr->get_as_name();
		set_link_ps_as_socket(attr->get_as_string().to_char(), true);
		if (!addRequired.is_set() &&
			requiredCount.get_max() == 0 && optionalCount.get_max() == 0 && totalCount.get_max() == 0) // check if anything is defined
		{
			requiredCount = Random::Number<int>(1);
		}
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("plug")))
	{
		name = attr->get_as_name();
		set_link_ps_as_plug(attr->get_as_string().to_char());
		if (!addRequired.is_set() &&
			requiredCount.get_max() == 0 && optionalCount.get_max() == 0 && totalCount.get_max() == 0) // check if anything is defined
		{
			requiredCount = Random::Number<int>(1);
		}
		canCreateNewPieces = false; // by default plugs can't create new pieces
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("plugDuplex")))
	{
		name = attr->get_as_name();
		set_link_ps_as_plug(attr->get_as_string().to_char(), true);
		if (!addRequired.is_set() &&
			requiredCount.get_max() == 0 && optionalCount.get_max() == 0 && totalCount.get_max() == 0) // check if anything is defined
		{
			requiredCount = Random::Number<int>(1);
		}
		canCreateNewPieces = false; // by default plugs can't create new pieces
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("duplex")))
	{
		name = attr->get_as_name();
		set_link_ps_as_duplex(attr->get_as_string().to_char());
		if (!addRequired.is_set() &&
			requiredCount.get_max() == 0 && optionalCount.get_max() == 0 && totalCount.get_max() == 0) // check if anything is defined
		{
			requiredCount = Random::Number<int>(1);
		}
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("provider")))
	{
		name = attr->get_as_name();
		set_link_ps_as_provider(attr->get_as_string().to_char());
		if (!addRequired.is_set() &&
			requiredCount.get_max() == 0 && optionalCount.get_max() == 0 && totalCount.get_max() == 0) // check if anything is defined
		{
			requiredCount = Random::Number<int>(0);
			optionalCount = Random::Number<int>(INF_INT);
		}
		canCreateNewPieces = false; // by default providers can't create new pieces
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("requester")))
	{
		name = attr->get_as_name();
		set_link_ps_as_requester(attr->get_as_string().to_char());
		if (!addRequired.is_set() &&
			requiredCount.get_max() == 0 && optionalCount.get_max() == 0 && totalCount.get_max() == 0) // check if anything is defined
		{
			requiredCount = Random::Number<int>(1);
		}
		canCreateNewPieces = false; // by default requesters can't create new pieces
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("refuse")))
	{
		name = attr->get_as_name();
		set_link_ps_as_refuse(attr->get_as_string().to_char());
		failIfConnected = true; // this will setup everything later!
	}

	if (IO::XML::Attribute const* attr = _node->get_attribute(TXT("outer")))
	{
		name = outerName = attr->get_as_name();
	}

	name = _node->get_name_attribute(TXT("name"), name);
	linkName = _node->get_name_attribute(TXT("linkName"), linkName);
	linkNameAlt = _node->get_name_attribute(TXT("linkNameAlt"), linkNameAlt);
	linkWith = _node->get_name_attribute(TXT("linkWith"), linkWith);
	linkWithAlt = _node->get_name_attribute(TXT("linkWithAlt"), linkWithAlt);
	if (!linkName.is_valid() && !name.is_valid())
	{
		error_loading_xml(_node, TXT("no \"linkName\" nor \"name\" provided! there has to be at least one of them"));
		result = false;
	}
	connectorTag = _node->get_name_attribute(TXT("connectorTag"), connectorTag);
	provideConnectorTag.load_from_xml(_node, TXT("provideConnectorTag"));
	inheritConnectorTag = _node->get_bool_attribute_or_from_child_presence(TXT("inheritConnectorTag"), inheritConnectorTag);
	inheritProvideConnectorTag = _node->get_bool_attribute_or_from_child_presence(TXT("inheritProvideConnectorTag"), inheritProvideConnectorTag);
	String const & sideAttr = _node->get_string_attribute(TXT("side"));
	side = sideAttr == TXT("b") || sideAttr == TXT("B") ? ConnectorSide::B : 
		  (sideAttr == TXT("a") || sideAttr == TXT("A") ? ConnectorSide::A : ConnectorSide::Any);
	canCreateNewPieces = _node->get_bool_attribute(TXT("canCreateNewPieces"), canCreateNewPieces);
	createNewPieceMark = _node->get_name_attribute(TXT("createNewPieceMark"), createNewPieceMark);
	allowFail = _node->get_bool_attribute(TXT("allowFail"), allowFail);
	forceCreateNewPieceProbability = _node->get_float_attribute(TXT("forceCreateNewPieceProbability"), forceCreateNewPieceProbability);
	forceCreateNewPieceLimit = _node->get_int_attribute(TXT("forceCreateNewPieceLimit"), forceCreateNewPieceLimit);
	priority = _node->get_float_attribute(TXT("priority"), priority);
	probabilityOfExistence = clamp(_node->get_float_attribute(TXT("probabilityOfExistence"), probabilityOfExistence), 0.0f, 1.0f);
	chooseChance = _node->get_float_attribute(TXT("chooseChance"), chooseChance);
	requiredAsOptionalProbability = clamp(_node->get_float_attribute(TXT("requiredAsOptionalProbability"), requiredAsOptionalProbability), 0.0f, 1.0f);
	requiresExternalConnected = _node->get_bool_attribute(TXT("requiresExternalConnected"), requiresExternalConnected);
	connectToOnlyIfNothingElse = _node->get_bool_attribute(TXT("connectToOnlyIfNothingElse"), connectToOnlyIfNothingElse);
	allowConnectingToSelf = _node->get_bool_attribute(TXT("allowConnectingToSelf"), allowConnectingToSelf);
	eachConnectorHasUniquePiece = _node->get_bool_attribute(TXT("eachConnectorHasUniquePiece"), eachConnectorHasUniquePiece);
	eachConnectorMayHaveNonUniquePieceForSingleOuterConnector = _node->get_bool_attribute(TXT("eachConnectorMayHaveNonUniquePieceForSingleOuterConnector"), eachConnectorMayHaveNonUniquePieceForSingleOuterConnector);
	samePieceConnectorGroup = _node->get_name_attribute(TXT("samePieceConnectorGroup"), samePieceConnectorGroup);
	preferCreatingNewPieces = _node->get_bool_attribute(TXT("preferCreatingNewPieces"), preferCreatingNewPieces);
	preferEachConnectorHasUniquePiece = _node->get_bool_attribute(TXT("preferEachConnectorHasUniquePiece"), preferEachConnectorHasUniquePiece);
	failIfConnected = _node->get_bool_attribute(TXT("failIfConnected"), failIfConnected);

	if (_node->has_attribute(TXT("requiresParameterToBeTrue")))
	{
		requiresParametersToBeTrue.push_back_unique(_node->get_name_attribute(TXT("requiresParameterToBeTrue")));
	}
	for_every(node, _node->children_named(TXT("requiresToBeTrue")))
	{
		if (node->has_attribute(TXT("parameter")))
		{
			requiresParametersToBeTrue.push_back_unique(node->get_name_attribute(TXT("parameter")));
		}
		else
		{
			error_loading_xml(node, TXT("no \"parameter\" provided"));
			result = false;
		}
	}
	if (_node->has_attribute(TXT("requiresParameterToBeFalse")))
	{
		requiresParametersToBeFalse.push_back_unique(_node->get_name_attribute(TXT("requiresParameterToBeFalse")));
	}
	for_every(node, _node->children_named(TXT("requiresToBeFalse")))
	{
		if (node->has_attribute(TXT("parameter")))
		{
			requiresParametersToBeFalse.push_back_unique(node->get_name_attribute(TXT("parameter")));
		}
		else
		{
			error_loading_xml(node, TXT("no \"parameter\" provided"));
			result = false;
		}
	}

	// assumptions
	if (failIfConnected)
	{
		canCreateNewPieces = false;
		addRequired.clear();
		requiredCount = Random::Number<int>(0);
		optionalCount = Random::Number<int>(1);
		totalCount = Random::Number<int>(0);
	}

	/*
	if (requiredCount.get_max() == 0 &&
		optionalCount.get_max() == 0)
	{
		error_loading_xml(_node, TXT("no required nor optional provided for connector %S/%S"), linkName.to_char(), linkNameAlt.to_char());
		result = false;
	}
	*/

	if (type == ConnectorType::Internal)
	{
		if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("external")))
		{
			internalExternalName = attr->get_as_name();
		}
	}
	if (type == ConnectorType::Normal)
	{
		if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("internal")))
		{
			internalExternalName = attr->get_as_name();
		}
	}
		
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("blocks")))
	{
		List<String> blockArray;
		attr->get_as_string().split(String::comma(), blockArray);
		for_every(blockToken, blockArray)
		{
			Name blockName = Name(blockToken->trim());
			if (blockName.is_valid())
			{
				memory_leak_suspect;
				blocks.push_back_unique(blockName);
				forget_memory_leak_suspect;
			}
		}
	}

	return result;
}

template <typename CustomType>
void Connector<CustomType>::make_required_single_only()
{
	addRequired.clear();
	requiredCount = Random::Number<int>(1);
	optionalCount = Random::Number<int>(0);
	totalCount = Random::Number<int>(0);
}

template <typename CustomType>
void Connector<CustomType>::set_link_ps_as_socket(tchar const * _link, bool _allowDuplex)
{
	linkName = Name(String::printf(TXT("%S [socket]"), _link));
	linkWith = Name(String::printf(TXT("%S [plug]"), _link));
	if (_allowDuplex)
	{
		linkNameAlt = Name(String::printf(TXT("%S [duplex]"), _link));
		linkWithAlt = Name(String::printf(TXT("%S [duplex]"), _link));
	}
	else
	{
		linkNameAlt = Name::invalid();
		linkWithAlt = Name::invalid();
	}
}

template <typename CustomType>
void Connector<CustomType>::set_link_ps_as_plug(tchar const * _link, bool _allowDuplex)
{
	linkName = Name(String::printf(TXT("%S [plug]"), _link));
	linkWith = Name(String::printf(TXT("%S [socket]"), _link));
	if (_allowDuplex)
	{
		linkNameAlt = Name(String::printf(TXT("%S [duplex]"), _link));
		linkWithAlt = Name(String::printf(TXT("%S [duplex]"), _link));
	}
	else
	{
		linkNameAlt = Name::invalid();
		linkWithAlt = Name::invalid();
	}
}

template <typename CustomType>
void Connector<CustomType>::set_link_ps_as_duplex(tchar const * _link)
{
	linkName = Name(String::printf(TXT("%S [duplex]"), _link));
	linkWith = Name(String::printf(TXT("%S [duplex]"), _link));
	linkNameAlt = Name::invalid();
	linkWithAlt = Name::invalid();

}

template <typename CustomType>
void Connector<CustomType>::set_link_ps_as_provider(tchar const * _link)
{
	linkName = Name(String::printf(TXT("%S [provider]"), _link));
	linkWith = Name(String::printf(TXT("%S [requester]"), _link));
	linkNameAlt = Name::invalid();
	linkWithAlt = Name::invalid();

}

template <typename CustomType>
void Connector<CustomType>::set_link_ps_as_requester(tchar const * _link)
{
	linkName = Name(String::printf(TXT("%S [requester]"), _link));
	linkWith = Name(String::printf(TXT("%S [provider]"), _link));
	linkNameAlt = Name::invalid();
	linkWithAlt = Name::invalid();

}

template <typename CustomType>
void Connector<CustomType>::set_link_ps_as_refuse(tchar const * _link)
{
	// same as requester, but I'd prefer to have separate function
	set_link_ps_as_requester(_link);
}

template <typename CustomType>
bool Connector<CustomType>::prepare(AN_NOT_CLANG_TYPENAME Piece<CustomType>* _piece, typename CustomType::PreparingContext * _context)
{
	bool result = true;
	result &= def.prepare(_piece, this, _context);
	/* todo
	if (!linkName.is_valid() && !linkWith.is_valid() && ! linkWithAlt.is_valid())
	{
		todo_important(TXT("error - no link name specified"));
		result = false;
	}
	*/
	return result;
}

///////////////////////////////////////////////////////////////////////////////////
// PIECE INSTANCE

template <typename CustomType>
PieceInstance<CustomType>::PieceInstance()
: piece(nullptr)
, inGenerationSpace(nullptr)
, generationSpace(nullptr)
, createdFromProvidedData(false)
{
}

template <typename CustomType>
PieceInstance<CustomType>::~PieceInstance()
{
}

struct ConnectorCountHelper
{
	Name connectorName;
	int required;
	int optional;
	bool optionalInfinite;
	ConnectorCountHelper() {}
	ConnectorCountHelper(Name const & _name, int _required, int _optional, bool _optionalInfinite) : connectorName(_name), required(_required), optional(_optional), optionalInfinite(_optionalInfinite) {}

	static ConnectorCountHelper const * find_in(Name const & _name, Array< ConnectorCountHelper > const & _array)
	{
		for_every_const(cch, _array)
		{
			if (_name == cch->connectorName)
			{
				return cch;
			}
		}
		return nullptr;
	}
};

template <typename CustomType>
void PieceInstance<CustomType>::step__setup(Generator<CustomType>* _generator, Piece<CustomType> const * _piece, ConnectorInstance<CustomType> const* _createdByConnectorInstance)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(piece == nullptr);
	an_assert(_piece != nullptr);
	an_assert(generationSpace == nullptr);
	an_assert(inGenerationSpace != nullptr);

	generationSpace = GenerationSpace<CustomType>::get_one();
	generationSpace->set_owner(this);

	createdFromProvidedData = false;

	if (_createdByConnectorInstance)
	{
		connectorTag = _createdByConnectorInstance->connectorTag;
		provideConnectorTag = _createdByConnectorInstance->provideConnectorTag;
	}
	else
	{
		connectorTag = Name::invalid();
		provideConnectorTag.clear();
	}

	piece = _piece;
#ifdef PIECE_COMBINER_DEBUG
	memory_leak_suspect;
	desc = piece->def.get_desc();
	forget_memory_leak_suspect;
#endif
	
	ARRAY_PREALLOC_SIZE(ConnectorCountHelper, connectorsCreatedInfo, piece->connectors.get_size());

	// add connectors, required always go first
	for_every(connector, piece->connectors)
	{
		if (_generator->ext_should_use_connector)
		{
			if (!_generator->ext_should_use_connector(*_generator, *this, *connector))
			{
				continue;
			}
		}
		// check if we actually should use this connector
		if (connector->probabilityOfExistence < 1.0f)
		{
			if (!_generator->randomGenerator->get_chance(connector->probabilityOfExistence))
			{
				continue;
			}
		}
		if (!connector->check_required_parameters(_generator))
		{
			continue;
		}
		// randomize how many connectors should we add
		int totalLimit = connector->totalCount.get(*_generator->randomGenerator);
		int required = connector->addRequired.get_with_default(_generator->get_parameters_provider(), 0) + connector->requiredCount.get(*_generator->randomGenerator);
		bool optionalInfinite = connector->optionalCount.get_max() == INF_INT;
		int optional = optionalInfinite ? connector->optionalCount.get_min() : connector->optionalCount.get(*_generator->randomGenerator);
		if (!optionalInfinite && connector->requiredAsOptionalProbability > 0.0f)
		{
			if (_generator->randomGenerator->get_chance(connector->requiredAsOptionalProbability))
			{
				optional += required;
				required = 0;
			}
		}
		if (connector->type == ConnectorType::No)
		{
			required = 1;
			optional = 0;
			totalLimit = 0;
			optionalInfinite = false;
		}
		if (connector->type == ConnectorType::Parent)
		{
			// this doesn't mean that there can't be more than one parent
			required = min(1, required);
		}

		// limit to totalLimit
		if (totalLimit > 0)
		{
			required = min(required, totalLimit);
			if (optionalInfinite)
			{
				// with infinite optional, auto fill to total limit
				optional = totalLimit - required;
				// but disable inifinite at the same time
				optionalInfinite = false;
			}
			else
			{
				optional = min(optional, totalLimit - required);
			}
		}
		
		// check if we have matching internal/external
		Name corresponding = Name::invalid();
		if (connector->type == ConnectorType::Internal ||
			connector->type == ConnectorType::Normal)
		{
			corresponding = connector->internalExternalName;
		}
		if (corresponding.is_valid())
		{
			if (ConnectorCountHelper const * foundOther = ConnectorCountHelper::find_in(corresponding, connectorsCreatedInfo))
			{
				// copy as they match
				required = foundOther->required;
				optional = foundOther->optional;
				optionalInfinite = foundOther->optionalInfinite;
			}
		}
		// store counters
		memory_leak_suspect;
		connectorsCreatedInfo.push_back(ConnectorCountHelper(connector->name, required, optional, optionalInfinite));
		forget_memory_leak_suspect;
		// add required
		while (required > 0)
		{
			_generator->do_step(new GenerationSteps::CreateConnectorInstance<CustomType>(connector, this, connectorTag, provideConnectorTag));
			--required;
		}
		if (connector->type != ConnectorType::Parent)
		{
			// add optionals
			// add infinite optional
			if (optionalInfinite)
			{
				_generator->do_step(new GenerationSteps::CreateConnectorInstance<CustomType>(connector, this, connectorTag, provideConnectorTag, GenerationSteps::CreateConnectorAs::OptionalInfinite));
				optional = 0;
			}
			// for infinite we want to do optional count - 1 as last one will be infinite
			while (optional > 0)
			{
				_generator->do_step(new GenerationSteps::CreateConnectorInstance<CustomType>(connector, this, connectorTag, provideConnectorTag, GenerationSteps::CreateConnectorAs::Optional));
				--optional;
			}
		}
	}

	// connect internal/external
	for_every_ptr(externalConnector, ownedConnectors)
	{
		if (externalConnector->get_connector()->type == ConnectorType::Normal &&
			externalConnector->get_connector()->internalExternalName.is_valid() &&
			externalConnector->internalExternalConnector == nullptr)
		{
			for_every_ptr(internalConnector, ownedConnectors)
			{
				if (internalConnector->get_connector()->type == ConnectorType::Internal &&
					internalConnector->internalExternalConnector == nullptr &&
					externalConnector->get_connector()->internalExternalName == internalConnector->get_connector()->name)
				{
					// connect internal and external through step
					_generator->do_step(new GenerationSteps::ConnectInternalExternal<CustomType>(internalConnector, externalConnector));
					break;
				}
			}
		}
	}

	if (_generator->ext_setup_piece_instance)
	{
		_generator->ext_setup_piece_instance(*_generator, *this);
	}
}

template <typename CustomType>
GenerationResult::Type PieceInstance<CustomType>::generate__try_connecting(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _tryConnector)
{
	// try connecting to all connectors in piece instance - if more than one matches, try random
	ARRAY_STATIC(ConnectorInstance<CustomType>*, foundConnectors, 4);// min(4, ownedConnectors.get_size()));
	// first, collect all matching
	for_every_ptr(pieceConnector, ownedConnectors)
	{
		if (pieceConnector->can_connect_with(_tryConnector))
		{
			foundConnectors.push_back_or_replace(pieceConnector, _generator->access_random_generator());
		}
	}
	// connect one of them
	while (! foundConnectors.is_empty())
	{
		int tryIdx = _generator->access_random_generator().get_int(foundConnectors.get_size());
		_generator->checkpoint();
		_tryConnector->connect_with(_generator, foundConnectors[tryIdx]);
		GenerationResult::Type result = _generator->generate__generation_loop();
		if (result == GenerationResult::Success)
		{
			return result;
		}
		_generator->revert();
		foundConnectors.remove_fast_at(tryIdx);
	}
#ifdef LOG_PIECE_COMBINER_FAILS
	log_to_file(TXT("fail: try connecting fail"));
#endif
	_generator->mark_failed();
	return GenerationResult::Failed;
}

template <typename CustomType>
bool PieceInstance<CustomType>::check_if_would_be_connected_as_unique_piece(PieceInstance<CustomType> const * _otherPiece) const
{
	if (!_otherPiece)
	{
		// yes, it would be unique! (it can be through provided connector)
		return true;
	}
	for_every_ptr(connector, ownedConnectors)
	{
		if (connector->get_owner_on_other_end() == _otherPiece)
		{
			// will double!
			return false;
		}
	}
	return true;
}

template <typename CustomType>
void PieceInstance<CustomType>::move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace)
{
	if (inGenerationSpace == _toGenerationSpace)
	{
		// we're already there
		return;
	}
	_generator->do_step(new GenerationSteps::MoveToSpace<CustomType>(this, _toGenerationSpace));
	for_every_ptr(connector, ownedConnectors)
	{
		// connectors will validate on their own
		connector->move_to_space(_generator, _toGenerationSpace);
	}
}

template <typename CustomType>
void PieceInstance<CustomType>::step__block_with(Generator<CustomType>* _generator, Connector<CustomType> const * _connector)
{
	an_assert(_generator->is_valid_step_call());
	for_every(blocks, _connector->blocks)
	{
		for_every_ptr(connector, ownedConnectors)
		{
			if (connector->get_connector()->name == *blocks)
			{
				connector->step__mark_blocked(_generator);
			}
		}
	}
	for_every(blocks, _connector->autoBlocks)
	{
		for_every_ptr(connector, ownedConnectors)
		{
			if (connector->get_connector()->autoId == *blocks)
			{
				connector->step__mark_blocked(_generator);
			}
		}
	}
}

template <typename CustomType>
void PieceInstance<CustomType>::step__unblock_with(Generator<CustomType>* _generator, Connector<CustomType> const * _connector)
{
	an_assert(_generator->is_valid_step_call());
	for_every(blocks, _connector->blocks)
	{
		for_every_ptr(connector, ownedConnectors)
		{
			if (connector->get_connector()->name == *blocks)
			{
				connector->step__mark_unblocked(_generator);
			}
		}
	}
	for_every(blocks, _connector->autoBlocks)
	{
		for_every_ptr(connector, ownedConnectors)
		{
			if (connector->get_connector()->autoId == *blocks)
			{
				connector->step__mark_unblocked(_generator);
			}
		}
	}
}

template <typename CustomType>
void PieceInstance<CustomType>::step__move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace)
{
	an_assert(_generator->is_valid_step_call());
	if (inGenerationSpace)
	{
		inGenerationSpace->step__remove_piece(_generator, this);
	}
	if (_toGenerationSpace)
	{
		_toGenerationSpace->step__add_piece(_generator, this);
	}
}

template <typename CustomType>
void PieceInstance<CustomType>::on_get()
{
	// clear data
	data = AN_CLANG_TYPENAME CustomType::PieceInstanceData();
	connectorTag = Name::invalid();
	provideConnectorTag.clear();
}

template <typename CustomType>
void PieceInstance<CustomType>::on_release()
{
	an_assert(inGenerationSpace == nullptr);
	piece = nullptr;
	generationSpace->clear();
	generationSpace->release();
	generationSpace = nullptr;
	for_every_ptr(connector, ownedConnectors)
	{
		// no need to disconnect owned connectors, generation space did that for us
		connector->release_ref();
	}
	ownedConnectors.clear();
}

template <typename CustomType>
void PieceInstance<CustomType>::step__add_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(!ownedConnectors.does_contain(_connectorInstance));
	_connectorInstance->add_ref();
	memory_leak_suspect;
	ownedConnectors.push_back(_connectorInstance);
	forget_memory_leak_suspect;
}

template <typename CustomType>
void PieceInstance<CustomType>::step__remove_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(ownedConnectors.does_contain(_connectorInstance));
	ownedConnectors.remove(_connectorInstance);
	_connectorInstance->release_ref();
}

template <typename CustomType>
void PieceInstance<CustomType>::check_if_complete()
{
	if (isAccessible)
	{
		// it was already checked
		return;
	}

	isAccessible = true;

	for_every_ptr(connector, ownedConnectors)
	{
		if (connector->get_connector()->def.is_valid_cycle_connector())
		{
			ConnectorInstance<CustomType>* wanderingConnector = connector;
			ConnectorInstance<CustomType>* onOtherEnd = nullptr;
			// whle looking for connector on the other end, mark pieces that such connector belongs to as accessible to allow pieces with just connectors inside to be used
			while (true)
			{
				if (wanderingConnector->connectedTo && wanderingConnector->connectedTo->internalExternalConnector)
				{
					if (wanderingConnector->connectedTo->get_owner())
					{
						wanderingConnector->connectedTo->get_owner()->isAccessible = true;
					}
					wanderingConnector = wanderingConnector->connectedTo->internalExternalConnector;
					continue;
				}
				onOtherEnd = wanderingConnector->connectedTo;
				break;
			}
			if (onOtherEnd &&
				onOtherEnd->get_owner())
			{
				onOtherEnd->get_owner()->check_if_complete();
			}
		}
	}

	check_if_complete_as_parent_child();
}

template <typename CustomType>
void PieceInstance<CustomType>::check_if_complete_as_parent_child()
{
	if (isAccessibleAsParentChild)
	{
		// it was already checked
		return;
	}

	isAccessibleAsParentChild = true;

	for_every_ptr(connector, ownedConnectors)
	{
		ConnectorType::Type connectorType = connector->get_connector()->type;
		if (connectorType == ConnectorType::Parent ||
			connectorType == ConnectorType::Child)
		{
			if (PieceInstance<CustomType>* onOtherEnd = connector->get_owner_on_other_end())
			{
				onOtherEnd->check_if_complete_as_parent_child();
			}
		}
	}
}

template <typename CustomType>
bool PieceInstance<CustomType>::build_cycle(BuildCycleContext<CustomType> const & _context, REF_ int* _piecesInCycleCount, REF_ int* _outerConnectorCountInCycle, REF_ bool* _canCreateAnything)
{
	an_assert(cycleId == 0);

	cycleId = _context.cycleId;

	if (_piecesInCycleCount)
	{
		++ (*_piecesInCycleCount);
	}
	
	bool isOpen = false; // assume if closed, will be modified when at least one is open

	for_every_ptr(connector, ownedConnectors)
	{
		ConnectorType::Type connectorType = connector->get_connector()->type;
		// ignore parent/child - that's not actual cycle
		if (((connectorType != ConnectorType::Parent && connectorType != ConnectorType::Child) ||
			 _context.generator->get_generation_rules().should_treat_parents_and_children_as_parts_of_cycle()) &&
			connectorType != ConnectorType::Internal &&
			! connector->internalExternalConnector) //
		{
			isOpen |= connector->build_cycle(_context, REF_ _piecesInCycleCount, REF_ _outerConnectorCountInCycle, REF_ _canCreateAnything);
		}
	}

	build_cycle_as_parent_child(_context);

	return isOpen;
}

template <typename CustomType>
void PieceInstance<CustomType>::build_cycle_as_parent_child(BuildCycleContext<CustomType> const& _context, int _dir)
{
	if (cycleIdAsParentChild)
	{
		return;
	}

	// we want to prevent situation in which we go up and down and up and down getting everyone involved
	// this is crucial for cases where there might be two separate circles within same parent-children group

	cycleIdAsParentChild = _context.cycleId;

	for_every_ptr(connector, ownedConnectors)
	{
		ConnectorType::Type connectorType = connector->get_connector()->type;
		if ((connectorType == ConnectorType::Parent && _dir >= 0) ||
			(connectorType == ConnectorType::Child && _dir <= 0))
		{
			if (PieceInstance<CustomType>* onOtherEnd = connector->get_owner_on_other_end())
			{
				onOtherEnd->build_cycle_as_parent_child(_context, connectorType == ConnectorType::Parent? 1 : -1);
			}
		}
	}
}

template <typename CustomType>
void PieceInstance<CustomType>::on_generation_space_clear(GenerationSpace<CustomType>* _generationSpace)
{
	if (inGenerationSpace == _generationSpace)
	{
		inGenerationSpace = nullptr;
		for_every_ptr(connector, ownedConnectors)
		{
			connector->on_generation_space_clear(_generationSpace);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
// CONNECTOR INSTANCE

template <typename CustomType>
ConnectorInstance<CustomType>::ConnectorInstance()
: connector(nullptr)
, owner(nullptr)
, inGenerationSpace(nullptr)
, createdFromProvidedData(false)
, connectedTo(nullptr)
, internalExternalConnector(nullptr)
{
}

template <typename CustomType>
void ConnectorInstance<CustomType>::on_get()
{
	// clear data
	data = AN_CLANG_TYPENAME CustomType::ConnectorInstanceData();
	connectorTag = Name::invalid();
	provideConnectorTag.clear();
}

template <typename CustomType>
void ConnectorInstance<CustomType>::on_release()
{
	owner = nullptr;
	inGenerationSpace = nullptr;
	connector = nullptr;
	if (connectedTo)
	{
		connectedTo->release_ref();
		connectedTo = nullptr;
	}
	if (internalExternalConnector)
	{
		internalExternalConnector->release_ref();
		internalExternalConnector = nullptr;
	}
}


template <typename CustomType>
bool ConnectorInstance<CustomType>::build_cycle(BuildCycleContext<CustomType> const& _context, REF_ int * _piecesInCycleCount, REF_ int * _outerConnectorCountInCycle, REF_ bool * _canCreateAnything)
{
	bool isOpen = false; // assume if closed, will be modified when at least one is open

	if (is_outer()) // ignore outers
	{
		if (_outerConnectorCountInCycle)
		{
			++(*_outerConnectorCountInCycle);
		}
	}

	// do for all, event for outers
	{
		if (ConnectorInstance<CustomType>* onOtherEnd = get_connector_on_other_end())
		{
			if (get_connector()->def.is_valid_cycle_connector()) // build only for normals
			{
				// check if there is piece on other end - if no, it means that it is outer
				if (PieceInstance<CustomType>* pieceOnOtherEnd = onOtherEnd->get_owner())
				{
					if (!pieceOnOtherEnd->is_in_cycle())
					{
						// go through connectors right to the end and mark pieces they are in that they are in cycle
						// this is to avoid treating pieces that are just bubbles of other pieces from being parts of separate cycles - unless of course they are connected directly
						mark_pieces_on_the_way_to_the_other_end_part_of_cycle(_context.cycleId);
						isOpen |= pieceOnOtherEnd->build_cycle(_context, REF_ _piecesInCycleCount, REF_ _outerConnectorCountInCycle, REF_ _canCreateAnything);
					}
				}
			}
		}
		else if (! _context.ignoreOptionals || !is_optional())
		{
			// check if we can still create anything
			if (_canCreateAnything)
			{
				// if we have string of connectors, check the last one to see if maybe we can use it to create new pieces
				ConnectorInstance<CustomType>* checkOnTheWay = this;
				while (checkOnTheWay)
				{
					if (!checkOnTheWay->connectedTo)
					{
						*_canCreateAnything |= checkOnTheWay->get_connector()->canCreateNewPieces;
						break;
					}
					else if (checkOnTheWay->connectedTo->internalExternalConnector)
					{
						checkOnTheWay = checkOnTheWay->connectedTo->internalExternalConnector;
					}
					else
					{
						break;
					}
				}
			}
			// doesn't have connector on other end - it is open (even if it is parent/child)
			isOpen = true;
		}
	}

	return isOpen;
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__setup(Generator<CustomType>* _generator, Connector<CustomType> const * _connector, PieceInstance<CustomType>* _owner, Optional<Name> const& _inheritConnectorTag, Optional<Name> const& _inheritProvideConnectorTag)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(inGenerationSpace == nullptr);
	an_assert(connectedTo == nullptr);
	an_assert(internalExternalConnector == nullptr);
	an_assert(owner == nullptr);

	createdFromProvidedData = false;

	connector = _connector;
	connectorTag = _connector->connectorTag; // will be overridden if created
	provideConnectorTag = connector->provideConnectorTag;
	if (_connector->inheritConnectorTag)
	{
		connectorTag = _inheritProvideConnectorTag.get(_inheritConnectorTag.get(connectorTag));
	}
	if (_connector->inheritProvideConnectorTag)
	{
		provideConnectorTag = _inheritProvideConnectorTag;
	}

#ifdef PIECE_COMBINER_DEBUG
	memory_leak_suspect;
	desc = connector->def.get_desc();
	forget_memory_leak_suspect;
#endif

	owner = _owner;
	isOuter = owner == nullptr;
	isInfinite = false;
	isOptional = false;
	isNoConnector = _connector->type == ConnectorType::No;
	blockCount = 0;
	side = connector->side;
	shouldNotExist = false;
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__be_optional(Generator<CustomType>* _generator)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(connector != nullptr);
	isOptional = true;
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__be_infinite(Generator<CustomType>* _generator)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(connector != nullptr);
	isInfinite = true;
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__connect_internal_external(Generator<CustomType>* _generator, ConnectorInstance* _with)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(get_connector()->internalExternalName == _with->get_connector()->name);
	an_assert(_with->get_connector()->internalExternalName == get_connector()->name);
	an_assert(internalExternalConnector == nullptr);
	an_assert(_with->internalExternalConnector == nullptr);
	internalExternalConnector = _with;
	internalExternalConnector->add_ref();
	_with->internalExternalConnector = this;
	_with->internalExternalConnector->add_ref();
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__disconnect_internal_external(Generator<CustomType>* _generator)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(internalExternalConnector != nullptr);
	an_assert(internalExternalConnector->internalExternalConnector == this);
	internalExternalConnector->internalExternalConnector->release_ref();
	internalExternalConnector->internalExternalConnector = nullptr;
	internalExternalConnector->release_ref();
	internalExternalConnector = nullptr;
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__connect(Generator<CustomType>* _generator, ConnectorInstance* _with)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(this != _with);
	an_assert(connectedTo == nullptr);
	an_assert(_with->connectedTo == nullptr);
	connectedTo = _with;
	_with->connectedTo = this;
	// both want to store ref, do it now
	connectedTo->add_ref();
	add_ref();
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__disconnect(Generator<CustomType>* _generator)
{
	an_assert(_generator->is_valid_step_call());
	an_assert(connectedTo != nullptr);
	an_assert(connectedTo->connectedTo == this);
	// both are ref counted, release ref now
	connectedTo->release_ref();
	release_ref();
	//
	connectedTo->connectedTo = nullptr;
	connectedTo = nullptr;
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__block(Generator<CustomType>* _generator)
{
	an_assert(_generator->is_valid_step_call());
	if (owner)
	{
		owner->step__block_with(_generator, connector);
	}
	else if (inGenerationSpace)
	{
		inGenerationSpace->step__block_non_owned_with(_generator, connector);
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__unblock(Generator<CustomType>* _generator)
{
	an_assert(_generator->is_valid_step_call());
	if (owner)
	{
		owner->step__unblock_with(_generator, connector);
	}
	else if (inGenerationSpace)
	{
		inGenerationSpace->step__unblock_non_owned_with(_generator, connector);
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__mark_blocked(Generator<CustomType>* _generator, bool _blockInternalExternal)
{
	an_assert(_generator->is_valid_step_call());
	++ blockCount;
	if (_blockInternalExternal)
	{
		if (internalExternalConnector)
		{
			internalExternalConnector->step__mark_blocked(_generator, false);
		}
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__mark_unblocked(Generator<CustomType>* _generator, bool _unblockInternalExternal)
{
	an_assert(_generator->is_valid_step_call());
	-- blockCount;
	if (_unblockInternalExternal) // just blocked first time
	{
		if (internalExternalConnector)
		{
			internalExternalConnector->step__mark_unblocked(_generator, false);
		}
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::on_connected(Generator<CustomType>* _generator)
{
	if (connector->type == ConnectorType::Parent)
	{
		an_assert(connectedTo->owner != nullptr);
		an_assert(owner != nullptr);
		// force owner to be moved to generation space - it will move this connector too
		owner->move_to_space(_generator, connectedTo->owner->get_generation_space());
	}
	if (isInfinite)
	{
		if (internalExternalConnector)
		{
			// create another copy of pair of infinite
			GenerationSteps::CreateConnectorInstance<CustomType>* stepA = new GenerationSteps::CreateConnectorInstance<CustomType>(connector, owner, connectorTag, provideConnectorTag, GenerationSteps::CreateConnectorAs::OptionalInfinite);
			GenerationSteps::CreateConnectorInstance<CustomType>* stepB = new GenerationSteps::CreateConnectorInstance<CustomType>(internalExternalConnector->connector, internalExternalConnector->owner, internalExternalConnector->connectorTag, internalExternalConnector->provideConnectorTag, GenerationSteps::CreateConnectorAs::OptionalInfinite);
			_generator->do_step(stepA);
			_generator->do_step(stepB);
			// connect internal and external through step
			_generator->do_step(new GenerationSteps::ConnectInternalExternal<CustomType>(stepA->connectorInstance, stepB->connectorInstance));
		}
		else
		{
			// create another copy of infinite
			_generator->do_step(new GenerationSteps::CreateConnectorInstance<CustomType>(connector, owner, connectorTag, provideConnectorTag, GenerationSteps::CreateConnectorAs::OptionalInfinite));
		}
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace)
{
	if (inGenerationSpace == _toGenerationSpace)
	{
		// we're already there
		return;
	}
	if (connector->type == ConnectorType::Child ||
		connector->type == ConnectorType::Internal)
	{
		// leave as they are, they are in internal space
		return;
	}
	// if is connected and not connected to outer, disconnect (we will be in different space - outer may leave where it is - it doesn't matter)
	if (is_connected() && !connectedTo->is_outer())
	{
		_generator->do_step(new GenerationSteps::Disconnect<CustomType>(this));
	}
	// move to new generation space
	_generator->do_step(new GenerationSteps::MoveToSpace<CustomType>(this, _toGenerationSpace));
}

template <typename CustomType>
void ConnectorInstance<CustomType>::step__move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace)
{
	an_assert(_generator->is_valid_step_call());
	if (inGenerationSpace)
	{
		inGenerationSpace->step__remove_connector(_generator, this);
	}
	if (_toGenerationSpace)
	{
		_toGenerationSpace->step__add_connector(_generator, this);
	}
}

template <typename CustomType>
bool ConnectorInstance<CustomType>::can_connect_with(ConnectorInstance<CustomType> const* _with) const
{
	if (this == _with)
	{
		return false;
	}
	if (connectorTag != _with->connectorTag)
	{
		return false;
	}
	if (get_connector()->type == ConnectorType::Internal && get_connector()->requiresExternalConnected && !internalExternalConnector)
	{
		return false;
	}
	if (_with->get_connector()->type == ConnectorType::Internal && _with->get_connector()->requiresExternalConnected && !_with->internalExternalConnector)
	{
		return false;
	}
	if (get_connector()->type == ConnectorType::No || _with->get_connector()->type == ConnectorType::No)
	{
		return false;
	}
	if ((inGenerationSpace == _with->inGenerationSpace) ||
		(get_connector()->type == ConnectorType::Parent && _with->get_connector()->type == ConnectorType::Child) ||
		(get_connector()->type == ConnectorType::Child && _with->get_connector()->type == ConnectorType::Parent))
	{
		return connector->can_connect_with(_with->connector);
	}
	else
	{
		return false;
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::mark_pieces_on_the_way_to_the_other_end_part_of_cycle(int _cycleId)
{
	if (connectedTo && connectedTo->internalExternalConnector)
	{
		if (auto * iecOwner = connectedTo->internalExternalConnector->get_owner())
		{
			// all pieces between two connectors that are just owners of internal/external connectors are marked as part of cycle
			// this way we won't get cycle orphans
			if (!iecOwner->is_in_cycle())
			{
				iecOwner->cycleId = _cycleId;
			}
		}
		// transfer to internal/external
		connectedTo->internalExternalConnector->mark_pieces_on_the_way_to_the_other_end_part_of_cycle(_cycleId);
	}
}

template <typename CustomType>
ConnectorInstance<CustomType>* ConnectorInstance<CustomType>::get_connector_on_other_end() const
{
	if (connectedTo && connectedTo->internalExternalConnector)
	{
		// transfer to internal/external
		return connectedTo->internalExternalConnector->get_connector_on_other_end();
	}
	return connectedTo;
}

template <typename CustomType>
PieceInstance<CustomType>* ConnectorInstance<CustomType>::get_owner_on_other_end() const
{
	if (ConnectorInstance<CustomType>* otherEnd = get_connector_on_other_end())
	{
		return otherEnd->get_owner();
	}
	else
	{
		return nullptr;
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::define_side()
{
	if (side == ConnectorSide::Any)
	{
		if (ConnectorInstance* onOtherEnd = get_connector_on_other_end())
		{
			side = ConnectorSide::opposite(onOtherEnd->side);
		}
		else
		{
			side = ConnectorSide::A;
		}
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::on_generation_space_clear(GenerationSpace<CustomType>* _generationSpace)
{
	if (inGenerationSpace == _generationSpace)
	{
		inGenerationSpace = nullptr;
	}
}

template <typename CustomType>
void ConnectorInstance<CustomType>::connect_with(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connector)
{
	_generator->do_step(new GenerationSteps::Connect<CustomType>(this, _connector));
	_generator->do_step(new GenerationSteps::BlockWith<CustomType>(this));
	_generator->do_step(new GenerationSteps::BlockWith<CustomType>(_connector));
	// handle parent/child connection and infinite connections
	on_connected(_generator);
	_connector->on_connected(_generator);
}

template <typename CustomType>
void ConnectorInstance<CustomType>::post_generation__disconnect()
{
	if (connectedTo)
	{
		// treat them as blocked
		++connectedTo->blockCount;
		++blockCount;

		auto* wasConnectedTo = connectedTo;
		connectedTo->connectedTo = nullptr;
		connectedTo = nullptr;

		if (wasConnectedTo->internalExternalConnector)
		{
			wasConnectedTo->internalExternalConnector->post_generation__disconnect();
		}
	}
}

#ifdef AN_DEVELOPMENT
#undef LOG_PIECE_COMBINER
#undef LOG_PIECE_COMBINER_FAILS
#undef LOG_PIECE_COMBINER_DETAILS
#undef LOG_PIECE_COMBINER_SUCCESS
#endif