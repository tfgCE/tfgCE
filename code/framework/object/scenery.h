#pragma once

#include "spaceBlockingObject.h"

#include "sceneryType.h"

namespace Framework
{
	class Scenery
	: public SpaceBlockingObject
	{
		FAST_CAST_DECLARE(Scenery);
		FAST_CAST_BASE(SpaceBlockingObject);
		FAST_CAST_END();

		typedef SpaceBlockingObject base;
	public:
		Scenery(SceneryType const * _sceneryType, String const & _name); // requires initialisation
		virtual ~Scenery();

		bool is_room_scenery() const { return sceneryType && sceneryType->is_room_scenery(); }

		SceneryType const* get_scenery_type() const { return sceneryType; }

	public: // IModulesOwner
		override_ Name const & get_object_type_name() const;

	protected:
		SceneryType const * sceneryType = nullptr;
	};
};

DECLARE_REGISTERED_TYPE(Framework::Scenery*);
