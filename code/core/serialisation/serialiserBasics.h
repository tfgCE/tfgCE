#pragma once

/**
 *	This is minimal set of serialiser related functionality/declarations.
 */

class Serialiser;

namespace IO
{
	class File;
};

template <typename Class>
bool serialise_data(Serialiser & _serialiser, Class & _data);

#define DECLARE_SERIALISER_FOR(Type) template <> bool serialise_data<Type>(Serialiser & _serialiser, Type & _data);
