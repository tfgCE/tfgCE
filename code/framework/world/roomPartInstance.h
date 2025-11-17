#pragma once

#include "room.h"
#include "roomPieceCombinerGenerator.h"
#include "..\..\core\mesh\mesh3dInstanceSet.h"
#include "..\..\core\collision\modelInstanceSet.h"
#include "roomPart.h"
#include "pointOfInterestTypes.h"
#include "pointOfInterestInstance.h"

namespace Framework
{
	class Room;
	class RoomPart;
	class Mesh;

	/**
	 *	?
	 */
	class RoomPartInstance
	{
	public:
		RoomPartInstance(RoomPart const * _roomPart);
		~RoomPartInstance();

		void for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _fpoi, Room const * _room, IsPointOfInterestValid _is_valid = nullptr) const;
		Array<PointOfInterestInstancePtr> const & get_pois() const { return pois; }

		RoomPart * get_room_part() const { return roomPart.get(); }

	public: // debug
		void debug_draw_pois(bool _setDebugContext);
		void debug_draw_mesh_nodes();

	protected: friend class Room;
		void place(Room * _insideRoom, Transform const & _placement);

	private:
		Room * insideRoom;
		UsedLibraryStored<RoomPart> roomPart;
		Transform placement; // if this ever changes, every poi should be updated!

		struct Element
		{
			Mesh* mesh;
			Meshes::Mesh3DInstance* appearance;
			int movementCollisionId;
			int preciseCollisionId;
			Array<PointOfInterestInstancePtr> pois;
			Element();
			~Element();
			void remove_from(Room * _room);
			void place(Room * _insideRoom, Transform const & _placement, RoomPart::Element const & _element);
		};

		Array<Element> elements;
		Array<PointOfInterestInstancePtr> pois;
	};
};
