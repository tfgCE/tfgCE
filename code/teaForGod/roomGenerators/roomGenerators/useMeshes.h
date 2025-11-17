#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\random\random.h"

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\world\roomGeneratorInfo.h"

#include "..\roomGenerationInfo.h"

#include "..\roomGeneratorBase.h"

class SimpleVariableStorage;

namespace Framework
{
	class Actor;
	class DoorInRoom;
	class Game;
	class Mesh;
	class Room;
	class SceneryType;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		class UseMeshes;

		/**
		 *	Can be used as a stand alone room, that's why it uses Base and is a room generator (not just room element)
		 */
		class UseMeshesInfo
		: public Base
		{
			FAST_CAST_DECLARE(UseMeshesInfo);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;
		public:
			UseMeshesInfo();
			virtual ~UseMeshesInfo();

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			struct UseMesh
			{
				Framework::UsedLibraryStored<Framework::Mesh> mesh;
				Transform placement = Transform::identity;
			};
			Array<UseMesh> useMeshes;
			struct UseScenery
			{
				Framework::UsedLibraryStored<Framework::SceneryType> sceneryType;
				Transform placement = Transform::identity;
			};
			Array<UseScenery> useSceneries;

			friend class UseMeshes;
		};

		class UseMeshes
		{
		public:
			UseMeshes(UseMeshesInfo const * _info);
			~UseMeshes();

			bool generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext);

		private:
			UseMeshesInfo const * info = nullptr;
		};

	};

};
