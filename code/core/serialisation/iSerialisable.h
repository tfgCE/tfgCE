#pragma once

#include "..\globalDefinitions.h"
#include "..\fastCast.h"

class Serialiser;

interface_class ISerialisable
{
	FAST_CAST_DECLARE(ISerialisable);
	FAST_CAST_END();
public:
	virtual bool serialise(Serialiser & _serialiser) = 0;
};
