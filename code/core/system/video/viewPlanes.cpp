#include "viewPlanes.h"

//

using namespace System;

//

ViewPlanes::ViewPlanes()
{
	SET_EXTRA_DEBUG_INFO(planes, TXT("ViewPlanes.planes"));
}

ViewPlanes::~ViewPlanes()
{
	clear();
}

void ViewPlanes::clear()
{
	planes.clear();
}
	
void ViewPlanes::add_plane(Plane const & _plane)
{
	planes.push_back(_plane);
}
