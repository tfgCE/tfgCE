#pragma once

#include "layoutElement.h"

#include "..\..\..\core\functionParamsStruct.h"

#include "..\roomGeneratorInfo.h"
#include "..\..\library\libraryStored.h"
#include "..\..\library\usedLibraryStoredOrTagged.h"

namespace Framework
{
	class EnvironmentType;
	class Mesh;
	class Region;
	class Room;
	class WallLayout;
	struct RoomLayoutContext;

	struct WallLayoutFixedDoorPlacement
	{
		float at = 0.0f; // if not provided, zero means it is in the centre

		void offset_by(Transform const& _onWallPlacement);

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
	};

	struct WallLayoutDisallowedDoorPlacement
	{
		Range range = Range::empty;

		void offset_by(Transform const& _onWallPlacement);

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
	};

	struct WallLayoutElement
	: public LayoutElement
	{
		typedef LayoutElement base;

		UsedLibraryStoredOrTagged<WallLayout> includeWallLayout;
		UsedLibraryStoredOrTagged<ObjectLayout> includeObjectLayout;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
	};

	namespace WallLayoutElementFlags
	{
		enum Flags
		{
			ActualWall = 0x01
		};
	};

	struct WallLayoutPrepared
	{
		Transform wallPlacement; // 0,0,0 - wall centre, Y+ outwards, so it is like we are looking at the wall from the centre of the room
		Range wallRange;
		Array<LayoutElementMeshInstance> meshes;
		Array<WallLayoutDisallowedDoorPlacement> disallowedDoorPlacements;
		Array<WallLayoutFixedDoorPlacement> fixedDoorPlacements; // if any provided, disallow door ranges are not used to find door placement (although they may still be used to invalidate door placement)
		bool usesFixedDoorPlacements = false; // fixedDoorPlacements will be removed, therefore we need to use this variable to know that there's no place left

		WallLayoutPrepared() { clear(); }

		void clear()
		{
			wallPlacement = Transform::identity;
			wallRange = Range::empty;
			meshes.clear();
			disallowedDoorPlacements.clear();
			fixedDoorPlacements.clear();
		}

		void prepare_empty(Transform const& _wallPlacement, float _wallLength)
		{
			clear();
			wallPlacement = _wallPlacement;
			wallRange = Range(-_wallLength * 0.5f, _wallLength * 0.5f);
		}

		void unify_disallowed_door_placements(); // to avoid overlapping ranges
	};

	/**
	 */
	class WallLayout
	: public LibraryStored
	{
		FAST_CAST_DECLARE(WallLayout);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		WallLayout(Library * _library, LibraryName const & _name);

		void async_prepare_layout(RoomLayoutContext const& _context, Room* _room, Transform const& _wallPlacement, float _wallLength, REF_ Random::Generator& _rg, OUT_ WallLayoutPrepared& _wallLayoutPrepared) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~WallLayout();

	private:
		Array<WallLayoutDisallowedDoorPlacement> disallowedDoorPlacements;
		Array<WallLayoutFixedDoorPlacement> fixedDoorPlacements;

		List<WallLayoutElement> elements;

		void async_prepare_layout_add_elements(RoomLayoutContext const& _context, Transform const& _onWallPlacement, float _wallLength, REF_ Random::Generator& _rg, OUT_ WallLayoutPrepared& _wallLayoutPrepared) const;

	};
};
