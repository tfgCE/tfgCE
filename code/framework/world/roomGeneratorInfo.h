#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\simpleVariableStorage.h"

#include "..\appearance\meshNode.h"
#include "..\library\libraryName.h"
#include "..\library\usedLibraryStored.h"
#include "..\meshGen\meshGenParam.h"

#include "roomGeneratorInfoSet.h"

#include <functional>

struct String;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class DoorTypeReplacer;
	class Library;
	class Room;
	class RoomGeneratorInfo;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;
	struct LibraryStoredRenamer;

	template<typename Class> struct MeshGenParam;

	struct RoomGeneratingContext
	{
		Array<MeshNodePtr> meshNodes;
		Range3 spaceUsed = Range3::empty; // might be used by some generators to place everything properly, this is more like suggestion rather than precise info
	};

	class RoomGeneratorInfo
	: public RefCountObject
	{
		FAST_CAST_DECLARE(RoomGeneratorInfo);
		FAST_CAST_END();

	public:
		typedef std::function< RoomGeneratorInfo* ()> Construct;

		static void register_room_generator_info(String const & _name, RoomGeneratorInfo::Construct _func);
		static void initialise_static();
		static void close_static();
		static RoomGeneratorInfo* create(String const & _name);

		// variable name should be library name
		template <typename Class>
		static Class* find(SimpleVariableStorage const & _variables, MeshGenParam<LibraryName> const & _value);
		// first tries variables: specific then general, then fallback specific and general, useful to first try horizontal/vertical then common
		template <typename Class>
		static Class* find(SimpleVariableStorage const & _variables, MeshGenParam<LibraryName> const & _valueSpecific, MeshGenParam<LibraryName> const & _valueGeneral);

		// variable name should be library name
		template <typename Class>
		static Class* find(SimpleVariableStorage const & _variables, Name const & _variableName, Class* _fallback = nullptr);
		// first tries variables: specific then general, then fallback specific and general, useful to first try horizontal/vertical then common
		template <typename Class>
		static Class* find(SimpleVariableStorage const & _variables, Name const & _variableNameSpecific, Name const & _variableNameGeneral, Class* _fallbackSpecific = nullptr, Class* _fallbackGeneral = nullptr);

	public:
		RoomGeneratorInfo() {}
		virtual ~RoomGeneratorInfo() {}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		String const& get_name() const { return name; }
#endif

		virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc) = 0;
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext) = 0;
		virtual bool apply_renamer(LibraryStoredRenamer const & _renamer, Library* _library = nullptr) = 0;
		virtual RoomGeneratorInfoPtr create_copy() const = 0;

		virtual RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region* _region, REF_ Random::Generator & _randomGenerator) const; // this should allow to generate custom pieces with variable door counts etc
		virtual bool generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext & _roomGeneratingContext) const = 0; // should mark room vr arranged/mesh generated
		virtual bool post_generate(Room* _room, REF_ RoomGeneratingContext& _roomGeneratingContext) const; // manage environments, emissives ets

		bool apply_generation_parameters_to(Room* _room) const;
		bool apply_generation_parameters_to(SimpleVariableStorage & _variables) const;
		SimpleVariableStorage const & get_generation_parameters() const { return generationParameters; }

		virtual bool requires_external_vr_placement() const { return false; } // by default rooms provide their own placement for vr
		int get_room_generator_priority() const { return roomGeneratorPriority; }

		DoorTypeReplacer * get_door_type_replacer(Room* _room) const;

	protected:
		void set_room_generator_priority(int _priority) { roomGeneratorPriority = _priority; }

	private:
#ifdef AN_DEVELOPMENT_OR_PROFILER
		String name; // set on create()
#endif
		int roomGeneratorPriority = 0; // this is generation priority, when there is a bunch of rooms to be generated, they are chosen basing on generation priority, then the order they were added (starting with the starting room)
		SimpleVariableStorage generationParameters; // parameters set to room on "generate"

		MeshGenParam<LibraryName> doorTypeReplacer; // does not have to be set, will override_ door replacer defined within room type

		Name placeDoorOnPOI; // places door on POI, currently just single door is handled

	};
};

