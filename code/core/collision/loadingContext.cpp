#include "loadingContext.h"

using namespace Collision;

REGISTER_FOR_FAST_CAST(LoadingContext);

LoadingContext::LoadingContext()
: create_physical_material_func(nullptr)
{
}

LoadingContext::~LoadingContext()
{
}

void LoadingContext::set_create_physical_material(CreatePhysicalMaterial _cpm)
{
	create_physical_material_func = _cpm;
}
