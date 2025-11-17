#pragma once

#include "layoutElement.h"
#include "layoutSocket.h"

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
	struct RoomLayoutContext;

	struct ObjectLayoutElement
	: public LayoutElement
	{
		typedef LayoutElement base;

		UsedLibraryStoredOrTagged<ObjectLayout> includeObjectLayout;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
	};

	struct ObjectLayoutPrepared
	{
		Array<LayoutElementMeshInstance> meshes;

		ObjectLayoutPrepared() { clear(); }

		void clear()
		{
			meshes.clear();
		}
	};

	/**
	 */
	class ObjectLayout
	: public LibraryStored
	{
		FAST_CAST_DECLARE(ObjectLayout);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		ObjectLayout(Library * _library, LibraryName const & _name);

		void async_prepare_layout(RoomLayoutContext const& _context, Room* _room, Transform const& _inRoomPlacement, REF_ Random::Generator& _rg, OUT_ ObjectLayoutPrepared& _objectLayoutPrepared) const;

		Optional<Transform> find_socket_placement(Name const& _socketName) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~ObjectLayout();

	private:
		List<ObjectLayoutElement> elements;

		Array<LayoutSocket> sockets; // can be only used for "usingSocket" now
		Array<UsedLibraryStored<Mesh>> copySocketsFrom;

		void async_prepare_layout_add_elements(RoomLayoutContext const& _context, Transform const& _placement, REF_ Random::Generator& _rg, OUT_ Array<LayoutElementMeshInstance>& _toMeshes) const;

		friend class WallLayout;
	};
};
