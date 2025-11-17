#pragma once

#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\math\math.h"

#include "lightSourceProxy.h"

namespace Meshes
{
	struct Mesh3DRenderingBuffer;
	struct Mesh3DRenderingBufferSet;
};

namespace Framework
{
	class DoorInRoom;
	class Room;
	class PresenceLink;
	interface_class IModulesOwner;

	namespace Render
	{
		class Context;
		class RoomProxy;
		class SceneBuildContext;

		/*
		 *	Represents object's presence proxy (object limited by clip planes)
		 */
		class PresenceLinkProxy
		: public SoftPooledObject<PresenceLinkProxy>
		{
		public:
			static PresenceLinkProxy* build_all_in_room(SceneBuildContext & _context, RoomProxy* _forRoomProxy, PresenceLink const * _link, DoorInRoom const * _throughDoorFromOuter, PresenceLinkProxy* _addToPresenceLinks = nullptr);

			void add_all_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBuffer & _renderingBuffer, Matrix44 const & _placement = Matrix44::identity, bool _placementOverride = false) const;
			void add_all_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBufferSet & _renderingBuffers, Matrix44 const & _placement = Matrix44::identity, bool _placementOverride = false) const;

			static void add_to_list(REF_ PresenceLinkProxy * & _list, PresenceLinkProxy * _toAdd);

			static PresenceLinkProxy* build(SceneBuildContext & _context, PresenceLink const * _link, DoorInRoom const * _throughDoor, Name const & _preferredAppearanceName = Name::invalid(), bool _onlyIfNameMatches = false);
			static void build_owner_for_foreground(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link);
			static void build_owner_for_foreground_owner_local(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link);
			static void build_owner_for_foreground_camera_local(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link);
			static void build_owner_for_foreground_camera_local_owner_orientation(SceneBuildContext & _context, Name const & _appearanceTag, std::function<void(PresenceLinkProxy*)> _do_for_every_link);

			static void build_owner_for_foreground_using_placement_in_room(SceneBuildContext & _context, Name const & _appearanceTag, Matrix44 const & _placementInRoom, std::function<void(PresenceLinkProxy*)> _do_for_every_link, tchar const * _debugInfo);

#ifndef LIGHTS_PER_ROOM_ONLY
			LightSourceSetForPresenceLinkProxy const & get_lights() const { return lights; }
#endif

			Matrix44 const & get_placement_in_room() const { return placementInRoomMat; }

		protected:
			override_ void on_release();

		private:
			PresenceLinkProxy* next;

			// stored appearance
			Meshes::Mesh3DInstance appearanceCopy;
			Matrix44 placementInRoomMat; // for front plane it is placement in camera's room (this is presence link's placement in room for rendering)
			PlaneSet clipPlanes; // per eye

#ifndef LIGHTS_PER_ROOM_ONLY
			LightSourceSetForPresenceLinkProxy lights; // stored selected lights
#endif

			static inline void get_display_for(SceneBuildContext & _context, PresenceLinkProxy* _link, IModulesOwner const * _owner);
		};

	};
};

TYPE_AS_CHAR(Framework::Render::PresenceLinkProxy);
