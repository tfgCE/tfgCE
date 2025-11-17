#pragma once

#include <functional>

#include "..\fastCast.h"
#include "..\functionParamsStruct.h"

#include "..\containers\array.h"
#include "..\containers\arrayStack.h"
#include "..\containers\arrayStatic.h"
#include "..\containers\list.h"
#include "..\containers\map.h"
#include "..\io\xml.h"
#include "..\math\math.h"
#include "..\memory\refCountObject.h"
#include "..\memory\softPooledObject.h"
#include "..\other\simpleVariableStorage.h"
#include "..\random\random.h"
#include "..\random\randomNumber.h"
#include "..\system\timeStamp.h"
#include "..\types\name.h"
#include "..\wheresMyPoint\iWMPOwner.h"
#include "..\wheresMyPoint\wmpParam.h"

#ifdef AN_DEVELOPMENT
#define PIECE_COMBINER_DEBUG
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define PIECE_COMBINER_VISUALISE
#endif

/*
 *	Note, if you get error on linking about unresolved symbol, then check if you included implementation:
 *		#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
 */

namespace PieceCombiner
{
	/*
	 *	Note:
	 *		step_ prefix means that method should be only called from step!
	 *		generate_ prefix means that method is part of generation process
	 */
	 
	/*
	 *	Outer connectors are provided during generation setup
	 *
	 *	Internal/external are used to separate generation spaces.
	 *	It is useful when we want to have a particular group of pieces inside our generation space and provide
	 *
	 *	Parent/child should be used to check whether we have ave less checks and are used to group pieces (limits can be a good example). Important thing to remember is that child connector is created
	 *	in internal generation space.
	 */

	// forward declarations
	template <typename CustomType> class Generator;
	template <typename CustomType> class GenerationStep;
	template <typename CustomType> struct GenerationRules;
	template <typename CustomType> struct GenerationSpace;
	template <typename CustomType> struct UsePiece;
	template <typename CustomType> struct Piece;
	template <typename CustomType> struct Connector;
	template <typename CustomType> struct PieceInstance;
	template <typename CustomType> struct ConnectorInstance;

	/*
	 *	DefaultCustomGenerator gives base classes for custom generation.
	 *
	 *	They can be used as bases, but there is no virtuality here - no need for that.
	 *	Deriving from these classes will just make slighlty bit easier as some functions will have default implementation.
	 */
	class DefaultCustomGenerator
	{
	public:
		struct LoadingContext
		{
			int empty;
		};

		struct PreparingContext
		{
			int empty;
		};

		struct GenerationContext
		{
			int empty;
		};

		struct PieceDef
		{
			Name name;

			bool load_from_xml(IO::XML::Node const * _node, LoadingContext * _context = nullptr) { return true; }
			bool prepare(Piece<DefaultCustomGenerator>* _piece, PreparingContext * _context = nullptr) { return true; }
			
			String get_desc() const { return name.to_string(); }
		};

		struct ConnectorDef
		{
			Name name;

			bool load_from_xml(IO::XML::Node const * _node, LoadingContext * _context = nullptr) { return true; }
			bool prepare(Piece<DefaultCustomGenerator>* _piece, Connector<DefaultCustomGenerator>* _connector, PreparingContext * _context = nullptr) { return true; }

			bool is_valid_cycle_connector() const { return true; } // if can create cycle/completion
			bool are_sides_symmetrical() const { return true; } // if symmetrical side A can join with side A - rule of thumb is to always join B to A
			bool can_join_to(ConnectorDef const & _other) const { return true; } // special rule if connectors can join together if can link with it - some can be two sided

			String get_desc() const { return name.to_string(); }
		};

		struct PieceInstanceData
		{
			int empty;

			String get_desc() const { return String::empty(); }
		};

		struct ConnectorInstanceData
		{
			int empty;
		};
	};

	namespace GenerationResult
	{
		enum Type
		{
			Success,
			Failed
		};
	};

	namespace ConnectorSide
	{
		enum Type
		{
			Any,
			A,
			B
		};

		inline Type opposite(Type _side) { return _side == A ? B : A; }
		inline bool can_connect(Type _first, Type _second) { return (_first == A && _second == B) || (_first == B && _second == A) || _first == Any || _second == Any; }
	};

	namespace ConnectorType
	{
		enum Type
		{
			/**
			 *
			 *
			 *						   <----- in generation space (inGenerationSpace)
			 *				   parent
			 *					 |
			 *				+----o--------+
			 *				|	 		  |
			 *				|  o-child	  o- normal
			 *				|			  |
			 *				| o-internal  |
			 *				|			  |
			 *		normal -o- internal   |
			 *				|			<---- internal generation space (generationSpace)
			 *				|    PIECE    |
			 *				+--INSTANCE---+
			 *
			 *
			 *	Note that basing on this picture internal and child connectors are almost the same,
			 *	similiarily to normal and parent connectors. But in fact child/parent connectors
			 *	have less checks done.
			 */
			Unknown,
			No, // no/null connetor (doesn't connect to anything)
			Normal, // normal/external, may be used in internal/external pair
			Internal, // internal connector, within generation space, may be used in internal/external pair
			Child, // connects to child - it is just "hanging around" in internal generation space
			Parent, // connects to parent - finds child connector in "in generation space"
		};

		inline Type parse(String const & _string);
	};

};

#include "pieceCombinerSteps.h"

namespace PieceCombiner
{
	template <typename CustomType> class DebugVisualiser;

