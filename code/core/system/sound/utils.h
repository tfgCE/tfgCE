#pragma once

#ifdef AN_FMOD
#include "fmod.hpp"
#include "fmod_studio.hpp"
#endif

#include "..\..\math\math.h"

namespace System
{
	namespace SoundUtils
	{
#ifdef AN_FMOD
		inline FMOD_VECTOR convert(Vector3 const & _vector);
#endif
	};
};

#include "utils.inl"
