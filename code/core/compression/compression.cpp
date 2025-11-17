#include "compression.h"

#include "bzlib.h"

#include "..\io\file.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

bool Compression::compress(IO::Interface& _input, IO::Interface& _output)
{
	bz_stream bzStream;

	bzStream.bzalloc = nullptr;
	bzStream.bzfree = nullptr;
	bzStream.opaque = nullptr;
	int result = BZ2_bzCompressInit(&bzStream, 1, 0, 30);

	if (result != BZ_OK)
	{
		return false;
	}

	int totalReadSize = 0;
	Array<int8> bufferIn;
	Array<int8> bufferOut;
	int bufferSize = 100000;
	bufferIn.set_size(bufferSize);
	bufferOut.set_size(bufferSize);
	while (true)
	{
		int readSize = _input.read(bufferIn.get_data(), bufferSize);
		totalReadSize += readSize;
		bzStream.next_out = bufferOut.get_data();
		bzStream.avail_out = bufferSize;
		int result;
		if (readSize > 0)
		{
			bzStream.next_in = bufferIn.get_data();
			bzStream.avail_in = readSize;
			result = BZ2_bzCompress(&bzStream, BZ_RUN);
		}
		else if (totalReadSize > 0)
		{
			result = BZ2_bzCompress(&bzStream, BZ_FINISH);
		}
		else
		{
			warn(TXT("nothing to compress, nothing provided"));
			break;
		}
		if (bzStream.avail_out < (uint)bufferSize) // how much did we use
		{
			_output.write(bufferOut.get_data(), bufferSize - bzStream.avail_out);
		}

		if (result == BZ_STREAM_END)
		{
			break;
		}
	}

	BZ2_bzCompressEnd(&bzStream);

	return true;
}
