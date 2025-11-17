#include "copyToMainConfig.h"

#include "concurrency\scopedSpinLock.h"
#include "io\xml.h"
#include "tags\tagCondition.h"
#include "system\core.h"

//

bool CopyToMainConfig::load_to_be_copied_to_main_config_xml(IO::XML::Node const* _node)
{
	bool copyToMainConfigNow = _node->get_bool_attribute_or_from_child_presence(TXT("copyToMainConfig"))
							|| _node->get_bool_attribute_or_from_child_presence(TXT("alwaysCopyToMainConfig"));
	if (_node->has_attribute(TXT("copyToMainConfigForSystemTag")))
	{
		TagCondition systemTagRequired;
		if (systemTagRequired.load_from_xml_attribute(_node, TXT("copyToMainConfigForSystemTag")))
		{
			if (systemTagRequired.check(System::Core::get_system_tags()))
			{
				copyToMainConfigNow = true;
			}
		}
	}

	if (copyToMainConfigNow)
	{
		Concurrency::ScopedSpinLock lock(copyToMainConfigLock);
		copyToMainConfig.push_back(IO::MemoryStorage());
		auto& ms = copyToMainConfig.get_last();
		ms.set_type(IO::InterfaceType::Text);
		_node->save_single_node_to_xml(&ms);
	}
	return true;
}

void CopyToMainConfig::save_to_be_copied_to_main_config_xml(IO::XML::Node* _containerNode)
{
	Concurrency::ScopedSpinLock lock(copyToMainConfigLock);
	for_every(ctmc, copyToMainConfig)
	{
		int line;
		bool error;
		ctmc->go_to(0); // read from the beginning
		RefCountObjectPtr<IO::XML::Node> mainConfigNode;
		mainConfigNode = IO::XML::Node::load_single_node_from_xml(ctmc, cast_to_nonconst(_containerNode->get_document()), line, error);
		if (mainConfigNode.get())
		{
			if (auto* attr = mainConfigNode->find_attribute(TXT("copyToMainConfig")))
			{
				mainConfigNode->delete_attribute(attr);
			}
			if (auto* attr = mainConfigNode->find_attribute(TXT("alwaysCopyToMainConfig")))
			{
				mainConfigNode->delete_attribute(attr);
			}
			if (auto* attr = mainConfigNode->find_attribute(TXT("copyToMainConfigForSystemTag")))
			{
				mainConfigNode->delete_attribute(attr);
			}
			_containerNode->add_node(mainConfigNode.get());
		}
	}
}

