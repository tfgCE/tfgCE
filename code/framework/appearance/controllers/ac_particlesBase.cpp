#include "ac_particlesBase.h"

#include "..\appearanceControllerPoseContext.h"

#include "..\meshParticles.h"

#include "..\..\debug\previewGame.h"
#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\object\temporaryObject.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\other\parserUtils.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

REGISTER_FOR_FAST_CAST(ParticlesBase);

ParticlesBase::ParticlesBase(ParticlesBaseData const * _data)
: base(_data)
, particlesBaseData(_data)
{
	rg.set_seed(this);
	if (particlesBaseData->deactivateAtVar.is_set())
	{
		deactivateAtVar = particlesBaseData->deactivateAtVar.get();
	}
}

ParticlesBase::~ParticlesBase()
{
}

void ParticlesBase::desire_to_deactivate()
{
	particles_deactivate();
}

void ParticlesBase::particles_deactivate()
{
	if (particlesActive)
	{
		particlesActive = false;
		if (auto* to = get_owner()->get_owner()->get_as_temporary_object())
		{
			to->particles_deactivated();
		}
	}
}

void ParticlesBase::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	deactivateAtVar.look_up<bool>(get_owner()->get_owner()->access_variables());
}

void ParticlesBase::activate()
{
	base::activate();

	particlesActive = true;
	if (auto* to = get_owner()->get_owner()->get_as_temporary_object())
	{
		to->particles_activated();
	}

	if (particlesBaseData->get_time_to_live().is_set())
	{
		timeToLive = rg.get(particlesBaseData->get_time_to_live().get());
	}
	else
	{
		timeToLive.clear();
	}
}

void ParticlesBase::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);

	if (particlesActive)
	{
		if (deactivateAtVar.is_valid() &&
			deactivateAtVar.get<bool>())
		{
			desire_to_deactivate();
		}
		if (auto* p = get_owner()->get_owner()->get_presence())
		{
			if (auto* attachedTo = p->get_attached_to())
			{
				int boneIdx;
				int socketIdx;
				Transform relativePlacement;
				if (p->get_attached_to_bone_info(boneIdx, relativePlacement))
				{
					// something on the way is scaled to zero
					if (attachedTo->get_appearance()->get_final_pose_LS()->get_bone_ms_from_ls(boneIdx).get_scale() == 0.0f)
					{
						desire_to_deactivate();
					}
				}
				if (p->get_attached_to_socket_info(socketIdx, relativePlacement))
				{
					if (attachedTo->get_appearance()->calculate_socket_ms(socketIdx).get_scale() == 0.0f)
					{
						desire_to_deactivate();
					}
				}
			}
			else
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (! fast_cast<PreviewGame>(Game::get()))
#endif
				if (particlesBaseData->deactivateIfNotAttached)
				{
					desire_to_deactivate();
				}
			}
		}
	}
	if (timeToLive.is_set())
	{
		if (timeToLive.get() >= 0.0f)
		{
			timeToLive = timeToLive.get() - _context.get_delta_time();
			if (timeToLive.get() < 0.0f)
			{
				desire_to_deactivate();
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(ParticlesBaseData);

AppearanceControllerData* ParticlesBaseData::create_data()
{
	return new ParticlesBaseData();
}

bool ParticlesBaseData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	deactivateIfNotAttached = _node->get_bool_attribute_or_from_child_presence(TXT("deactivateIfNotAttached"), deactivateIfNotAttached);

	timeToLive.load_from_xml(_node, TXT("timeToLive"));

	deactivateAtVar.load_from_xml(_node, TXT("deactivateAtVarID"));

	return result;
}

bool ParticlesBaseData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void ParticlesBaseData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	deactivateAtVar.fill_value_with(_context);
}

