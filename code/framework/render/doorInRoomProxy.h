#pragma once

#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\mesh\mesh3dInstanceSetInlined.h"
#include "..\..\core\math\math.h"

namespace Meshes
{
	struct Mesh3DRenderingBuffer;
};

namespace Framework
{
	class DoorInRoom;
	class Room;

	namespace Render
	{
		class Context;
		class SceneBuildContext;
		class PresenceLinkProxy;
		class EnvironmentProxy;

		class DoorInRoomProxy
		: public SoftPooledObject<DoorInRoomProxy>
		{
		public:
			static DoorInRoomProxy * build(SceneBuildContext & _context, DoorInRoom const * _doorInRoom, DoorInRoomProxy *& _addTolist, bool _cameThrough);

			void add_all_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBufferSet & _renderingBuffers) const;

			Matrix44 const& get_placement_in_room() const { return placementInRoomMat; }

			Meshes::Mesh3DInstanceSetInlined & access_appearance() { return appearanceCopy; }

			bool did_came_through() const { return cameThrough; }

		protected: friend class SoftPool<DoorInRoomProxy>;
			override_ void on_release();

		private:
			bool cameThrough = false;
			DoorInRoomProxy* next;

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifndef AN_CLANG
			DoorInRoom* doorInRoom; // for reference (and debugger)
#endif
#endif

			Matrix44 placementInRoomMat;
			Meshes::Mesh3DInstanceSetInlined appearanceCopy;
		};

	};
};

TYPE_AS_CHAR(Framework::Render::DoorInRoomProxy);
