#include "physicalMaterial.h"

#include "loadingContext.h"

using namespace Collision;

REGISTER_FOR_FAST_CAST(PhysicalMaterial);

PhysicalMaterial::PhysicalMaterial()
: collisionFlags(Flags::defaults())
{
}

PhysicalMaterial::~PhysicalMaterial()
{
}

bool PhysicalMaterial::load_from_xml(IO::XML::Node const * _node, LoadingContext const & _loadingContext)
{
	bool result = true;

	result &= collisionInfoProvider.load_from_xml(_node);

	if (IO::XML::Node const * child = _node->first_child_named(TXT("collisionFlags")))
	{
		result &= collisionFlags.load_from_xml(child);
	}

	return result;
}

bool PhysicalMaterial::load_from_xml(IO::XML::Attribute const * _attr, LoadingContext const & _loadingContext)
{
	an_assert(false, TXT("can't load material from attribute - maybe you wanted to load stub or something?"));
	return false;
}

bool PhysicalMaterial::load_material_from_xml(RefCountObjectPtr<PhysicalMaterial>& _material, IO::XML::Node const * _node, LoadingContext const & _loadingContext)
{
	bool result = true;
	Name tempName;
	if (_loadingContext.should_load_material_from_attribute(OUT_ tempName))
	{
		if (IO::XML::Attribute const * attr = _node->get_attribute(tempName.to_char()))
		{
			_loadingContext.create_physical_material(_material);
			if (_material.is_set())
			{
				result &= _material->load_from_xml(attr, _loadingContext);
			}
			else
			{
				error_loading_xml(_node, TXT("could not create material"));
				result = false;
			}
		}
	}
	else if (_loadingContext.should_load_material_from_child(OUT_ tempName))
	{
		if (IO::XML::Node const * child = _node->first_child_named(tempName.to_char()))
		{
			_loadingContext.create_physical_material(_material);
			if (_material.is_set())
			{
				result &= _material->load_from_xml(child, _loadingContext);
			}
			else
			{
				error_loading_xml(child, TXT("could not create material"));
				result = false;
			}
		}
	}
	else
	{
		an_assert(false, TXT("not loading anything?"));
	}
	return result;
}

void PhysicalMaterial::apply_to(REF_ CheckSegmentResult & _result) const
{
	collisionInfoProvider.apply_to(_result);
}

void PhysicalMaterial::log_usage_info(LogInfoContext & _context) const
{

}
