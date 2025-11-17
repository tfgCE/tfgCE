#include "tools.h"

#include "..\core\mainSettings.h"

void Tools::initialise_static()
{
	if (MainSettings::global().should_allow_importing())
	{
		FBX::initialise_static();
		SimpleImporters::initialise_static();
	}
}

void Tools::close_static()
{
	if (MainSettings::global().should_allow_importing())
	{
		FBX::close_static();
		SimpleImporters::close_static();
	}
}

void Tools::finished_importing()
{
	if (MainSettings::global().should_allow_importing())
	{
		FBX::Importer::finished_importing();
		SimpleImporters::finished_importing();
	}
}