	namespace FindFreeConnectorsBest
	{
		enum Type
		{
			Normal, // best is usual, none, nothing special
			InternalExternal,
			Parent,
			Outer, // check isOuter
		};
	};

	namespace FindFreeConnectorsReason
	{
		enum Type
		{
			ForLoop,
			ForLoopWithOptionals,
			ToComplete
		};
	};

	namespace GenerateLoopReason
	{
		enum Type
		{
			Normal,
			TryOptionals
		};
	};

	namespace TryConnectingConnectorMethod
	{
		enum Type
		{
			Default, // whatever suits
			IgnorePreferCreatingNewPieces,
			IgnorePreferEachConnectorHasUniquePiece,
			
			NUM
		};
	};

	struct AddPieceTypeParams
	{
		ADD_FUNCTION_PARAM(AddPieceTypeParams, float, probabilityCoef, with_probability_coef);
		ADD_FUNCTION_PARAM(AddPieceTypeParams, float, mulProbabilityCoef, with_mul_probability_coef);
		ADD_FUNCTION_PARAM_PTR(AddPieceTypeParams, Random::Number<int> const, limitCount, with_limit_count);
		ADD_FUNCTION_PARAM_PLAIN(AddPieceTypeParams, Name, canBeCreatedOnlyViaConnectorWithCreateNewPieceMark, can_be_created_only_via_connector_with_create_new_piece_mark);
	};

	/*
	 *	Handles generation.
	 *	Uses provided pieces.
	 *	Creates piece instances and connector instances.
	 */
	template <typename CustomType>
	class Generator
	{
	public:
		typedef std::function<bool(Generator<CustomType> & _generator)> UpdateAndValidate;
		typedef std::function<void(Generator<CustomType> const & _generator, PieceInstance<CustomType> & _pieceInstance)> SetupPieceInstance;
		typedef std::function<bool(Generator<CustomType> const & _generator, PieceInstance<CustomType> const & _pieceInstance, Connector<CustomType> const & _connector)> ShouldUseConnector;

		struct OuterConnector
		{
			Connector<CustomType> const* connector = nullptr;
			ConnectorSide::Type side = ConnectorSide::A;
			// side for outer connector is opposite of what we request to be connected inside
			//
			// example 1:
			// you have door in outer world	D										 D
			// piece 1 is connected through D to... something					1 A- D
			// we want to generate that something,
			//   that's why we need D to act as outer connector						 D [ o?-  ... ]
			// imagine that in the end we want to have piece 1
			//   connected to piece 2 inside									1 A-   -B 2
			// this happens through door										1 A- D -B 2
			// in outer world door in room A (side a) should be used			1 A- D
			// in inner world door in room B (side b) should be used				 D -B 2
			// therefore we request that inside connector would be B
			//  so it has to be connected to A										 ? A- -B 2
			// that's why we need outer connector to be A (to connect to B)		1 A- D [ oA-  -B 2 ]
			//
			// example 2:
			// you have piece 1 and 2, connected					1 ----- 2
			// connector at 1 is side A, connector at 2 is side B	1 A- -B 2
			// now imagine that 1 is sub generator
			// (o will be outer connector, 3, 4 inside pieces)	[ 4 A- -B 3 ?- -?o ]1 A- -B 2
			// 4 is connected with 3 (A on 4, B on 3)
			// piece 2 will be connected in the end to 3
			// therefore 3 should have connector A on its side
			// that's why o should be B							[ 4 A- -B 3 A- -Bo ]1 A- -B 2
			// and finally we will get something like this			4 A- -B 3 A- -B 2
			//
			// it's just about what do you consider outer connector to be representing
			// connection to another element (example 2) or "connector" itself (example 1)


			int index = 0; // in allConnectorInstances
			typename CustomType::ConnectorInstanceData data;
			Optional<Name> inheritedConnectorTag;
			Optional<Name> inheritedProvideConnectorTag;

			OuterConnector() : connector(nullptr) {}
			OuterConnector(Connector<CustomType> const* _connector, ConnectorInstance<CustomType> const* _outerConnectorInstance, typename CustomType::ConnectorInstanceData const& _data, ConnectorSide::Type _side);
		};

		struct ProvidedPiece
		{
			RefCountObjectPtr<Piece<CustomType>> piece;
			int index = 0; // in allPieceInstances
			typename CustomType::PieceInstanceData data;

			ProvidedPiece() : piece(nullptr) {}
			ProvidedPiece(Piece<CustomType> const* _piece, typename CustomType::PieceInstanceData const& _data) : piece(_piece), data(_data) {}
		};

	public:
		Generator(GenerationRules<CustomType> const * _generationRules = nullptr);
		~Generator();

		void clear();

		bool has_piece_type(Piece<CustomType> const * _piece) const;
		void add_piece_type(Piece<CustomType> const * _piece, AddPieceTypeParams const & _params = AddPieceTypeParams());

		int add_piece(Piece<CustomType> const * _piece, typename CustomType::PieceInstanceData const & _data); // for provided pieces - returns index of provided piece
		int add_outer_connector(Connector<CustomType> const * _connector, ConnectorInstance<CustomType> const* _outerConnectorInstance, typename CustomType::ConnectorInstanceData const & _data, ConnectorSide::Type _side = ConnectorSide::Type::A); // for outer connectors - returns index for outer connector

		void output_setup(LogInfoContext & _log) const;

