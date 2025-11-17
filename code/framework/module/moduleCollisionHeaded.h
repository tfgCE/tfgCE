#pragma once

#include "moduleCollision.h"

#include "..\..\core\mesh\boneID.h"

namespace Framework
{
	class ModuleCollisionHeaded
	: public Framework::ModuleCollision
	{
		FAST_CAST_DECLARE(ModuleCollisionHeaded);
		FAST_CAST_BASE(ModuleCollision);
		FAST_CAST_END();

		typedef ModuleCollision base;
	public:
		static RegisteredModule<ModuleCollision>& register_itself();

		ModuleCollisionHeaded(IModulesOwner* _owner);
		virtual ~ModuleCollisionHeaded();

	public: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const* _moduleData);

	public:
		void update_collision_primitive_using_head();

		float get_above_head_height() const { return aboveHeadHeight; }
		float get_min_head_altitude() const { return max(minHeadAltitude, gradientStepHole); } // to make it go above step and not get stuck on it

		Collision::Flags const& get_head_collides_with_flags() const { return headCollidesWithFlags; }

	private:
		Meshes::BoneID head;
		float aboveHeadHeight = 0.1f;
		float minHeadAltitude = 0.3f;

		Collision::Flags headCollidesWithFlags;
	};

};

