#pragma once

#include "..\fastCast.h"

namespace PieceCombiner
{
	namespace GenerationSteps
	{
		template <typename CustomType>
		class Checkpoint
		: public PooledObject<Checkpoint<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(Checkpoint);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			Checkpoint(int _id);
			override_ void execute(Generator<CustomType>* _generator) {}
			override_ void revert(Generator<CustomType>* _generator) {}
			//
			int get_id() const { return id; }
		private:
			int id;
		};

		template <typename CustomType>
		class CreatePieceInstance
		: public PooledObject<CreatePieceInstance<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(CreatePieceInstance);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			CreatePieceInstance(Piece<CustomType> const * _piece, GenerationSpace<CustomType>* _generationSpace, ConnectorInstance<CustomType> const* _createdByConnectorInstance);
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			GenerationSpace<CustomType>* generationSpace = nullptr;
			Piece<CustomType> const * piece = nullptr;
			ConnectorInstance<CustomType> const* createdByConnectorInstance = nullptr;
			//
			PieceInstance<CustomType>* pieceInstance = nullptr;
		};

		namespace CreateConnectorAs
		{
			enum Type
			{
				Normal,
				Optional,
				OptionalInfinite
			};
		};

		template <typename CustomType>
		class CreateConnectorInstance
		: public PooledObject<CreateConnectorInstance<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(CreateConnectorInstance);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			CreateConnectorInstance(Connector<CustomType> const * _connector, PieceInstance<CustomType>* _owner, Optional<Name> const& _inheritConnectorTag, Optional<Name> const& _inheritProvideConnectorTag, CreateConnectorAs::Type _type = CreateConnectorAs::Normal);
			CreateConnectorInstance(Connector<CustomType> const * _connector, GenerationSpace<CustomType>* _generationSpace, Optional<Name> const& _inheritConnectorTag, Optional<Name> const& _inheritProvideConnectorTag); // for outers
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			GenerationSpace<CustomType>* generationSpace = nullptr;
			CreateConnectorAs::Type type = CreateConnectorAs::Normal;
			//
			Connector<CustomType> const * connector = nullptr;
			Optional<Name> inheritConnectorTag;
			Optional<Name> inheritProvideConnectorTag;
			ConnectorSide::Type side = ConnectorSide::Any;
			PieceInstance<CustomType>* owner = nullptr;
			//
			ConnectorInstance<CustomType>* connectorInstance = nullptr;
		};

		template <typename CustomType>
		class Connect
		: public PooledObject<Connect<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(Connect);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			Connect(ConnectorInstance<CustomType>* _connectorA, ConnectorInstance<CustomType>* _connectorB);
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			ConnectorInstance<CustomType>* connectorA = nullptr;
			ConnectorInstance<CustomType>* connectorB = nullptr;
		};

		template <typename CustomType>
		class BlockSingle
		: public PooledObject<BlockSingle<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(BlockSingle);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			BlockSingle(ConnectorInstance<CustomType>* _connector);
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			ConnectorInstance<CustomType>* connector = nullptr;
		};

		template <typename CustomType>
		class BlockWith
		: public PooledObject<BlockWith<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(BlockWith);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			BlockWith(ConnectorInstance<CustomType>* _connector);
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			ConnectorInstance<CustomType>* connector = nullptr;
		};

		template <typename CustomType>
		class Disconnect
		: public PooledObject<Disconnect<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(Disconnect);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			Disconnect(ConnectorInstance<CustomType>* _connector);
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			ConnectorInstance<CustomType>* connectorA = nullptr;
			ConnectorInstance<CustomType>* connectorB = nullptr;
		};

		template <typename CustomType>
		class MoveToSpace
		: public PooledObject<MoveToSpace<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(MoveToSpace);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			MoveToSpace(ConnectorInstance<CustomType>* _connector, GenerationSpace<CustomType>* _toGenerationSpace);
			MoveToSpace(PieceInstance<CustomType>* _piece, GenerationSpace<CustomType>* _toGenerationSpace);
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			ConnectorInstance<CustomType>* connector = nullptr;
			PieceInstance<CustomType>* piece = nullptr;
			GenerationSpace<CustomType>* destSpace = nullptr;
			//
			GenerationSpace<CustomType>* fromSpace = nullptr;
		};

		template <typename CustomType>
		class ConnectInternalExternal
		: public PooledObject<ConnectInternalExternal<CustomType> >
		, public GenerationStep<CustomType>
		{
			FAST_CAST_DECLARE(ConnectInternalExternal);
			FAST_CAST_BASE(GenerationStep<CustomType>);
			FAST_CAST_END();

			typedef GenerationStep<CustomType> base;
		public:
			ConnectInternalExternal(ConnectorInstance<CustomType>* _connectorA, ConnectorInstance<CustomType>* _connectorB);
			override_ void execute(Generator<CustomType>* _generator);
			override_ void revert(Generator<CustomType>* _generator);
			//
			ConnectorInstance<CustomType>* connectorA = nullptr;
			ConnectorInstance<CustomType>* connectorB = nullptr;
		};

	};

};