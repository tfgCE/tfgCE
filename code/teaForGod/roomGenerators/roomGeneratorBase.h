#pragma once

#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\world\roomGeneratorInfo.h"

#include "..\ai\aiManagerInfo.h"

namespace Framework
{
	class ObjectType;
};

namespace TeaForGodEmperor
{
	namespace RoomGenerators
	{
		struct SpawnObjectInfo
		{
			Framework::LibraryName objectTypeName;
			Array<Framework::UsedLibraryStored<Framework::ObjectType>> objectTypes;
			TagCondition objectTypesTagged;
			Framework::MeshGenParam<int> amount = 1;
			Tags tagSpawned;
			Transform placement = Transform::identity;
			Name placeAtPOI;
			bool shareMesh = false;
			int shareMeshLimit = 0;
			
			enum WhenToSpawn
			{
				OnRoomGeneration,
				ViaDelayedObjectCreation
			} whenToSpawn = OnRoomGeneration;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		};

		/**
		 *	Base for all actual room generators
		 *	Has some common additional things that might be used by normal generators, eg:
		 *		Background (that will use doors or actors or some variables stored in room)
		 *	Generators should use variables collected from rooms, that means that those are variables taken from actual rooms, room types (custom parameters), regions (and types).
		 */
		class Base
		: public Framework::RoomGeneratorInfo
		{
			FAST_CAST_DECLARE(Base);
			FAST_CAST_BASE(Framework::RoomGeneratorInfo);
			FAST_CAST_END();

			typedef RoomGeneratorInfo base;
		public:
			Base();
			virtual ~Base();

			float get_wall_offset() const { return wallOffset; }

			AI::ManagerInfo const & get_ai_managers() const { return aiManagers; }

		public: // RoomGeneratorInfo
			implement_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			implement_ bool apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library = nullptr);
			implement_ Framework::RoomGeneratorInfoPtr create_copy() const;

			implement_ bool generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;
			implement_ bool post_generate(Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

		protected:
			virtual bool internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const = 0;

		private:
			Framework::RoomGeneratorInfoSet backgroundRoomGeneratorInfo;

			bool chooseRandomBackgroundRoomGenerator = false; // if false, will include all background room generators

			Framework::MeshGenParam<bool> setEmissiveFromEnvironment; // by default we look for setEmissiveFromEnvironment bool variable
			Framework::MeshGenParam<bool> allowEmissiveFromEnvironment; // by default we look for allowEmissiveFromEnvironment bool variable
			Framework::MeshGenParam<bool> zeroEmissive; // remove all emissive (uses zeroEmissive bool variable)

			AI::ManagerInfo aiManagers;

			float wallOffset = -0.01f; // offset to allow doors to be placed in front of the walls

			Array<SpawnObjectInfo> spawnObjectsInfo;

			void spawn_objects(Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const;

			friend class PlatformsLinear;
		};
	};

};