		OuterConnector * get_outer_connector(int _outerConnectorIndex);
		ConnectorInstance<CustomType> * get_outer_connector_instance(int _outerConnectorIndex);
		int get_outer_connector_count() const { return outerConnectors.get_size(); }

		Array<PieceInstance<CustomType>*> & access_all_piece_instances() { return allPieceInstances; }
		Array<ConnectorInstance<CustomType>*> & access_all_connector_instances() { return allConnectorInstances; }
		void set_parameters_provider(WheresMyPoint::IOwner* _wmpOwner) { parametersProvider = _wmpOwner; }
		WheresMyPoint::IOwner* get_parameters_provider() const { return parametersProvider; }

		void use_generation_rules(GenerationRules<CustomType> const * _generationRules);
		void use_update_and_validate(UpdateAndValidate _update_and_validate) { ext_update_and_validate = _update_and_validate; }
		void use_setup_piece_instance(SetupPieceInstance _setup_piece_instance) { ext_setup_piece_instance = _setup_piece_instance; }
		void use_should_use_connector(ShouldUseConnector _should_use_connector) { ext_should_use_connector = _should_use_connector; }

		GenerationResult::Type generate(Random::Generator * _randomGenerator = nullptr, int _tryCount = 15);

	public:
		typename CustomType::GenerationContext const & get_generation_context() const { return generationContext; }
		typename CustomType::GenerationContext & access_generation_context() { return generationContext; }

		DebugVisualiser<CustomType>* access_debug_visualiser() { return debugVisualiser.get(); }

	public:
		bool is_valid_step_call() const;
		void mark_failed();
		bool check_fail_count(); // true means we're fine

	public:
		bool are_both_in_same_cycle(PieceInstance<CustomType>* _pieceInstanceA, PieceInstance<CustomType>* _pieceInstanceB);
		bool are_both_in_same_cycle(ConnectorInstance<CustomType>* _connectorInstanceA, ConnectorInstance<CustomType>* _connectorInstanceB);
		void calculate_cycle_info(PieceInstance<CustomType>* _pieceInstance, OUT_ int & _cycleSize, OUT_ int & _outerConnectorCountInCycle, OUT_ bool & _canCreateAnything);

		GenerationRules<CustomType> const & get_generation_rules() const { return generationRules; }

	public:
		void step__add_piece_instance_to_all(PieceInstance<CustomType>* _pieceInstance);
		void step__add_connector_instance_to_all(ConnectorInstance<CustomType>* _connectorInstance);
		void step__remove_piece_instance_from_all(PieceInstance<CustomType>* _pieceInstance);
		void step__remove_connector_instance_from_all(ConnectorInstance<CustomType>* _connectorInstance);

	protected:
		int checkpoint();
		bool revert(); // revert to last checkpoint including step that is checkpoint
		bool revert_to(int _id); // revert to checkpoint identified by id

	protected:
		bool check_if_complete(ConnectorInstance<CustomType>* _forConnector = nullptr); // checks if all pieces and connectors can be accessed
		bool check_if_limits_are_broken() const; // check if we are fine against limits
		bool check_if_one_outer_connector_per_piece_for_piece_connections_is_compromised() const; // check if onlyOneConnectorToOuter is ok
		bool check_if_uniqueness_for_piece_connections_is_compromised() const; // check if eachConnectorHasUniquePiece is ok
		bool check_if_connector_groups_are_compromised() const; // check if samePieceConnectorGroup is ok
		bool check_if_there_are_impossible_generation_spaces() const; // check if there is any impossible
		bool check_if_there_are_separate_closed_cycles(bool _ignoreOptionals = false); // check if there are separate closed cycles - more than one
		bool check_if_there_is_fail_connection(); // check if any existing connector should fail generation
		void remove_all_not_needed_connectors();
		void define_side_for_all_connectors();

	protected: friend struct PieceInstance<CustomType>;
			   friend struct ConnectorInstance<CustomType>;
		void do_step(GenerationStep<CustomType>* _step);
		void revert_step(GenerationStep<CustomType>* _step);
		Random::Generator & access_random_generator() { return *randomGenerator; }
	private:
		int checkpointId;

		int loopsTaken;

		Random::Generator ownRandomGenerator;
		Random::Generator* randomGenerator = nullptr;
#ifdef PIECE_COMBINER_DEBUG
		bool workingWithSteps;
		int insideStep;
		int debugIdx;
#endif

		RefCountObjectPtr<DebugVisualiser<CustomType>> debugVisualiser; // always have pointer, it might be null though
		
		UpdateAndValidate ext_update_and_validate = nullptr;
		SetupPieceInstance ext_setup_piece_instance = nullptr;
		ShouldUseConnector ext_should_use_connector = nullptr;

		typename CustomType::GenerationContext generationContext;

		int failCount = 0;

		Optional<GenerationRules<CustomType>> generationRulesSource;
		GenerationRules<CustomType> generationRules; // all ranges turned into single values

		Array<UsePiece<CustomType> > pieces; // available pieces, they may have altered/overriden probability coef

		GenerationSpace<CustomType>* generationSpace;
		Array<GenerationStep<CustomType>*> steps;
		Array<OuterConnector> outerConnectors; // available outerConnectors
		Array<ProvidedPiece> providedPieces; // pieces provided by user - they have to be incorporated
		Array<PieceInstance<CustomType>*> allPieceInstances; // all (!) pieces
		Array<ConnectorInstance<CustomType>*> allConnectorInstances; // all (!) connectors

