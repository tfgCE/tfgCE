#pragma once

#include "..\..\..\core\functionParamsStruct.h"

#include "..\roomGeneratorInfo.h"
#include "..\..\library\libraryStored.h"
#include "..\..\library\usedLibraryStoredOrTagged.h"

namespace Framework
{
	class EnvironmentType;
	class Mesh;
	class ObjectLayout;
	class Region;
	class Room;
	class WallLayout;

	struct RoomLayoutContext
	{
		TagCondition roomContentTagged;
	};

	/**
	 *	Room layout is a layout for rooms with elements that can be replaced
	 */
	class RoomLayout
	: public LibraryStored
	{
		FAST_CAST_DECLARE(RoomLayout);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		RoomLayout(Library * _library, LibraryName const & _name);

		Random::Number<int> const& get_door_count() const { return doorCount.count; }
		int get_max_total_door_count() const { return doorCount.maxTotal; }

	public:
		struct CreateRoomParams
		{
			ADD_FUNCTION_PARAM(CreateRoomParams, Random::Generator, randomGenerator, use_random_generator);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(CreateRoomParams, Region*, region, in_region, nullptr);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(CreateRoomParams, SubWorld*, subWorld, in_sub_world, nullptr);
			ADD_FUNCTION_PARAM_PLAIN(CreateRoomParams, Tags, roomContentTagsRequired, with_room_content_required_tags);
		};
		Room* async_create_room(CreateRoomParams const & _params);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ bool setup_custom_preview_room(SubWorld* _testSubWorld, REF_ Room*& _testRoomForCustomPreview, Random::Generator const & _rg);

		override_ void debug_draw() const {}

	protected:
		virtual ~RoomLayout();

	private:
		Tags roomTags;
		Vector2 roomSize = Vector2(4.0f, 4.0f);
		float roomHeight = 2.8f;
		
		struct DoorCount
		{
			Random::Number<int> count = Random::Number<int>(1); // this is the count we want to have, has to be at least 1
			int maxTotal = 0; // this is max we allow, if 0, will assume required count
		} doorCount;

		Tags roomContentTagsRequired;	// if we choose stuff that is inside the room by tags, we first try to select with additional help of this tag condition
										// this way we can select different content of shelves depending on whether a shelve is inside a bedroom or in a store

		List<UsedLibraryStoredOrTagged<EnvironmentType>> environmentTypes;
		List<UsedLibraryStoredOrTagged<Mesh>> floorTileMeshes;

		List<UsedLibraryStoredOrTagged<ObjectLayout>> objectLayout;

		// X/Y Minus/Plus
		// clockwise starting with YPlus
		List<UsedLibraryStoredOrTagged<WallLayout>> wallLayoutYP;
		List<UsedLibraryStoredOrTagged<WallLayout>> wallLayoutXP;
		List<UsedLibraryStoredOrTagged<WallLayout>> wallLayoutYM;
		List<UsedLibraryStoredOrTagged<WallLayout>> wallLayoutXM;

		// similar to wall but only for left corner, we also start in the corner
		List<UsedLibraryStoredOrTagged<ObjectLayout>> leftCornerLayoutYP;
		List<UsedLibraryStoredOrTagged<ObjectLayout>> leftCornerLayoutXP;
		List<UsedLibraryStoredOrTagged<ObjectLayout>> leftCornerLayoutYM;
		List<UsedLibraryStoredOrTagged<ObjectLayout>> leftCornerLayoutXM;

	private:
		// this class works as an external interface for room generator info
		class RoomGeneratorInterface
		: public RoomGeneratorInfo // limited functionality, this is used solely for generating a room
		{
			FAST_CAST_DECLARE(RoomLayout);
			FAST_CAST_BASE(RoomGeneratorInfo);
			FAST_CAST_END();

		public:
			RoomGeneratorInterface(RoomLayout* _roomLayout);
			virtual ~RoomGeneratorInterface();

			void clear_room_layout();

		public: // RoomGeneratorInfo
			/* not to be used */ implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			/* not to be used */ implement_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			/* not to be used */ implement_ bool apply_renamer(LibraryStoredRenamer const& _renamer, Library* _library = nullptr);
			/* not to be used */ implement_ RoomGeneratorInfoPtr create_copy() const;
			//
			/* not to be used */ implement_ RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> generate_piece_for_region_generator(REF_ PieceCombiner::Generator<RegionGenerator>& _generator, Region* _region, REF_ Random::Generator& _randomGenerator) const;

			implement_ bool generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const;
			implement_ bool post_generate(Room* _room, REF_ RoomGeneratingContext& _roomGeneratingContext) const;

		private:
			RoomLayout* roomLayout = nullptr;
		};

		friend class RoomGeneratorInterface;
		RefCountObjectPtr<RoomGeneratorInterface> roomGeneratorInterface;

		bool generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const;
		bool post_generate(Room* _room, REF_ RoomGeneratingContext& _roomGeneratingContext) const;
	};
};
