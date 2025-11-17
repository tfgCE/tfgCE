#include "..\casts.h"

RefCountObject::RefCountObject()
: refCount( 0 )
{
}

void RefCountObject::add_ref() const
{
	refCount.increase();
}

bool RefCountObject::release_ref() const
{
	an_assert(refCount.get() > 0, TXT("wasn't this object already destroyed?"));
    if (refCount.decrease() == 0)
    {
		destroy_ref_count_object();
		return true;
    }
    else
    {
        return false;
    }
}

void RefCountObject::destroy_ref_count_object() const
{
	cast_to_nonconst(this)->destroy_ref_count_object();
}

void RefCountObject::destroy_ref_count_object()
{
    delete this;
}