		WheresMyPoint::IOwner* parametersProvider = nullptr;

		void prepare_for_generation();

		int revert_checkpoint(); // returns number of last checkpoint

		GenerationResult::Type generate__turn_outer_connector_into_connector_instance(int _idx);
		GenerationResult::Type generate__turn_provided_piece_into_piece_instance(int _idx);
		GenerationResult::Type generate__generation_loop(GenerateLoopReason::Type _reason = GenerateLoopReason::Normal);
		GenerationResult::Type generate__try_connecting(ConnectorInstance<CustomType>* _tryConnector); // try to connect this connector to anything
		GenerationResult::Type generate__try_to_complete();
		GenerationResult::Type generate__try_to_complete_with(ConnectorInstance<CustomType>* _tryConnector);
		GenerationResult::Type generate__no_free_connectors();
		
		GenerationResult::Type apply_post_generation();

		void find_free_connectors(ArrayStatic< ConnectorInstance<CustomType>*,16 > & _bestConnectorInstances, FindFreeConnectorsReason::Type _reason);

#ifdef PIECE_COMBINER_DEBUG
		void validate_process__generation_space_aspect(); // check if everything is properly placed in generation spaces
#endif
#ifdef PIECE_COMBINER_VISUALISE
		void clear_visualise_issues();
#endif

		friend class DebugVisualiser<CustomType>;
	};

	template <typename CustomType>
	struct UsePiece
	{
		// definitors (we can have multiple piece types as long as they use multiple piece marks)
		RefCountObjectPtr<Piece<CustomType>> piece;
		Name canBeCreatedOnlyViaConnectorWithCreateNewPieceMark; // only connector that is set to create pieces marked in the same way
		// 
		Random::Number<int> const * limitCount = nullptr;
		Optional<float> probabilityCoef; // will override piece's probability coef
		Optional<float> mulProbabilityCoef; // always multiplies result (even from the above one)

		UsePiece();
		UsePiece(Piece<CustomType> const * _piece);

		void instantiate(Random::Generator& _generator);

		bool operator ==(UsePiece const & _other) const { return piece == _other.piece && canBeCreatedOnlyViaConnectorWithCreateNewPieceMark == _other.canBeCreatedOnlyViaConnectorWithCreateNewPieceMark; }

	private:
		int actualLimitCount = -1;

		friend class Generator<CustomType>;
	};

	/*
	 *	Piece is basic element for generation
	 *	Contains connectors that can be used to connect to other pieces on same level, internal connectors and parent/child (there can be only one parent)
	 *	Can map connectors to internal connectors
	 */
	template <typename CustomType>
	struct Piece
	: public RefCountObject
	{
	public:
		typename CustomType::PieceDef def;
	// TODO private?
		Array<Connector<CustomType>> connectors;
		float probabilityCoef = 1.0f; // chance of choosing
		Random::Number<int> limitCount;
		Optional<bool> onlyOneConnectorToOuter; // only one connector may connect to outer, if not provided, gets from the rules, internal/external is ignored
		bool eachConnectorHasUniquePiece = false; // each connector has to lead to unique piece - this is only for normal connections
		bool eachConnectorMayHaveNonUniquePieceForSingleOuterConnector = false; // if we have single outer connector uniqueness is ignored
		bool eachConnectorHasSamePiece = false; // each connector has to lead to same piece - this is only for normal connections
		bool preferCreatingNewPieces = false; // each connector will try to create new piece - this is only for normal connections
		bool preferEachConnectorHasUniquePiece = false; // prefer this, but allow to have same piece - this is only for normal connections
		bool disconnectIrrelevantConnectorsPostGeneration = false; // if set, doubled pieces or connecting to itself will be disconnected

		Piece();

		bool has_connectors() const { return ! connectors.is_empty(); }
		
		bool can_connect_with(Connector<CustomType> const * _with) const;

		bool load_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context = nullptr);
		bool load_plug_connectors_from_xml(IO::XML::Node const * _node, tchar const * _plugAttrName, tchar const * _plugNodeName, OUT_ bool * _loadedAnything = nullptr, std::function<bool(IO::XML::Node const * _node, Connector<CustomType> & _connector)> _load_custom = nullptr);
		bool load_socket_connectors_from_xml(IO::XML::Node const * _node, tchar const * _socketNodeName, OUT_ bool * _loadedAnything = nullptr, std::function<bool (IO::XML::Node const * _node, Connector<CustomType> & _connector)> _load_custom = nullptr);
		bool prepare(typename CustomType::PreparingContext * _context = nullptr);

		int add_connector(Connector<CustomType> const & _connector) { connectors.push_back(_connector); return connectors.get_size() - 1; }
		void make_connectors_block_each_other(Array<int> const & _indices);

		void set_probability(float _probability) { probabilityCoef = _probability; }
		float get_probability() const { return probabilityCoef; }

		int get_amount_of_randomisation_tries() const;

