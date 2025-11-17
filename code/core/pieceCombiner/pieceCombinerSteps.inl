///////////////////////////////////////////////////////////////////////////////////
// CHECKPOINT

template <typename CustomType>
REGISTER_FOR_FAST_CAST(Checkpoint<CustomType>);

template <typename CustomType>
GenerationSteps::Checkpoint<CustomType>::Checkpoint(int _id)
: id(_id)
{
}

///////////////////////////////////////////////////////////////////////////////////
// CREATE PIECE INSTANCE

template <typename CustomType>
REGISTER_FOR_FAST_CAST(CreatePieceInstance<CustomType>);

template <typename CustomType>
CreatePieceInstance<CustomType>::CreatePieceInstance(Piece<CustomType> const * _piece, GenerationSpace<CustomType>* _generationSpace, ConnectorInstance<CustomType> const* _createdByConnectorInstance)
: generationSpace(_generationSpace)
, piece(_piece)
, createdByConnectorInstance(_createdByConnectorInstance)
, pieceInstance(nullptr)
{
}

template <typename CustomType>
void CreatePieceInstance<CustomType>::execute(Generator<CustomType>* _generator)
{
	an_assert(pieceInstance == nullptr);
	memory_leak_suspect;
	pieceInstance = PieceInstance<CustomType>::get_one();
	forget_memory_leak_suspect;

	_generator->step__add_piece_instance_to_all(pieceInstance);
	generationSpace->step__add_piece(_generator, pieceInstance);

	pieceInstance->step__setup(_generator, piece, createdByConnectorInstance);
}

