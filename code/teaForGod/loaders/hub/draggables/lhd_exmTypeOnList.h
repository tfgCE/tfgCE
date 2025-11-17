#pragma once

#include "lhd_exmType.h"

#include "..\interfaces\lhi_exmTypeProvider.h"

namespace Loader
{
	namespace HubDraggables
	{
		class EXMTypeOnList
		: public EXMType
		, public HubInterfaces::IEXMTypeProvider
		{
			FAST_CAST_DECLARE(EXMTypeOnList);
			FAST_CAST_BASE(EXMType);
			FAST_CAST_BASE(HubInterfaces::IEXMTypeProvider);
			FAST_CAST_END();
		public:
			Name id;
			String desc;

			EXMTypeOnList(TeaForGodEmperor::EXMType const* _exmType);

			static int compare_refs(void const* _a, void const* _b);

		public: // IEXMTypeProvider
			implement_ TeaForGodEmperor::EXMType const* get_exm_type() const { return exmType; }
		};
	};
};