	private:
		bool load_something_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context);
		bool load_one_of_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context);
		bool load_grouped_connectors_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context);
		bool load_connector_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context);

		void validate_auto_id_for(Connector<CustomType>& _connector);
	};

	/*
	 *	Connector connecting instances
	 *
	 *	Special pairs (default behaviour unless specified otherwise):
	 *	 1.	Socket/Plug
	 *		This is most basic functionality for hierarchical generation - pieces through sockets create new pieces.
	 *		Socket requires and connects ONE plug. CAN create new pieces. (when loaded through socket connectors, they allow to fail, those methods are quite specific - check where they are used - that's why it is so)
	 *		Plug requires and connects to ONE socket. Can't create new pieces.
	 *	 2. Duplex
	 *		Not exactly a pair. Requires to connect to one of same type.
	 *		Requires and connects ONE other sibling. CAN create new pieces.
	 *		This may only connect to other duplex!
	 *	 3.	Provider/Requester/Refuse
	 *		Used to tell or require that something is available (eg. "we're green" room tells that and furniture knows whether they're allowed/needed or not)
	 *		Provider allows any number of requesters to connect (optional connectors). Can't create new pieces.
	 *		Requester requires and connects to ONE provider. Can't create new pieces.
	 *		Refuse is like requester but fails when connected.
	 *
	 *	If you want to mix plug/socket with duplex, do either <oneOf> or use socketDuplex/plugDuplex
	 *
	 *	On top of that, ConnectorTag may be used:
	 *	 1.	ConnectorTags on both connectors have to match (empty to non-empty don't match)
	 *	 2.	ProvideConnectorTag does not match but may define what connector instance has as a ConnectorTag (or may just carry ProvideConnectorTag further)
	 *		Both are defined with inheritConnectorTag and inheritProvideConnectorTag
	 *	 3.	ConnectorTags and provideConnectorTags are distributed via:
	 *		 a.	PieceInstances (all connector instances that inherit take them)
	 *			(provideConnectorTag makes piece instance created by it to have provideConnectorTag,
	 *			 then when connector instances are created they may inherit PCT or CT)
	 *		 b. outer connectors (again inheritance has to be explicitly ordered, CT and PCT are taken from a connector instance that is used as an outer)
	 */
	template <typename CustomType>
	struct Connector
	: public RefCountObject
	{
	public:
		typename CustomType::ConnectorDef def;
	// TODO private?
		ConnectorType::Type type;
		ConnectorSide::Type side = ConnectorSide::Any;
		Name name; // for identification only
		int autoId = NONE; // for internal blocking
		Name linkName; // pair of link name & link with is used to determine valid connections between connectors
		Name linkWith;
		Name linkNameAlt; // alternative version (with we want plug/socket to be used with duplex
		Name linkWithAlt; // if set, can be used as linkWith
		Name internalExternalName; // to connect internal/external
		Name outerName; // to know how to translate external (for upper level of generation) to outer connectors (in internal generation) outers are provided
		Name connectorTag; // connector tag is an extra condition - they have to match (empty does not match non-empty)
		Optional<Name> provideConnectorTag; // provides connector tag to whatever accepts it on the other side (most likely piece instance)
		bool inheritConnectorTag = false; // inherit when created or from outer (requires to be explicitly ordered)
		bool inheritProvideConnectorTag = false; // as above
		bool connectToOnlyIfNothingElse = false; // it will be chosen only if there are no other connectors
		bool requiresExternalConnected = false; // if internal/external and is internal, requires external to be connected before trying to connect
		bool allowConnectingToSelf = false;
		bool eachConnectorHasUniquePiece = false; // each connector has to lead to unique piece - this is only for normal connections
		bool eachConnectorMayHaveNonUniquePieceForSingleOuterConnector = false; // if we have single outer connector uniqueness is ignored
		Name samePieceConnectorGroup; // if connectors in a group should be connected to same piece
		bool preferCreatingNewPieces = false; // each connector will try to create new piece - this is only for normal connections
		bool preferEachConnectorHasUniquePiece = false; // prefer this, but allow to have same piece - this is only for normal connections
		bool failIfConnected = false; // issues failing if gets connected to anything
		Array<Name> blocks; // names of connectors that this one blocks
		Array<int> autoBlocks; // names of connectors that this one blocks
		bool canCreateNewPieces = false;
		Name createNewPieceMark; // out of pieces will only get these who have marked set in the same manner (useful when sharing connectors but we want only specific pieces to be created)
		bool allowFail = false; // allow failing connection
		float forceCreateNewPieceProbability = 0.0f; // even if not needed
		int forceCreateNewPieceLimit = 0; // if we exceeded limit, we won't create new piece
		float priority = 0; // priority when choosing from free/available to start connection
		float probabilityOfExistence = 1.0f; // probability at which this one exists
		float chooseChance = 1.0f; // probability at which this one is chosen
		float requiredAsOptionalProbability = 0.0f; // probability at which this one has required turned into optional
		Array<Name> requiresParametersToBeTrue; // checks among parameters_provoded
		Array<Name> requiresParametersToBeFalse;
		WheresMyPoint::Param<int> addRequired;
		Random::Number<int> requiredCount = Random::Number<int>(0); // has to have this number of actual connectors
		Random::Number<int> optionalCount = Random::Number<int>(0); // this is how many actual connectors it may have - they are used as connectors that only accept connections (* - well they can be optionally used as trying to connect)
																	// (total count of connectors is required + optional)
		Random::Number<int> totalCount = Random::Number<int>(0); // total limit for better control (if 0, there's no limit)

		Connector(ConnectorType::Type _type = ConnectorType::Unknown);

		bool check_required_parameters(Generator<CustomType>* _generator) const;

		bool can_connect_with(Connector<CustomType> const * _with) const;

		bool load_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context = nullptr);
		bool prepare(AN_NOT_CLANG_TYPENAME Piece<CustomType>* _piece, typename CustomType::PreparingContext * _context = nullptr);

	public:
		void make_required_single_only();
		void make_can_create_new_pieces(bool _canCreateNewPieces = true) { canCreateNewPieces = _canCreateNewPieces; }

		void set_link_ps_as_socket(tchar const * _link, bool _allowDuplex = false);
		void set_link_ps_as_plug(tchar const * _link, bool _allowDuplex = false);

		void set_link_ps_as_duplex(tchar const * _link);

		void set_link_ps_as_provider(tchar const * _link);
		void set_link_ps_as_requester(tchar const * _link);
		void set_link_ps_as_refuse(tchar const * _link);

		void be_normal_connector() { type = ConnectorType::Normal; }

		void set_probability_of_existence(float _probability) { probabilityOfExistence = _probability; }
		void set_choose_chance(float _chance) { chooseChance = _chance; }
		void allow_fail(bool _allowFail) { allowFail = _allowFail; }
		void fail_if_connected(bool _failIfConnected) { failIfConnected = _failIfConnected; }
	};

	/*
	 * Rules for generation, how many pieces max, when to start connecting to same, chance for using optional as normal, etc
	 */
	template <typename CustomType>
	struct GenerationRules
	{
	public:
		GenerationRules();

		float get_chance_to_use_optional_connector_as_normal() const { return chanceToUseOptionalConnectorAsNormal.get_min(); }
		int get_number_of_pieces_required_in_cycle_to_allow_connection_to_other_cycle() const { return numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle.get_min(); }
		bool should_allow_only_one_outer_connector_per_piece() const { return onlyOneConnectorToOuter; }
		bool should_allow_connecting_provided_pieces() const { return allowConnectingProvidedPieces; }
		int get_piece_limit() const { return pieceLimit.get_min(); }
		bool should_treat_parents_and_children_as_parts_of_cycle() const { return treatParentsAndChildrenAsPartsOfCycle; }

		bool load_from_xml(IO::XML::Node const * _node, typename CustomType::LoadingContext * _context = nullptr);

	protected:
		Random::Number<float> chanceToUseOptionalConnectorAsNormal; // chance (per generation_loop) to use optional connector as normal one
		Random::Number<int> numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle; // how many pieces are required to connect two separate cycles
		bool onlyOneConnectorToOuter; // this is also automatically working if numberOfPiecesRequiredInCycleToAllowConnectionToOtherCycle is non-zero
		bool allowConnectingProvidedPieces;
		Random::Number<int> pieceLimit; // pieces limit
		bool treatParentsAndChildrenAsPartsOfCycle;

	private:
		void instantiate(Optional<GenerationRules> _rules, Random::Generator& _generator);

		friend class Generator<CustomType>;
	};

	/*
	 * Generation space is inside each piece instance that can map connectors to internal connectors
	 */
	template <typename CustomType>
	struct GenerationSpace
	: public SoftPooledObject<GenerationSpace<CustomType> >
	{
	public:
		GenerationSpace();

		bool is_main_generation_space() const { return owner == nullptr; }

	protected: friend class Generator<CustomType>;	
		bool check_if_impossible() const;
		
	protected: friend class GenerationSteps::CreatePieceInstance<CustomType>;
			   friend struct PieceInstance<CustomType>;
		void step__add_piece(Generator<CustomType>* _generator, PieceInstance<CustomType>* _pieceInstance);
		void step__remove_piece(Generator<CustomType>* _generator, PieceInstance<CustomType>* _pieceInstance);

	protected: friend class GenerationSteps::CreateConnectorInstance<CustomType>;
			   friend struct ConnectorInstance<CustomType>;
		void step__add_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance);
		void step__remove_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance);

	protected: // SoftPooledObject
			   friend class SoftPooledObject<GenerationSpace<CustomType> >;
		override_ void on_get();
		override_ void on_release();

	protected: friend struct PieceInstance<CustomType>;
		void set_owner(PieceInstance<CustomType>* _owner);

	protected: friend struct ConnectorInstance<CustomType>;
		void step__block_non_owned_with(Generator<CustomType>* _generator, Connector<CustomType> const* _connector);
		void step__unblock_non_owned_with(Generator<CustomType>* _generator, Connector<CustomType> const* _connector);

	private:
		PieceInstance<CustomType>* owner;
		Array<PieceInstance<CustomType>*> pieces;
		Array<ConnectorInstance<CustomType>*> connectors;

		void clear();

		friend class DebugVisualiser<CustomType>;
	};

	template <typename CustomType>
	struct BuildCycleContext
	{
		Generator<CustomType>* generator = nullptr;
		int cycleId = 0;
		bool ignoreOptionals = false;
	};

	template <typename CustomType>
	struct PieceInstance
	: public RefCountObject
	, public SoftPooledObject<PieceInstance<CustomType> >
	{
#ifdef PIECE_COMBINER_VISUALISE
	public:
		bool visualiseIssue = false;
#endif
	public:
		Piece<CustomType> const * get_piece() const { return piece.get(); }
		GenerationSpace<CustomType>* get_in_generation_space() const { return inGenerationSpace; }
		GenerationSpace<CustomType>* get_generation_space() const { return generationSpace; }
		Array<ConnectorInstance<CustomType>*> const & get_owned_connectors() const { return ownedConnectors; }
		
		bool was_created_from_provided_data() const { return createdFromProvidedData; }

		typename CustomType::PieceInstanceData const & get_data() const { return data; }
		typename CustomType::PieceInstanceData & access_data() { return data; }

	public:
		bool check_if_would_be_connected_as_unique_piece(PieceInstance<CustomType> const * _otherPiece) const;

	protected: friend class SoftPooledObject<PieceInstance<CustomType> >;
			   friend class SoftPool<PieceInstance<CustomType> >;
		PieceInstance();
		~PieceInstance();

	protected: friend class GenerationSteps::CreatePieceInstance<CustomType>;
		void step__setup(Generator<CustomType>* _generator, Piece<CustomType> const * _piece, ConnectorInstance<CustomType> const* _createdByConnectorInstance);

	protected: friend class GenerationSteps::CreateConnectorInstance<CustomType>;
		void step__add_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance);
		void step__remove_connector(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connectorInstance);

	protected: friend class Generator<CustomType>;
		GenerationResult::Type generate__try_connecting(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _tryConnector);

	protected: friend struct ConnectorInstance<CustomType>;
		void step__block_with(Generator<CustomType>* _generator, Connector<CustomType> const * _connector);
		void step__unblock_with(Generator<CustomType>* _generator, Connector<CustomType> const * _connector);

	protected: friend class GenerationSteps::MoveToSpace<CustomType>;
		void move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace);
		void step__move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace);

	protected: // RefCountObject
		override_ void destroy_ref_count_object() { SoftPooledObject<PieceInstance<CustomType> >::release(); }

	protected: // SoftPooledObject
		override_ void on_get();
		override_ void on_release();

	protected: friend class Generator<CustomType>;
		void check_if_complete();
		void check_if_complete_as_parent_child();
		void clear_accessibility() { isAccessible = false; isAccessibleAsParentChild = false; }
	public:
		bool was_accessed() const { return isAccessible || isAccessibleAsParentChild; }
		bool was_accessed_through_same_level() const { return isAccessible; } // not as parent/child

	protected: // friend class Generator<CustomType>;
		bool build_cycle(BuildCycleContext<CustomType> const & _context, REF_ int * _piecesInCycleCount = nullptr, REF_ int * _outerConnectorCountInCycle = nullptr, REF_ bool * _canCreateAnything = nullptr); // build cycle - return false if it is closed
		void build_cycle_as_parent_child(BuildCycleContext<CustomType> const& _context, int _dir = 0);
		void clear_cycle() { cycleId = 0; cycleIdAsParentChild = 0; }
		bool is_in_cycle() const { return cycleId != 0; }
		bool is_part_of_cycle() const { return cycleId != 0 || cycleIdAsParentChild != 0; }
		int get_cycle_id() const { return cycleId; }

	protected: friend struct GenerationSpace<CustomType>;
		void on_generation_space_clear(GenerationSpace<CustomType>* _generationSpace);

	private:
		RefCountObjectPtr<Piece<CustomType>> piece;
