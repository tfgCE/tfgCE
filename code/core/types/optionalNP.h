#pragma once

template <typename Element> struct Optional;

struct NotProvided // to be used as default parameter for optional
{
	bool notProvided;
};

#define NP NotProvided()
