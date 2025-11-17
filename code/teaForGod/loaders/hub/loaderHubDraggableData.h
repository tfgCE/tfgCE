#pragma once

#include "..\..\..\core\fastCast.h"
#include "..\..\..\core\memory\refCountObject.h"
#include "..\..\..\core\types\name.h"

#include "..\..\tutorials\tutorialHubId.h"

namespace Loader
{
	interface_class IHubDraggableData
	: public RefCountObject
	{
		FAST_CAST_DECLARE(IHubDraggableData);
		FAST_CAST_END();

	public:
		TeaForGodEmperor::TutorialHubId tutorialHubId;
	};
};
