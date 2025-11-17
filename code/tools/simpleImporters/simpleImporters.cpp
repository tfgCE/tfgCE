#include "simpleImporters.h"
#include "textureImporter.h"

void SimpleImporters::initialise_static()
{
	TextureImporter::initialise_static();
}

void SimpleImporters::close_static()
{
	TextureImporter::close_static();
}

void SimpleImporters::finished_importing()
{
	TextureImporter::finished_importing();
}