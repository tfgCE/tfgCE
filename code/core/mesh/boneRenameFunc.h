#pragma once

#include <functional>

struct Name;

namespace Meshes
{
	typedef std::function<Name(Name const & _name, Optional<Vector3> const & _loc)> BoneRenameFunc;
};
