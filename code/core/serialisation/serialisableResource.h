#pragma once

#include "iSerialisable.h"
#include "..\io\digested.h"

class SerialisableResource
: public virtual ISerialisable
{
	FAST_CAST_DECLARE(SerialisableResource);
	FAST_CAST_BASE(ISerialisable);
	FAST_CAST_END();

public:
	SerialisableResource();
	virtual ~SerialisableResource() {}

	void digest_source(IO::Interface* _source);
	void set_digested_definition(IO::Digested const & _digestedDefinition) { digestedDefinition = _digestedDefinition; }
	bool check_if_actual(IO::Interface* _source, IO::Digested const & _digestedDefinition); // check if resource is actual against source and definition

	void serialise_resource_to_file(String const & _fileName);

	void set_digested_source(IO::Digested const & _digestedSource) { digestedSource = _digestedSource; }
	IO::Digested const & get_digested_source() const { return digestedSource; }

public: // ISerialisable
	virtual bool serialise(Serialiser & _serialiser);

protected:
	uint16 version; // more than 16384 versions would mean serious problems. to be honest, 256 versions is A LOT
	IO::Digested digestedSource; // this is used to check if source (imported file) has changed
	IO::Digested digestedDefinition; // might be used for various things, for example import information from xml - with this we can check if import definition or other parameters changed - might be invalid/null
};
