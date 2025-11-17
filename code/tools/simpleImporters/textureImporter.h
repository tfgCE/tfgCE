#pragma once

namespace SimpleImporters
{
	class TextureImporter
	{
	public:
		static void initialise_static();
		static void close_static();
		static void finished_importing();
	};
};
