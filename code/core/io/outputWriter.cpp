#include "outputWriter.h"

#include "..\casts.h"

using namespace IO;

OutputWriter::OutputWriter()
: MemoryStorage()
{
	set_type(InterfaceType::Text);
}

OutputWriter::~OutputWriter()
{
	flush_to_output();
}

void OutputWriter::flush_to_output()
{
	for_every_ptr(buffer, buffers)
	{
		an_assert(sizeof(char8) == sizeof(byte));
		char8 asChar[bufSize + 1];
		memory_copy(asChar, buffer->data, sizeof(byte) * buffer->length);
		asChar[buffer->length] = 0;
		output(String::from_char8_utf(asChar).to_char())
	}
	clear();
}
