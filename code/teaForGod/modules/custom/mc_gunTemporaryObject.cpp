#include "mc_gunTemporaryObject.h"

#include "..\..\utils.h"
#include "..\..\..\framework\module\moduleAppearanceImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\object\temporaryObject.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// object variables / appearance uniforms
DEFINE_STATIC_NAME(projectileEmissiveColour);
DEFINE_STATIC_NAME(projectileEmissiveBaseColour);

// appearance uniforms
DEFINE_STATIC_NAME(emissiveColour); // we use emissive directly, without emissive controller!
DEFINE_STATIC_NAME(emissiveBaseColour);

// module params
DEFINE_STATIC_NAME(autoGetEmissiveColours);
DEFINE_STATIC_NAME(dontAutoGetEmissiveColours);

//

REGISTER_FOR_FAST_CAST(GunTemporaryObject);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new GunTemporaryObject(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & GunTemporaryObject::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("gunTemporaryObject")), create_module);
}

GunTemporaryObject::GunTemporaryObject(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

GunTemporaryObject::~GunTemporaryObject()
{
}

void GunTemporaryObject::reset()
{
	// clear them
	projectileEmissiveColour = Colour::none;
	projectileEmissiveBaseColour = Colour::none;
	get_owner()->access_variables().access<Colour>(NAME(projectileEmissiveColour)) = projectileEmissiveColour;
	get_owner()->access_variables().access<Colour>(NAME(projectileEmissiveBaseColour)) = projectileEmissiveBaseColour;

	base::reset();
}

void GunTemporaryObject::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	autoGetEmissiveColours = _moduleData->get_parameter<bool>(this, NAME(autoGetEmissiveColours), autoGetEmissiveColours);
	autoGetEmissiveColours = !_moduleData->get_parameter<bool>(this, NAME(dontAutoGetEmissiveColours), !autoGetEmissiveColours);
}

void GunTemporaryObject::activate()
{
	base::activate();

	an_assert(fast_cast<Framework::TemporaryObject>(get_owner()), TXT("this module should only be used by temporary objects"));

	// update from what we were set with
	if (autoGetEmissiveColours)
	{
		projectileEmissiveColour = get_owner()->get_variables().get_value<Colour>(NAME(projectileEmissiveColour), Colour::none);
		projectileEmissiveBaseColour = get_owner()->get_variables().get_value<Colour>(NAME(projectileEmissiveBaseColour), Colour::none);
	}
	else
	{
		projectileEmissiveColour = Colour::none;
		projectileEmissiveBaseColour = Colour::none;
	}

	// rest should be copied via copyMaterialParameters

	// set it just one time
	an_assert(get_owner()->get_appearances().get_size() == 1, TXT("only single appearance handled"));
	if (auto* ap = get_owner()->get_appearance())
	{
		if (autoGetEmissiveColours)
		{
			Utils::get_projectile_emissives_from_appearance(ap, OUT_ projectileEmissiveColour, OUT_ projectileEmissiveBaseColour);
			Utils::calculate_projectile_emissive_colour_base(projectileEmissiveColour, projectileEmissiveBaseColour);
		}

		// set projectile emissives to appearance
		if (!projectileEmissiveColour.is_none())
		{
			an_assert(!projectileEmissiveBaseColour.is_none());
			ap->set_shader_param(NAME(emissiveColour), projectileEmissiveColour.to_vector4());
			ap->set_shader_param(NAME(emissiveBaseColour), projectileEmissiveBaseColour.to_vector4());
		}
	}
}
