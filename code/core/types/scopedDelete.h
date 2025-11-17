#pragma once

#include "..\utils.h"

#define SCOPED_DELETE(type, variable) ScopedDelete<type> temp_variable_named(variable)(variable);

/**
 *	Deletes object when leaving scope
 *	Example:
 *	{
 *		Object* obj = new Object();
 *		SCOPED_DELETE(Object, obj);
 *
 *		// do stuff on obj
 *		if (...)
 *		{
 *			return;
 *		}
 *
 *		// do more stuff on obj
 *	}
 */
template <typename Element>
struct ScopedDelete
{
public:
	ScopedDelete(Element * _obj);
	~ScopedDelete();

private:
	Element * obj;
};

#include "scopedDelete.inl"