#ifdef PIECE_COMBINER_DEBUG
		String desc;
#endif
		typename CustomType::PieceInstanceData data;
		GenerationSpace<CustomType>* inGenerationSpace; // set with add/remove of generation space
		GenerationSpace<CustomType>* generationSpace; // internal generation space
		Array<ConnectorInstance<CustomType>*> ownedConnectors; // owned connectors (that originate in this piece)

		Name connectorTag; // from connector data (via which we were created)
		Optional<Name> provideConnectorTag;
		bool createdFromProvidedData;
		bool isAccessible; // is accessible from starting piece (or whatever else, set during check for completion)
		bool isAccessibleAsParentChild;
		int cycleId;
		int cycleIdAsParentChild;

#ifdef AN_DEVELOPMENT
	private: friend class DebugVisualiser<CustomType>;
		Vector2 dvLocation = Vector2::zero; // always relative to centre of generation space (local, important for generation spaces inside other pieces)
		Vector2 dvMove = Vector2::zero; // movement calculated
		float dvRadius = 1.0f;
		float dvInnerRadius = 1.0f; // not scaled
		float dvInnerScale = 1.0f;
#endif
	};

	template <typename CustomType>
	struct ConnectorInstance
	: public RefCountObject
	, public SoftPooledObject<ConnectorInstance<CustomType> >
	{
#ifdef PIECE_COMBINER_VISUALISE
	public:
		bool visualiseIssue = false;
#endif
	public:
		bool is_available() const { return connectedTo == nullptr && blockCount == 0 && ! isNoConnector; }
		bool is_connected() const { return connectedTo != nullptr; }
		bool is_optional() const { return isOptional; }
		bool is_outer() const { return isOuter; }
		bool is_blocked() const { return blockCount != 0; }
		bool is_no_connector() const { return isNoConnector; }
		bool is_internal_external() const { return internalExternalConnector != nullptr; }
		bool is_in_main_generation_space() const { return inGenerationSpace->is_main_generation_space(); }
		Name const & get_connector_tag() const { return connectorTag; }
		Optional<Name> const & get_provide_connector_tag() const { return provideConnectorTag; }
		Connector<CustomType> const * get_connector() const { return connector; }
		GenerationSpace<CustomType> * get_in_generation_space() const { return inGenerationSpace; }
		ConnectorInstance* get_internal_external() const { return internalExternalConnector; }
		bool can_connect_with(ConnectorInstance<CustomType> const * _with) const;
		ConnectorSide::Type get_side() const { return side; }

		bool was_created_from_provided_data() const { return createdFromProvidedData; }

		PieceInstance<CustomType>* get_owner() const { return owner; }
		ConnectorInstance<CustomType>* get_connector_on_other_end() const;
		PieceInstance<CustomType>* get_owner_on_other_end() const;

		void mark_pieces_on_the_way_to_the_other_end_part_of_cycle(int _cycleId);

		bool should_prefer_creating_new_pieces() const { return (connector && connector->preferCreatingNewPieces) || (owner && owner->piece.get() && owner->piece->preferCreatingNewPieces); }
		bool should_prefer_each_connector_has_unique_piece() const { return (connector && connector->preferEachConnectorHasUniquePiece) || (owner && owner->piece.get() && owner->piece->preferEachConnectorHasUniquePiece); }

		typename CustomType::ConnectorInstanceData const & get_data() const { return data; }
		typename CustomType::ConnectorInstanceData & access_data() { return data; }

		void connect_with(Generator<CustomType>* _generator, ConnectorInstance<CustomType>* _connector);

	public:
		void post_generation__disconnect(); // disconnect without step, due to extra rules

	protected: friend class SoftPooledObject<ConnectorInstance<CustomType> >;
			   friend class SoftPool<ConnectorInstance<CustomType> >;
		ConnectorInstance();

	protected: // RefCountObject
		override_ void destroy_ref_count_object() { SoftPooledObject<ConnectorInstance<CustomType> >::release(); }

	protected: // SoftPooledObject
		override_ void on_get();
		override_ void on_release();

	protected: friend class GenerationSteps::CreateConnectorInstance<CustomType>;
	   void step__setup(Generator<CustomType>* _generator, Connector<CustomType> const * _connector, PieceInstance<CustomType>* _owner, Optional<Name> const & _inheritConnectorTag, Optional<Name> const& _inheritProvideConnectorTag);
	   void step__be_optional(Generator<CustomType>* _generator);
	   void step__be_infinite(Generator<CustomType>* _generator);

	protected: friend class GenerationSteps::ConnectInternalExternal<CustomType>;
	   void step__connect_internal_external(Generator<CustomType>* _generator, ConnectorInstance* _with);
	   void step__disconnect_internal_external(Generator<CustomType>* _generator);

	protected: friend class GenerationSteps::Connect<CustomType>;
			   friend class GenerationSteps::Disconnect<CustomType>;
	   void step__connect(Generator<CustomType>* _generator, ConnectorInstance* _with);
	   void step__disconnect(Generator<CustomType>* _generator);

	protected: friend class GenerationSteps::BlockWith<CustomType>;
	   void step__block(Generator<CustomType>* _generator);
	   void step__unblock(Generator<CustomType>* _generator);

	protected: friend class GenerationSteps::BlockSingle<CustomType>;
	   void step__mark_blocked(Generator<CustomType>* _generator, bool _blockInternalExternal = true);
	   void step__mark_unblocked(Generator<CustomType>* _generator, bool _unblockInternalExternal = true);

	protected: friend struct PieceInstance<CustomType>;
		void move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace);

	protected: // piece instance and generator friend
		bool build_cycle(BuildCycleContext<CustomType> const& _context, REF_ int * _piecesInCycleCount, REF_ int * _outerConnectorCountInCycle, REF_ bool * _canCreateAnything);

	protected: friend class GenerationSteps::MoveToSpace<CustomType>;
		void step__move_to_space(Generator<CustomType>* _generator, GenerationSpace<CustomType>* _toGenerationSpace);

	protected: friend class Generator<CustomType>;
		void define_side();

	protected: friend struct GenerationSpace<CustomType>;
		void on_generation_space_clear(GenerationSpace<CustomType>* _generationSpace);

	private:
		Connector<CustomType> const * connector;
