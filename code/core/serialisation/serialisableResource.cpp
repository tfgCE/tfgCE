#include "serialisableResource.h"

#include "serialiser.h"

#include "..\io\file.h"

REGISTER_FOR_FAST_CAST(SerialisableResource);

SerialisableResource::SerialisableResource()
: version(0)
{
}

bool SerialisableResource::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, version);
	result &= serialise_data(_serialiser, digestedSource);
	result &= serialise_data(_serialiser, digestedDefinition);
	return result;
}

void SerialisableResource::digest_source(IO::Interface* _source)
{
	digestedSource.digest(_source);
}

bool SerialisableResource::check_if_actual(IO::Interface* _source, IO::Digested const & _digestedDefinition)
{
	IO::Digested givenSourceDigested(_source);
	an_assert(givenSourceDigested.is_valid(), TXT("there was a problem with digesting source file, maybe we should not get to this point at all?"));
	an_assert(!digestedDefinition.is_valid() || _digestedDefinition.is_valid(), TXT("provided definition should be digested, maybe we missed digest_definition somewhere on the way?"));
	return digestedSource == givenSourceDigested && (!digestedDefinition.is_valid() || digestedDefinition == _digestedDefinition);
}

void SerialisableResource::serialise_resource_to_file(String const & _fileName)
{
	if (!_fileName.is_empty())
	{
		// store newly imported
		IO::File file;
		file.create(_fileName);
		file.set_type(IO::InterfaceType::Binary);
		if (file.is_open())
		{
			serialise(Serialiser::writer(&file).temp_ref());
		}
	}
}