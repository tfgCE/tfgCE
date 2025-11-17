#include "moduleCollisionHeaded.h"

#include "modules.h"
#include "moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// module params
DEFINE_STATIC_NAME(headBone);
DEFINE_STATIC_NAME(aboveHeadHeight);
DEFINE_STATIC_NAME(minHeadAltitude);
DEFINE_STATIC_NAME(headCollidesWithFlags);

//

REGISTER_FOR_FAST_CAST(ModuleCollisionHeaded);

static ModuleCollision* create_module(IModulesOwner* _owner)
{
	return new ModuleCollisionHeaded(_owner);
}

RegisteredModule<ModuleCollision>& ModuleCollisionHeaded::register_itself()
{
	return Modules::collision.register_module(String(TXT("headed")), create_module);
}

ModuleCollisionHeaded::ModuleCollisionHeaded(IModulesOwner* _owner)
: base(_owner)
{

}

ModuleCollisionHeaded::~ModuleCollisionHeaded()
{

}

void ModuleCollisionHeaded::reset()
{
	head.invalidate();
}

void ModuleCollisionHeaded::setup_with(ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);
	
	if (_moduleData)
	{
		head.set_name(_moduleData->get_parameter<Name>(this, NAME(headBone), head.get_name()));
		aboveHeadHeight = _moduleData->get_parameter<float>(this, NAME(aboveHeadHeight), aboveHeadHeight);
		minHeadAltitude = _moduleData->get_parameter<float>(this, NAME(minHeadAltitude), minHeadAltitude);
		headCollidesWithFlags.apply(_moduleData->get_parameter<String>(this, NAME(headCollidesWithFlags)));
	}
}

void ModuleCollisionHeaded::update_collision_primitive_using_head()
{
	if (auto* a = get_owner()->get_appearance())
	{
		if (auto* skeleton = a->get_skeleton())
		{
			if (auto* s = skeleton->get_skeleton())
			{
				if (!head.is_valid() && head.is_name_valid())
				{
					head.look_up(s);
				}
				if (head.is_valid())
				{
					// this way it will work for any case, will just adjust to actual head placement
					float height = 0.0f;
					{
						Transform headOS = a->get_ms_to_os_transform().to_world(a->get_final_pose_MS()->get_bone(head.get_index()));
						height = headOS.get_translation().z + aboveHeadHeight;
					}
					if (height > 0.0f)
					{
						update_collision_primitive_height(height);
					}
				}
			}
		}
	}
}
