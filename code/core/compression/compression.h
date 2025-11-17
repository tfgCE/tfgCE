#pragma once

namespace IO
{
	class Interface;
};

namespace Compression
{
	bool compress(IO::Interface& _input, IO::Interface& _output);
};
