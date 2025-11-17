#include "missionsDefinition.h"

#include "..\game\game.h"
#include "..\library\library.h"

#include "..\..\core\tags\tag.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(MissionsDefinition);
LIBRARY_STORED_DEFINE_TYPE(MissionsDefinition, missionsDefinition);

MissionsDefinition::MissionsDefinition(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

MissionsDefinition::~MissionsDefinition()
{
}

bool MissionsDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	sets.clear();
	for_every(node, _node->children_named(TXT("set")))
	{
		RefCountObjectPtr<MissionsDefinitionSet> mds;
		mds = new MissionsDefinitionSet();
		mds->id = node->get_name_attribute_or_from_child(TXT("id"), mds->id);

		for_every(n, node->children_named(TXT("starting")))
		{
			Framework::UsedLibraryStored<Pilgrimage> p;
			if (p.load_from_xml(n, TXT("pilgrimage"), _lc))
			{
				mds->startingPilgrimages.push_back(p);
			}
			else
			{
				result = false;
			}
		}
		
		mds->missionsTagged.load_from_xml_attribute_or_child_node(node, TXT("missionsTagged"));

		sets.push_back(mds);
	}

	lootProgressMilestones.load_from_xml_attribute_or_child_node(_node, TXT("lootProgressMilestones"));

	return result;
}

bool MissionsDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every_ref(set, sets)
	{
		for_every(p, set->startingPilgrimages)
		{
			result &= p->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
		}
	}

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		for_every_ref(set, sets)
		{
			set->missions.clear();
			if (auto* l = Library::get_current_as<Library>())
			{
				for_every_ptr(md, l->get_mission_definitions().get_tagged(set->missionsTagged))
				{
					set->missions.grow_size(1);
					set->missions.get_last() = md;
				}
			}
		}
	}

	return result;
}
