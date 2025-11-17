#include "lhd_exmTypeOnList.h"

#include "..\..\..\library\exmType.h"

//

using namespace Loader;
using namespace HubDraggables;

//

REGISTER_FOR_FAST_CAST(EXMTypeOnList);

EXMTypeOnList::EXMTypeOnList(TeaForGodEmperor::EXMType const* _exmType)
: EXMType(_exmType)
, id(_exmType->get_id())
, desc(_exmType->get_ui_name().get())
{}

int EXMTypeOnList::compare_refs(void const * _a, void const * _b)
{
	RefCountObjectPtr<EXMTypeOnList> const & a = *(plain_cast<RefCountObjectPtr<EXMTypeOnList>>(_a));
	RefCountObjectPtr<EXMTypeOnList> const & b = *(plain_cast<RefCountObjectPtr<EXMTypeOnList>>(_b));
	return String::diff_icase(a->id.to_string(), b->id.to_string());
};