template <typename CustomType>
void CreatePieceInstance<CustomType>::revert(Generator<CustomType>* _generator)
{
	an_assert(pieceInstance != nullptr);

	generationSpace->step__remove_piece(_generator, pieceInstance);
	_generator->step__remove_piece_instance_from_all(pieceInstance);

	pieceInstance = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////
// CREATE CONNECTOR INSTANCE

template <typename CustomType>
REGISTER_FOR_FAST_CAST(CreateConnectorInstance<CustomType>);

template <typename CustomType>
CreateConnectorInstance<CustomType>::CreateConnectorInstance(Connector<CustomType> const * _connector, PieceInstance<CustomType>* _owner, Optional<Name> const& _inheritConnectorTag, Optional<Name> const& _inheritProvideConnectorTag, CreateConnectorAs::Type _type)
: generationSpace(nullptr)
, type(_type)
, connector(_connector)
, inheritConnectorTag(_inheritConnectorTag)
, inheritProvideConnectorTag(_inheritProvideConnectorTag)
, owner(_owner)
, connectorInstance(nullptr)
{
	an_assert(owner != nullptr);
	if (connector->type == ConnectorType::Internal || connector->type == ConnectorType::Child)
	{
		generationSpace = owner->get_generation_space();
	}
	else if (connector->type == ConnectorType::Normal || connector->type == ConnectorType::Parent || connector->type == ConnectorType::No)
	{
		generationSpace = owner->get_in_generation_space();
	}
	else
	{
		an_assert(false);
	}
}

template <typename CustomType>
CreateConnectorInstance<CustomType>::CreateConnectorInstance(Connector<CustomType> const * _connector, GenerationSpace<CustomType>* _generationSpace, Optional<Name> const& _inheritConnectorTag, Optional<Name> const& _inheritProvideConnectorTag)
: generationSpace(_generationSpace)
, type(CreateConnectorAs::Normal)
, connector(_connector)
, inheritConnectorTag(_inheritConnectorTag)
, inheritProvideConnectorTag(_inheritProvideConnectorTag)
, owner(nullptr)
, connectorInstance(nullptr)
{
	an_assert(generationSpace != nullptr);
}

template <typename CustomType>
void CreateConnectorInstance<CustomType>::execute(Generator<CustomType>* _generator)
{
	an_assert(connectorInstance == nullptr);
	an_assert(generationSpace != nullptr);
	memory_leak_suspect;
	connectorInstance = ConnectorInstance<CustomType>::get_one();
	forget_memory_leak_suspect;
	connectorInstance->step__setup(_generator, connector, owner, inheritConnectorTag, inheritProvideConnectorTag);
	
	if (type == CreateConnectorAs::Optional)
	{
		connectorInstance->step__be_optional(_generator);
	}
	if (type == CreateConnectorAs::OptionalInfinite)
	{
		connectorInstance->step__be_optional(_generator);
		connectorInstance->step__be_infinite(_generator);
	}

	_generator->step__add_connector_instance_to_all(connectorInstance);
	generationSpace->step__add_connector(_generator, connectorInstance);
	if (owner)
	{
		owner->step__add_connector(_generator, connectorInstance);
	}
}

template <typename CustomType>
void CreateConnectorInstance<CustomType>::revert(Generator<CustomType>* _generator)
{
	an_assert(connectorInstance != nullptr);

	if (owner)
	{
		owner->step__remove_connector(_generator, connectorInstance);
	}
	generationSpace->step__remove_connector(_generator, connectorInstance);
	_generator->step__remove_connector_instance_from_all(connectorInstance);

	connectorInstance = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////
// CONNECT INTERNAL EXTERNAL

template <typename CustomType>
REGISTER_FOR_FAST_CAST(ConnectInternalExternal<CustomType>);

template <typename CustomType>
ConnectInternalExternal<CustomType>::ConnectInternalExternal(ConnectorInstance<CustomType>* _connectorA, ConnectorInstance<CustomType>* _connectorB)
: connectorA(_connectorA)
, connectorB(_connectorB)
{
	an_assert(connectorA != nullptr);
	an_assert(connectorB != nullptr);
	an_assert(connectorA->internalExternalConnector == nullptr);
	an_assert(connectorB->internalExternalConnector == nullptr);
}

template <typename CustomType>
void ConnectInternalExternal<CustomType>::execute(Generator<CustomType>* _generator)
{
	an_assert(connectorA->internalExternalConnector == nullptr);
	an_assert(connectorB->internalExternalConnector == nullptr);
	connectorA->step__connect_internal_external(_generator, connectorB);
}

template <typename CustomType>
void ConnectInternalExternal<CustomType>::revert(Generator<CustomType>* _generator)
{
	connectorA->step__disconnect_internal_external(_generator);
}

///////////////////////////////////////////////////////////////////////////////////
// CONNECT

template <typename CustomType>
REGISTER_FOR_FAST_CAST(Connect<CustomType>);

template <typename CustomType>
Connect<CustomType>::Connect(ConnectorInstance<CustomType>* _connectorA, ConnectorInstance<CustomType>* _connectorB)
: connectorA(_connectorA)
, connectorB(_connectorB)
{
}

template <typename CustomType>
void Connect<CustomType>::execute(Generator<CustomType>* _generator)
{
	an_assert(!connectorA->is_connected());
	an_assert(!connectorB->is_connected());

	connectorA->step__connect(_generator, connectorB);
}

template <typename CustomType>
void Connect<CustomType>::revert(Generator<CustomType>* _generator)
{
	an_assert(connectorA->is_connected());
	an_assert(connectorB->is_connected());

	connectorA->step__disconnect(_generator);
}

///////////////////////////////////////////////////////////////////////////////////
// BLOCK SINGLE (connector)

template <typename CustomType>
REGISTER_FOR_FAST_CAST(BlockSingle<CustomType>);

template <typename CustomType>
BlockSingle<CustomType>::BlockSingle(ConnectorInstance<CustomType>* _connector)
: connector(_connector)
{
}

template <typename CustomType>
void BlockSingle<CustomType>::execute(Generator<CustomType>* _generator)
{
	connector->step__mark_blocked(_generator);
}

template <typename CustomType>
void BlockSingle<CustomType>::revert(Generator<CustomType>* _generator)
{
	connector->step__mark_unblocked(_generator);
}

///////////////////////////////////////////////////////////////////////////////////
// BLOCK WITH (connector)

template <typename CustomType>
REGISTER_FOR_FAST_CAST(BlockWith<CustomType>);

template <typename CustomType>
BlockWith<CustomType>::BlockWith(ConnectorInstance<CustomType>* _connector)
: connector(_connector)
{
}

template <typename CustomType>
void BlockWith<CustomType>::execute(Generator<CustomType>* _generator)
{
	connector->step__block(_generator);
}

template <typename CustomType>
void BlockWith<CustomType>::revert(Generator<CustomType>* _generator)
{
	connector->step__unblock(_generator);
}

///////////////////////////////////////////////////////////////////////////////////
// DISCONNECT

template <typename CustomType>
REGISTER_FOR_FAST_CAST(Disconnect<CustomType>);

template <typename CustomType>
Disconnect<CustomType>::Disconnect(ConnectorInstance<CustomType>* _connector)
: connectorA(_connector)
, connectorB(_connector->connectedTo)
{
}

template <typename CustomType>
void Disconnect<CustomType>::execute(Generator<CustomType>* _generator)
{
	an_assert(connectorA->is_connected() && connectorA->connectedTo == connectorB);
	an_assert(connectorB->is_connected() && connectorB->connectedTo == connectorA);

	connectorA->step__disconnect(_generator);
}

template <typename CustomType>
void Disconnect<CustomType>::revert(Generator<CustomType>* _generator)
{
	an_assert(! connectorA->is_connected());
	an_assert(! connectorB->is_connected());

	connectorA->step__connect(_generator, connectorB);
}

///////////////////////////////////////////////////////////////////////////////////
// MOVE TO SPACE

template <typename CustomType>
REGISTER_FOR_FAST_CAST(MoveToSpace<CustomType>);

template <typename CustomType>
MoveToSpace<CustomType>::MoveToSpace(ConnectorInstance<CustomType>* _connector, GenerationSpace<CustomType>* _toGenerationSpace)
: connector(_connector)
, piece(nullptr)
, destSpace(_toGenerationSpace)
, fromSpace(nullptr)
{
}

template <typename CustomType>
MoveToSpace<CustomType>::MoveToSpace(PieceInstance<CustomType>* _piece, GenerationSpace<CustomType>* _toGenerationSpace)
: connector(nullptr)
, piece(_piece)
, destSpace(_toGenerationSpace)
, fromSpace(nullptr)
{
}

template <typename CustomType>
void MoveToSpace<CustomType>::execute(Generator<CustomType>* _generator)
{
	if (connector)
	{
		fromSpace = connector->get_in_generation_space();
		connector->step__move_to_space(_generator, destSpace);
	}
	if (piece)
	{
		fromSpace = piece->get_in_generation_space();
		piece->step__move_to_space(_generator, destSpace);
	}
}

template <typename CustomType>
void MoveToSpace<CustomType>::revert(Generator<CustomType>* _generator)
{
	if (connector)
	{
		connector->step__move_to_space(_generator, fromSpace);
	}
	if (piece)
	{
		piece->step__move_to_space(_generator, fromSpace);
	}
}
