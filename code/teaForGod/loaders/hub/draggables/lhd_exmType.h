#pragma once

#include "..\loaderHubDraggableData.h"

namespace TeaForGodEmperor
{
	class EXMType;
};

namespace Loader
{
	namespace HubDraggables
	{
		class EXMType
		: public Loader::IHubDraggableData
		{
			FAST_CAST_DECLARE(EXMType);
			FAST_CAST_BASE(Loader::IHubDraggableData);
			FAST_CAST_END();
		public:
			EXMType(TeaForGodEmperor::EXMType const* _exmType = nullptr) : exmType(_exmType) {}
			
			TeaForGodEmperor::EXMType const * exmType = nullptr;
		};
	};
};