#ifdef PIECE_COMBINER_DEBUG
		String desc;
#endif
		typename CustomType::ConnectorInstanceData data;
		PieceInstance<CustomType> * owner;
		GenerationSpace<CustomType> * inGenerationSpace; // set with add/remove
		Name connectorTag; // from connector data or inherited
		Optional<Name> provideConnectorTag; // from connector data
		bool createdFromProvidedData;
		// this is shared amongst all using same connector
		bool isOuter; // was provided during generation setup? it could be provided from upper level of generation or is just some socket
		bool isInfinite;
		bool isOptional; // if optional, doesn't have to be connected - it works more like an entrance that accepts incoming connector - for connectors that have max set to infinity there's always one connector extra
		bool isNoConnector;
		ConnectorSide::Type side; // defined at the very end, when generation has finished
		int blockCount; // block count
		bool shouldNotExist;

		ConnectorInstance* connectedTo;
		ConnectorInstance* internalExternalConnector; // to connect internal with external

		void on_connected(Generator<CustomType>* _generator);

#ifdef AN_DEVELOPMENT
	private: friend class DebugVisualiser<CustomType>;
		Vector2 dvLocation = Vector2::zero; // if has owner, this is calculated as owner's location + radius at angle
		Vector2 dvMove = Vector2::zero; // movement calculated
		float dvAngle = 0.0f;
#endif
	};

	template <typename CustomType>
	class GenerationStep
	{
		FAST_CAST_DECLARE(GenerationStep);
		FAST_CAST_END();
	public:
		GenerationStep();
		virtual void execute(Generator<CustomType>* _generator) = 0;
		virtual void revert(Generator<CustomType>* _generator) = 0;
	protected:
		virtual ~GenerationStep() {}

		friend class Generator<CustomType>;
	};

};
