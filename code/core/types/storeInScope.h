#pragma once

#define STORE_IN_SCOPE(type, variable) StoreInScope<type> copyOf_##variable(variable);

/**
 *	Stores and restores value of elemenet
 */
template <typename Element>
struct StoreInScope
{
public:
	StoreInScope(Element & _source);
	~StoreInScope();

private:
	Element & source;
	Element copy;
};

#include "storeInScope.inl"
