#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGenerationInfo.h"

#include "..\roomGeneratorBase.h"

class SimpleVariableStorage;

namespace Framework
{
	class Actor;
	class DoorInRoom;
	class Game;
	class MeshGenerator;
	class Room;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class SimpleCorridor;

		class SimpleCorridorInfo
		: public Base
		{
			FAST_CAST_DECLARE(SimpleCorridorInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			SimpleCorridorInfo();
			virtual ~SimpleCorridorInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			Framework::MeshGenParam<Framework::LibraryName> corridorMeshGenerator;
			Framework::MeshGenParam<Range> sameDirMaxTileDistForIPs; // should be range

			friend class SimpleCorridor;
		};

		class SimpleCorridor
		{
		public:
			SimpleCorridor(SimpleCorridorInfo const * _simpleCorridorInfo);
			~SimpleCorridor();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			SimpleCorridorInfo const * info;
		};

	};

};
