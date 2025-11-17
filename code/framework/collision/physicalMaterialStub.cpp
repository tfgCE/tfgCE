#include "physicalMaterialStub.h"

#include "physicalMaterial.h"
#include "physicalMaterialLoadingContext.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(PhysicalMaterialStub);

PhysicalMaterialStub::PhysicalMaterialStub()
{
	be<PhysicalMaterialStub>();
}

bool PhysicalMaterialStub::load_from_xml(IO::XML::Node const * _node, Collision::LoadingContext const & _loadingContext)
{
	an_assert(false, TXT("should not be used to load from child"));
	return false;
}

bool PhysicalMaterialStub::load_from_xml(IO::XML::Attribute const * _attr, Collision::LoadingContext const & _loadingContext)
{
	PhysicalMaterialLoadingContext const * physicalMaterialLoadingContext = fast_cast<PhysicalMaterialLoadingContext>(&_loadingContext);
	an_assert(physicalMaterialLoadingContext);
	return libraryName.setup_with_string(_attr->get_as_string(), physicalMaterialLoadingContext->get_library_loading_context());
}
