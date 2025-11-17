#pragma once

#include "moduleData.h"
#include "..\collision\collisionTrace.h"
#include "..\..\core\collision\collisionFlags.h"
#include "..\..\core\math\math.h"
#include "..\..\core\containers\array.h"

namespace Framework
{
	class Room;

	struct PresenceCollisionTrace
	: public CollisionTrace
	{
		typedef CollisionTrace base;
	public:
		PresenceCollisionTrace(): base() {}
		PresenceCollisionTrace(CollisionTraceInSpace::Type _locationsIn) : base(_locationsIn) {}

		int get_priority() const { return priority; }

		bool load_from_xml(IO::XML::Node const * _node);
	private:

		int priority = 0;
	};

	class ModulePresenceData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModulePresenceData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();

		typedef ModuleData base;
	public:
		ModulePresenceData(LibraryStored* _inLibraryStored);

	public: // ModuleData
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:	friend class ModulePresence;

		Collision::Flags presenceTraceFlags;
		Collision::Flags presenceTraceRejectFlags;
		bool noBasedOnPresenceTraces = false;
		bool noGravityPresenceTraces = false;
		Array<PresenceCollisionTrace> basedOnPresenceTraces; // gets basedOn from first trace hit
		Array<PresenceCollisionTrace> gravityPresenceTraces; // uses priority to weight traces properly
	};
};


