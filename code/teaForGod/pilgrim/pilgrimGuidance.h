#pragma once

#include "..\..\core\math\math.h"

#include "..\game\playerSetup.h"

namespace Framework
{
	class IModulesOwner;
};

namespace TeaForGodEmperor
{
	struct PilgrimGuidance
	{
		Guidance::Type guidanceType = Guidance::Strength;

		bool validGuidance = false;
		Vector3 guidanceOwnerLocVR = Vector3::zero; // location in vr space
		Vector3 guidanceDirOS = Vector3::zero;

		/**
		 *	_guidanceSocketInOwner to get proper forward direction
		 */
		bool update(Framework::IModulesOwner const * _pilgrim, Framework::IModulesOwner const * _owner, float _deltaTime, Optional<bool> const & _forceUpdate, Name const & _guidanceSocketInOwner, Name const & _povSocketInPilgrim = Name::invalid(), float _usePOVLine = 0.0f);
	};
};
