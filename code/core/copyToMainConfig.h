#pragma once

#include "containers\list.h"
#include "io\memoryStorage.h"

struct CopyToMainConfig
{
	List<IO::MemoryStorage> copyToMainConfig; // will copy thing from this storage to mainConfig.xml instead of utilising on the go
	Concurrency::SpinLock copyToMainConfigLock = Concurrency::SpinLock(TXT("copyToMainConfigLock"));

	bool load_to_be_copied_to_main_config_xml(IO::XML::Node const* _node);
	void save_to_be_copied_to_main_config_xml(IO::XML::Node* _containerNode);
};
